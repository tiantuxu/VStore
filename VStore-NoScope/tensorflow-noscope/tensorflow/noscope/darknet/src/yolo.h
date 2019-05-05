#ifndef TENSORFLOW_VUSE_DARKNET_YOLO_H_
#define TENSORFLOW_VUSE_DARKNET_YOLO_H_

#include <vector>

#include "darknet.h"
#include "network.h"

namespace yolo {

using std::string;

class YOLO {
 public:
  /*
   * person = 0
   * car = 2
   * bus = 5
   */
  YOLO(string cfgfile, string weightfile, const int kTargetClass);

  float LabelFrame(image im);

 private:
  constexpr static float kThresh_ = 0.2;
  constexpr static float kNms_ = 0.4;
  constexpr static int kClasses_ = 80;  // Maybe fixme?
  const int kTargetClass_;

  network net_;
  layer l_;

  std::vector<box> boxes_;
  std::vector<float *> probs_;
};

}  // namespace yolo

#endif
