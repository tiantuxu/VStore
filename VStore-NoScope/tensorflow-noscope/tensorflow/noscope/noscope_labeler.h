#ifndef TENSORFLOW_VUSE_VUSELABELER_H_
#define TENSORFLOW_VUSE_VUSELABELER_H_

#include "opencv2/opencv.hpp"

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"

#include "tensorflow/noscope/filters.h"
#include "tensorflow/noscope/noscope_data.h"
#include "tensorflow/noscope/darknet/src/yolo.h"


namespace noscope {

class NoscopeLabeler {
  friend class FrameData; // access the internals

 public:
  // tensorflow doesn't support unique_ptr
  NoscopeLabeler(tensorflow::Session *session,
              yolo::YOLO *yolo_classifier,
              noscope::filters::DifferenceFilter diff_filt,
              const std::string& avg_fname,
              const noscope::NoscopeData& data);

  // This currently ignores the upper threshold
  void RunDifferenceFilter(const float lower_thresh, const float upper_thresh,
                           const bool const_ref, const size_t kRef);

  void PopulateCNNFrames();

  void RunSmallCNN(const float lower_thresh, const float upper_thresh);

  void RunYOLO(const bool actually_run);

  void DumpConfidences(const std::string& fname,
                       const std::string& model_name,
                       const size_t kSkip,
                       const bool kSkipSmallCNN,
                       const float diff_thresh,
                       const float distill_thresh_lower,
                       const float distill_thresh_upper,
                       const std::vector<double>& runtime);

 private:
  constexpr static size_t kNumThreads_ = 1;
  constexpr static size_t kMaxCNNImages_ = 512;
  constexpr static size_t kNbChannels_ = 3;
  constexpr static size_t kDiffDelay_ = 1;

  enum Status {
    kUnprocessed,
    kSkipped,
    kDiffFiltered,
    kDiffUnfiltered,
    kDistillFiltered,
    kDistillUnfiltered,
    kYoloLabeled
  };

  const size_t kNbFrames_;

  const noscope::NoscopeData& all_data_;

  std::vector<Status> frame_status_;
  std::vector<bool> labels_;

  const noscope::filters::DifferenceFilter kDifferenceFilter_;
  std::vector<float> diff_confidence_;

  std::vector<int> cnn_frame_ind_;
  std::vector<float> cnn_confidence_;

  yolo::YOLO *yolo_;
  std::vector<int> yolo_frame_ind_;
  std::vector<float> yolo_confidence_;

  cv::Mat avg_;

  tensorflow::Session *session_;

  std::vector<tensorflow::Tensor> dist_tensors_;
};

} // namespace noscope

#endif  // TENSORFLOW_VUSE_VUSELABELER_H_
