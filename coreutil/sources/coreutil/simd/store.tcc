#pragma once
#include "../simd.h"
#if defined(__i386__) || defined(__x86_64__)
# include <x86intrin.h>
#endif

namespace coreutil {

#if (defined(__i386__) || defined(__x86_64__)) && defined(__SSE__)
template <>
inline void simd_storeu(v4sf *p, v4sf x) {
  _mm_storeu_ps(reinterpret_cast<float *>(p), x);
}
template <>
inline void simd_storeu(v2sd *p, v2sd x) {
  _mm_storeu_pd(reinterpret_cast<double *>(p), x);
}
#endif

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX__)
template <>
inline void simd_storeu(v8sf *p, v8sf x) {
  _mm256_storeu_ps(reinterpret_cast<float *>(p), x);
}
template <>
inline void simd_storeu(v4sd *p, v4sd x) {
  _mm256_storeu_pd(reinterpret_cast<double *>(p), x);
}
#endif

#if (defined(__arm__) || defined(__aarch64__)) && defined(__ARM_NEON)
template <>
inline void simd_storeu(v4sf *p, v4sf x) {
  *p = x;
}
template <>
inline void simd_storeu(v2sd *p, v2sd x) {
  *p = x;
}
#endif

}  // namespace coreutil
