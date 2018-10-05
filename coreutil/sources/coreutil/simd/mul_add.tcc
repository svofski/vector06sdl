#pragma once
#include "../simd.h"
#if defined(__i386__) || defined(__x86_64__)
# include <x86intrin.h>
#endif

namespace coreutil {

#if (defined(__i386__) || defined(__x86_64__)) && defined(__FMA__)
template <>
inline v4sf simd_mul_add(v4sf a, v4sf b, v4sf c) {
  return _mm_fmadd_ps(a, b, c);
}
template <>
inline v2sd simd_mul_add(v2sd a, v2sd b, v2sd c) {
  return _mm_fmadd_pd(a, b, c);
}

template <>
inline v4sf simd_mul_neg_add(v4sf a, v4sf b, v4sf c) {
  return _mm_fnmadd_ps(a, b, c);
}
template <>
inline v2sd simd_mul_neg_add(v2sd a, v2sd b, v2sd c) {
  return _mm_fnmadd_pd(a, b, c);
}
#endif

template <class V>
inline V simd_mul_add(V a, V b, V c) {
  return a * b + c;
}

template <class V>
inline V simd_mul_neg_add(V a, V b, V c) {
  return -(a * b) + c;
}

}  // namespace coreutil
