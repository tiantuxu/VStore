#ifndef TENSORFLOW_VUSE_VUSEDATA_H_
#define TENSORFLOW_VUSE_VUSEDATA_H_

#include "opencv2/opencv.hpp"
#include "opencv2/videoio.hpp"
#include "tensorflow/noscope/RxManager.h"
#include "tensorflow/noscope/config.h"

//extern vs::Frame frame[3][3600];
//extern size_t reso;

namespace noscope {
extern vs::Frame frame[3][3600];
//static const int r = resolution[reso];

class NoscopeData {
 public:

    static const cv::Size kYOLOResol_;
    static const cv::Size kDiffResol_;
    static const cv::Size kDistResol_;
    static const cv::Size kSCNNResol_;

  constexpr static size_t kNbChannels_ = 3;
  constexpr static size_t kYOLOFrameSize_ = 60 * 60 * kNbChannels_;
  constexpr static size_t kDiffFrameSize_ = 60 * 60 * kNbChannels_;
  constexpr static size_t kDistFrameSize_ = 50 * 50 * kNbChannels_;
  constexpr static size_t kSCNNFrameSize_ = 600 * 600 * kNbChannels_;


    //constexpr static size_t kNbChannels_ = 3;
    //const static size_t kYOLOFrameSize_;
    //const static size_t kDiffFrameSize_;
    //const static size_t kDistFrameSize_;

  const size_t kNbFrames_;
  const size_t kSkip_;

  std::vector<uint8_t> yolo_data_;
  std::vector<uint8_t> diff_data_;
  std::vector<float> dist_data_;

  //NoscopeData(const std::string& fname, const size_t kSkip, const size_t kNbFrames, const size_t kStart);
  NoscopeData(vs::Frame frame[][3600], const size_t kSkip, const size_t kNbFrames, const size_t kStart);

  //NoscopeData(const std::string& fname);

  //void DumpAll(const std::string& fname);
};

} // namespace noscope

#endif