#include <stdlib.h>
#include <x86intrin.h>

#include <iostream>
#include <iterator>

#include "opencv2/opencv.hpp"

#include "tensorflow/noscope/filters.h"
#include <iostream>
#include <fstream>
using namespace std;
// ok C++
namespace noscope {
namespace filters {

// FIXME
static float weights[kNbChannels][kNbBlocks][kNbBlocks];

void LoadWeights(const std::string& fname) {

  std::ifstream is(fname);
  std::istream_iterator<float> start(is), end;
  std::vector<float> loaded_weights(start, end);

  if (loaded_weights.size() != kNbBlocks * kNbBlocks * kNbChannels) {
    std::cerr << "Loaded " << loaded_weights.size() << " weights\n";
    std::cerr << "Expected " << kNbBlocks * kNbBlocks * kNbChannels << "\n";
    throw std::runtime_error("Failed to load DD weights");
  }

  size_t ind = 0;
  for (size_t j = 0; j < kNbBlocks; j++) {
    for (size_t k = 0; k < kNbBlocks; k++) {
      for (size_t i = 0; i < kNbChannels; i++) {
        weights[i][j][k] = loaded_weights[ind++] / (255 * 255);
      }
    }
  }
}

float DoNothing(const uint8_t *f1, const uint8_t *f2) {

  return -100;
}

float BlockedMSE(const uint8_t *f1, const uint8_t *f2) {

  cv::Mat frame1(cv::Size(kResol, kResol), CV_8UC3, const_cast<uint8_t *>(f1));
  cv::Mat frame2(cv::Size(kResol, kResol), CV_8UC3, const_cast<uint8_t *>(f2));
  cv::Mat diff(cv::Size(kResol, kResol), CV_8UC3);
  // If saturation does what I think, it should be fine
  cv::absdiff(frame1, frame2, diff);
  diff.convertTo(diff, CV_32FC3);
  diff = diff.mul(diff);

  std::vector<cv::Mat> channels;
  cv::split(diff, channels);

  float ret = 0;
  for (size_t i = 0; i < kNbChannels; i++) {
    for (size_t j = 0; j < kNbBlocks; j++) {
      for (size_t k = 0; k < kNbBlocks; k++) {
        cv::Rect block(cv::Point(j * kBlockSize, k * kBlockSize),
                       cv::Point((j + 1) * kBlockSize, (k + 1) * kBlockSize));
        cv::Mat tmp = channels[i](block);
        ret += cv::sum(tmp)[0] * weights[i][j][k];
      }
    }
  }
  return ret;
}

float GlobalMSE(const uint8_t *f1, const uint8_t *f2) {
  
  const size_t N = kResol * kResol * kNbChannels;
  int acc = 0;
  for (size_t i = 0; i < N; i++) {
    const int diff = f1[i] - (int) f2[i];
    acc += diff * diff;
  }
  return acc / (float) N;
}

} // namespace filters
} // namespace noscope
