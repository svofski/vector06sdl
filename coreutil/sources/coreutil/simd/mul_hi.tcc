#pragma once
#include "../simd.h"
#if defined(__i386__) || defined(__x86_64__)
# include <x86intrin.h>
#endif
#if (defined(__arm__) || defined(__aarch64__)) && defined(__ARM_NEON)
# include <arm_neon.h>
#endif

namespace coreutil {

#if defined(__i386__) || defined(__x86_64__)

template <>
inline v8ss simd_mul_hi(v8ss a, v8ss b) {
  return v8ss(_mm_mulhi_epi16(__m128i(a), __m128i(b)));
}

template <>
inline v8us simd_mul_hi(v8us a, v8us b) {
  return v8us(_mm_mulhi_epu16(__m128i(a), __m128i(b)));
}

#endif

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX2__)

template <>
inline v16ss simd_mul_hi(v16ss a, v16ss b) {
  return v16ss(_mm256_mulhi_epi16(__m256i(a), __m256i(b)));
}

template <>
inline v16us simd_mul_hi(v16us a, v16us b) {
  return v16us(_mm256_mulhi_epu16(__m256i(a), __m256i(b)));
}

#endif

#if (defined(__arm__) || defined(__aarch64__)) && defined(__ARM_NEON)

// NOTE: ARM mul_hi is slightly wrong with input a=b=(negative minimum).
//  Probably not important in general but worth being known

template <>
inline v4ss simd_mul_hi(v4ss a, v4ss b) {
  return vshr_n_s16(vqdmulh_s16(a, b), 1);
}

template <>
inline v8ss simd_mul_hi(v8ss a, v8ss b) {
  return vshrq_n_s16(vqdmulhq_s16(a, b), 1);
}

#endif

}  // namespace coreutil
