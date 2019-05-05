#include <stdlib.h>
#include <x86intrin.h>

#include "tensorflow/noscope/mse.h"

namespace noscope {

// http://stackoverflow.com/questions/23189488/horizontal-sum-of-32-bit-floats-in-256-bit-avx-vector
static inline float _mm256_reduce_add_ps(__m256 x) {
    /* ( x3+x7, x2+x6, x1+x5, x0+x4 ) */
    const __m128 x128 = _mm_add_ps(_mm256_extractf128_ps(x, 1), _mm256_castps256_ps128(x));
    /* ( -, -, x1+x3+x5+x7, x0+x2+x4+x6 ) */
    const __m128 x64 = _mm_add_ps(x128, _mm_movehl_ps(x128, x128));
    /* ( -, -, -, x0+x1+x2+x3+x4+x5+x6+x7 ) */
    const __m128 x32 = _mm_add_ss(x64, _mm_shuffle_ps(x64, x64, 0x55));
    /* Conversion to float is a no-op on x86-64 */
    return _mm_cvtss_f32(x32);
}

float MSE_AVX(const float *f1, const float *f2, const size_t N) {
  __m256 acc =  _mm256_set1_ps(0);
  for (size_t i = 0; i < (N & (~7)); i += 8) {
    __m256 tmp = _mm256_sub_ps(_mm256_loadu_ps(f1 + i), _mm256_loadu_ps(f2 + i));
    tmp = _mm256_mul_ps(tmp, tmp);
    acc = _mm256_add_ps(acc, tmp);
  }
  float mse = _mm256_reduce_add_ps(acc);
  for (size_t i = (N / 8) * 8; i < N; i++) {
    const float diff = f1[i] - f2[i];
    mse += diff * diff;
  }
  return mse / N;
}

float MSE_C(const float *f1, const float *f2, const size_t N) {
  float acc = 0;
  for (size_t i = 0; i < N; i++) {
    const float diff = f1[i] - f2[i];
    acc += diff * diff;
  }
  return acc / N;
}

float MSE_AVX2(const uint8_t *f1, const uint8_t *f2, const size_t N) {
  int sum = 0;
  for (size_t i = 0; i < N; i++) {
    const int tmp = f1[i] - (int) f2[i];
    sum += tmp * tmp;
  }
  return sum / (float) N;
}

} // namespace noscope
