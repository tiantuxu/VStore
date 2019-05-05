#ifndef TENSORFLOW_VUSE_MEANSQUAREDERROR_H_
#define TENSORFLOW_VUSE_MEANSQUAREDERROR_H_

#include <sys/types.h>
#include <cstdint>

namespace noscope {

float MSE_AVX(const float *f1, const float *f2, const size_t N);

float MSE_AVX2(const uint8_t *f1, const uint8_t *f2, const size_t N);

} // namespace noscope

#endif  // TENSORFLOW_VUSE_MEANSQUAREDERROR_H_
