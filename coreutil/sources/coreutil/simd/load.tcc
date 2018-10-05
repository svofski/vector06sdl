#pragma once
#include "../simd.h"
#if defined(__i386__) || defined(__x86_64__)
# include <x86intrin.h>
#endif

namespace coreutil {

#if (defined(__i386__) || defined(__x86_64__)) && defined(__SSE__)
template <>
inline v4sf simd_loadu(const v4sf *p) {
  return _mm_loadu_ps(reinterpret_cast<const float *>(p));
}
template <>
inline v2sd simd_loadu(const v2sd *p) {
  return _mm_loadu_pd(reinterpret_cast<const double *>(p));
}

#define SIMD_DEFINE_M128I_LOAD(bits)                              \
  template <>                                                     \
  inline simd_t<int##bits##_t, 128 / bits>                        \
  simd_loadu(const simd_t<int##bits##_t, 128 / bits> *p) {        \
    return simd_t<int##bits##_t, 128 / bits>(                     \
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(p)));   \
  }                                                               \
  template <>                                                     \
  inline simd_t<uint##bits##_t, 128 / bits>                       \
  simd_loadu(const simd_t<uint##bits##_t, 128 / bits> *p) {       \
    return simd_t<uint##bits##_t, 128 / bits>(                    \
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(p)));   \
  }

SIMD_DEFINE_M128I_LOAD(8);
SIMD_DEFINE_M128I_LOAD(16);
SIMD_DEFINE_M128I_LOAD(32);
SIMD_DEFINE_M128I_LOAD(64);

#undef SIMD_DEFINE_M128I_LOAD

#endif

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX__)
template <>
inline v8sf simd_loadu(const v8sf *p) {
  return _mm256_loadu_ps(reinterpret_cast<const float *>(p));
}
template <>
inline v4sd simd_loadu(const v4sd *p) {
  return _mm256_loadu_pd(reinterpret_cast<const double *>(p));
}
#endif

#if (defined(__arm__) || defined(__aarch64__)) && defined(__ARM_NEON)
template <>
inline v4sf simd_loadu(const v4sf *p) {
  return *p;
}
template <>
inline v2sd simd_loadu(const v2sd *p) {
  return *p;
}
#endif

}  // namespace coreutil
