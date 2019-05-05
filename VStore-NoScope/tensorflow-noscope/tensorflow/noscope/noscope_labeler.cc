#include <sys/mman.h>

#include <chrono>
#include <ctime>
#include <random>
#include <algorithm>
#include <iterator>

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"

#include "tensorflow/core/common_runtime/gpu/gpu_device.h"
#include "tensorflow/core/platform/cuda.h"

#include "opencv2/opencv.hpp"

#include "tensorflow/noscope/measure.h"

#include "tensorflow/noscope/noscope_labeler.h"
#include "tensorflow/noscope/noscope_data.h"
#include "tensorflow/noscope/darknet/src/yolo.h"
#include "tensorflow/noscope/darknet/src/image.h"

//using namespace std;
//size_t reso;
namespace noscope {

NoscopeLabeler::NoscopeLabeler(tensorflow::Session *session,
                         yolo::YOLO *yolo_classifier,
                         noscope::filters::DifferenceFilter diff_filt,
                         const std::string& avg_fname,
                         const noscope::NoscopeData& data) :
    kNbFrames_(data.kNbFrames_),
    all_data_(data),
    frame_status_(kNbFrames_, kUnprocessed), labels_(kNbFrames_),
    kDifferenceFilter_(diff_filt),
    diff_confidence_(kNbFrames_),
    cnn_confidence_(kNbFrames_),
    yolo_(yolo_classifier),
    yolo_confidence_(kNbFrames_),
    avg_(NoscopeData::kDistResol_, CV_32FC3),
    session_(session) {
  std::ifstream is(avg_fname);
  std::istream_iterator<float> start(is), end;
  std::vector<float> nums(start, end);
  if (nums.size() != NoscopeData::kDistFrameSize_) {
  //if (nums.size() != reso * reso * 3) {
    throw std::runtime_error("nums not right size");
  }

  memcpy(avg_.data, &nums[0], NoscopeData::kDistFrameSize_ * sizeof(float));
  //memcpy(avg_.data, &nums[0], reso * reso * 3 * sizeof(float));
}

void NoscopeLabeler::RunDifferenceFilter(const float lower_thresh,
                                      const float upper_thresh,
                                      const bool const_ref,
                                      const size_t kRef) {

  int countf = 0;
  int countu = 0;

  k2_measure("startDF");

  const std::vector<uint8_t>& kFrameData = all_data_.diff_data_;
  const int kFrameSize = NoscopeData::kDiffFrameSize_;
  //const int kFrameSize = reso * reso * 3;

  I("i = %d", kNbFrames_);
  #pragma omp parallel for num_threads(kNumThreads_) schedule(static)
  for (size_t i = kDiffDelay_; i < kNbFrames_; i++) {
    const uint8_t *kRefImg = const_ref ?
        &kFrameData[kRef * kFrameSize] :
        &kFrameData[(i - kDiffDelay_) * kFrameSize];

    //uint8_t *kRefImg = const_ref ?
                             //&kFrameData[kRef * kFrameSize] :
                             //&kFrameData[(i - kDiffDelay_) * kFrameSize];
    float tmp = kDifferenceFilter_.fp(&kFrameData[i * kFrameSize], kRefImg);

    diff_confidence_[i] = tmp;
    if (tmp < lower_thresh) {
      labels_[i] = false;
      frame_status_[i] = kDiffFiltered;
      countf = countf+1;
      //std::cout << "1" << std::endl;
    } else {
      frame_status_[i] = kDiffUnfiltered;
      countu = countu+1;
      //std::cout << "0" << std::endl;
    }
  }
 

  for (size_t i = kDiffDelay_; i < kNbFrames_; i++){
    if (frame_status_[i] == kDiffUnfiltered){
      cnn_frame_ind_.push_back(i);
      
    }
  }
  k2_measure("endDF");
  k2_measure_flush();



  std::cout << "kDiffDelay_ " << kDiffDelay_ << " kNbFrames_ " << kNbFrames_ << " lower_thresh " << lower_thresh << " frames filtered by DF: " << countf << " frames unfiltered by DF: " << countu << "\n";
}

void NoscopeLabeler::PopulateCNNFrames() {
  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < kDiffDelay_; i++) cnn_frame_ind_.push_back(i);

  const std::vector<float>& kDistData = all_data_.dist_data_;
  const int kFrameSize = NoscopeData::kDistFrameSize_;
  //const int kFrameSize = reso * reso * 3;

  using namespace tensorflow;
  const size_t kNbCNNFrames = cnn_frame_ind_.size();
  const size_t kNbLoops = (kNbCNNFrames + kMaxCNNImages_ - 1) / kMaxCNNImages_;
  const float* avg = (float *) avg_.data;
  for (size_t i = 0; i < kNbLoops; i++) {
    const size_t kImagesToRun =
        std::min(kMaxCNNImages_, cnn_frame_ind_.size() - i * kMaxCNNImages_);
    Tensor input(DT_FLOAT,
                 TensorShape({kImagesToRun,
                             NoscopeData::kDistResol_.height,
                             NoscopeData::kDistResol_.width,
                              //reso,
                              //reso,
                             kNbChannels_}));
    auto input_mapped = input.tensor<float, 4>();
    float *tensor_start = &input_mapped(0, 0, 0, 0);
    #pragma omp parallel for
    for (size_t j = 0; j < kImagesToRun; j++) {
      const size_t kImgInd = i * kMaxCNNImages_ + j;
      float *output = tensor_start + j * kFrameSize;
      const float *input = &kDistData[cnn_frame_ind_[kImgInd] * kFrameSize];
      for (size_t k = 0; k < kFrameSize; k++)
        output[k] = input[k] / 255. - avg[k];
    }
    dist_tensors_.push_back(input);
  }


  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;
  // std::cout << "PopulateCNNFrames time: " << diff.count() << " s" << std::endl;
}

void NoscopeLabeler::RunSmallCNN(const float lower_thresh, const float upper_thresh) {
  
  using namespace tensorflow;
  int countf=0;
  int countu = 0;
  int counttrue = 0;
  int countfalse = 0;
  // Round up

  k2_measure("start small CNN");
  const size_t kNbCNNFrames = cnn_frame_ind_.size();
  const size_t kNbLoops = (kNbCNNFrames + kMaxCNNImages_ - 1) / kMaxCNNImages_;

  I("i = %d", kNbLoops);

  for (size_t i = 0; i < kNbLoops; i++) {
    const size_t kImagesToRun =
        std::min(kMaxCNNImages_, cnn_frame_ind_.size() - i * kMaxCNNImages_);
    auto input = dist_tensors_[i];
    /*cudaHostRegister(&(input.tensor<float, 4>()(0, 0, 0, 0)),
                     kImagesToRun * kFrameSize * sizeof(float),
                     cudaHostRegisterPortable);*/


    std::vector<tensorflow::Tensor> outputs;
    std::vector<std::pair<string, tensorflow::Tensor> > inputs = {
      {"input_img", input},
      // {"keras_learning_phase", learning_phase},
    };

    tensorflow::Status status = session_->Run(inputs, {"output_prob"}, {}, &outputs);
    // TF_CHECK_OK(status);
    // FIXME: should probably check the tensor output size here.

    {
      auto output_mapped = outputs[0].tensor<float, 2>();
      //I("i = %d", kImagesToRun);

      for (size_t j = 0; j < kImagesToRun; j++) {
        Status s; //put semicolon here
        const int kInd = cnn_frame_ind_[i * kMaxCNNImages_ + j];
        cnn_confidence_[kInd] = output_mapped(j, 1);
        if (output_mapped(j, 1) < lower_thresh) {
          labels_[kInd] = false;
          s = kDistillFiltered;
          countf = countf+1;
          countfalse = countfalse +1;

          std::cout << j << " " << "0" << std::endl;

        } else if (output_mapped(j, 1) > upper_thresh) {
          labels_[kInd] = true;
          s = kDistillFiltered;
          countf = countf+1;
          counttrue = counttrue +1;

          std::cout << j << " " << "1" << std::endl;

        } else {
          s = kDistillUnfiltered;
          countu = countu+1;

          std::cout << j << " " << "2" << std::endl;

        }
        frame_status_[kInd] = s;
      }
    }
  }
  k2_measure("end small CNN");
  k2_measure_flush();

  std::cout << "frames filtered by smallCNN: " << countf << " frames unfiltered by smallCNN: " << countu << " True labels: " << counttrue << " False labels: " << countfalse << " upper thresh: " << upper_thresh << " lower thresh" << lower_thresh << "\n";
}

static image ipl_to_image(IplImage* src) {
  unsigned char *data = (unsigned char *)src->imageData;
  // float *data = (float *) src->imageData;

  int h = src->height;
  int w = src->width;
  int c = src->nChannels;
  int step = src->widthStep;// / sizeof(float);
  image out = make_image(w, h, c);
  int count = 0;

  for (int k = 0; k < c; ++k) {
    for (int i = 0; i < h; ++i) {
      for (int j = 0; j < w; ++j) {
        out.data[count++] = data[i*step + j*c + (2 - k)] / 255.;
        // out.data[count++] = data[i*step + j*c + k] / 255.;
      }
    }
  }
  return out;
}
static void noscope_rgbgr_image(image im) {
  int i;

  for (i = 0; i < im.w*im.h; ++i) {
    float swap = im.data[i];
    im.data[i] = im.data[i+im.w*im.h*2];
    im.data[i+im.w*im.h*2] = swap;
  }
}

void NoscopeLabeler::RunYOLO(const bool actually_run) {
  int labeled_by_yolo1 = 0;
  int labeled_by_yolo2 = 0;
  k2_measure("StartYOLO");
  I("i = %d", kNbFrames_);

  if (actually_run) {
    for (size_t i = 0; i < kNbFrames_; i++) {
      // Run YOLO on every unprocessed frame
      if (frame_status_[i] == kDistillFiltered)
        continue;
      if (frame_status_[i] == kDiffFiltered)
        continue;
      cv::Mat cpp_frame(NoscopeData::kYOLOResol_, CV_8UC3,
      const_cast<uint8_t *>(&all_data_.yolo_data_[i * all_data_.kYOLOFrameSize_]));
      //const_cast<uint8_t *>(&all_data_.yolo_data_[i * reso * reso* 3]));
      //I("i = %d", i);
      I("start YOLO");
      IplImage frame = cpp_frame;
      image yolo_frame = ipl_to_image(&frame);
      // noscope_rgbgr_image(yolo_frame);
      yolo_confidence_[i] = yolo_->LabelFrame(yolo_frame);
      //std::cout << yolo_confidence_[i] << std::endl;
      free_image(yolo_frame);
      /*
      if (yolo_confidence_[i] != 0){
        std::cout << "1" << std::endl;
        //std::cerr << "frame " << i << ": " << yolo_confidence_[i] << "\n";
      }
      else{
        std::cout << "0" << std::endl;
      }
      */
      labels_[i] = yolo_confidence_[i] > 0;
      frame_status_[i] = kYoloLabeled;
      labeled_by_yolo2 = labeled_by_yolo2 +1;
      I("end YOLO");
    }
  } else {
    for (size_t i = 0; i < kNbFrames_; i++){
      if (frame_status_[i] == kDistillUnfiltered){
        frame_status_[i] = kYoloLabeled;
        labeled_by_yolo1 = labeled_by_yolo1 +1;
      }
    }
  }
  k2_measure("EndYOLO");
  k2_measure_flush();

  std::cout << "labeled by yolo1: " << labeled_by_yolo1 << " labeled by yolo2: " << labeled_by_yolo2 << "\n";  
}

void NoscopeLabeler::DumpConfidences(const std::string& fname,
                                  const std::string& model_name,
                                  const size_t kSkip,
                                  const bool kSkipSmallCNN,
                                  const float diff_thresh,
                                  const float distill_thresh_lower,
                                  const float distill_thresh_upper,
                                  const std::vector<double>& runtimes) {
  std::ofstream csv_file;
  csv_file.open(fname);

  std::stringstream rt;
  std::copy(runtimes.begin(), runtimes.end(), std::ostream_iterator<double>(rt, " "));

  csv_file << "# diff_thresh: "  << diff_thresh <<
      ", distill_thresh_lower: " << distill_thresh_lower <<
      ", distill_thresh_upper: " << distill_thresh_upper <<
      ", skip: " << kSkip <<
      ", skip_cnn: " << kSkipSmallCNN <<
      ", runtime: " << rt.str() << "\n";
  csv_file << "# model: " << model_name <<
      ", diff_detection: " << kDifferenceFilter_.name << "\n";

  csv_file << "# frame,status,diff_confidence,cnn_confidence,yolo_confidence,label\n";
  for (size_t i = 0; i < kNbFrames_; i++) {
    csv_file << (kSkip*i + 1) << ",";
    csv_file << frame_status_[i] << ",";
    csv_file << diff_confidence_[i] << ",";
    csv_file << cnn_confidence_[i] << ",";
    csv_file << yolo_confidence_[i] << ",";
    csv_file << labels_[i] << "\n";

    // repeat the previous label for skipped frames
    for(size_t j = 0; j < kSkip-1; j++){
      csv_file << (kSkip*i + j + 1) << ",";
      csv_file << kSkipped << ",";
      csv_file << 0 << ",";
      csv_file << 0 << ",";
      csv_file << 0 << ",";
      csv_file << labels_[i] << "\n";
    }
  }
}

} // namespace noscope
