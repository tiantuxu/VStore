#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <iostream>

#include "network.h"
#include "detection_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "demo.h"
#include "option_list.h"
#include "region_layer.h"

#include "tensorflow/noscope/darknet/src/yolo.h"

namespace yolo {

using std::string;

YOLO::YOLO(string cfgfile, string weightfile, const int kTargetClass) :
    kTargetClass_(kTargetClass),
    net_(parse_network_cfg(const_cast<char *>(cfgfile.c_str()))),
    l_(net_.layers[net_.n - 1]),
    boxes_(l_.w * l_.h * l_.n),
    probs_(l_.w * l_.h * l_.n) {
  load_weights(&net_, const_cast<char *>(weightfile.c_str()));

  // FIXME: more batches?
  set_batch_network(&net_, 1);
  layer l = net_.layers[net_.n - 1];
  if (l.classes != kClasses_) {
    std::cerr << l.n << "\n";
    throw std::runtime_error("YOLO network has wrong # of classes");
  }
  srand(2222222);

  // FIXME: dealloc
  for (int j = 0; j < l.w * l.h * l.n; j++)
    probs_[j] = (float *) calloc(kClasses_, sizeof(float));
}

float YOLO::LabelFrame(image im) {
  /*image sized = resize_image(im, net_.w, net_.h);
  float *X = sized.data;*/
  float *X = im.data;
  layer l = net_.layers[net_.n - 1];

  network_predict(net_, X);
  if (l.type == DETECTION) {
    get_detection_boxes(l, 1, 1, kThresh_, &probs_[0], &boxes_[0], 0);
  } else if (l.type == REGION) {
    int *map = 0;
    get_region_boxes(l, im.w, im.h, net_.w, net_.h, kThresh_, &probs_[0], &boxes_[0], 0, 0, map, kThresh_, 0);
  } else {
    throw std::runtime_error("YOLO last layer must be detections");
  }
  if (kNms_ > 0) do_nms(&boxes_[0], &probs_[0], l.w*l.h*l.n, kClasses_, kNms_);

  float max_prob = 0;
  for (int j = 0; j < l.w * l.h * l.n; j++) {
    const float confidence = probs_[j][kTargetClass_];
    if (confidence > max_prob)
      max_prob = confidence;
  }
  return max_prob;
}

}  // namespace yolo
