#ifndef TENSORFLOW_VUSE_FILTERS_H_
#define TENSORFLOW_VUSE_FILTERS_H_

#include <stdlib.h>
#include <x86intrin.h>

#include <string>

#include "opencv2/opencv.hpp"


// ok C++
namespace noscope {
namespace filters {

typedef float (*DifferenceFilterFP)
    (const uint8_t *, const uint8_t *);

typedef struct DifferenceFilter {
  const DifferenceFilterFP fp;
  const std::string name;
} DifferenceFilter;

const size_t kResol = 100;
const size_t kNbChannels = 3;
const size_t kNbBlocks = 10;
const size_t kBlockSize = 10;

void LoadWeights(const std::string& fname);

float DoNothing(const uint8_t *f1, const uint8_t *f2);

float BlockedMSE(const uint8_t *f1, const uint8_t *f2);

float GlobalMSE(const uint8_t *f1, const uint8_t *f2);

} // namespace filters
} // namespace noscope

#endif  // TENSORFLOW_VUSE_FILTERS_H_
