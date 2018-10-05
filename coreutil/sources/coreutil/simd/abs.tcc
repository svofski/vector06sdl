#pragma once
#include "../simd.h"
#include <boost/integer.hpp>
#include <type_traits>

namespace coreutil {
namespace simd_detail {

template <class V, bool F = std::is_floating_point<simd_element_type<V>>()>
struct simd_abs_impl;

template <class V> inline V simd_abs_f(V x);
template <class V> inline V simd_abs_i(V x);

template <class V> struct simd_abs_impl<V, true> {
  inline V operator()(V x) { return simd_abs_f(x); }
};
template <class V> struct simd_abs_impl<V, false> {
  inline V operator()(V x) { return simd_abs_i(x); }
};

template <class V>
inline V simd_abs_f(V x) {
  typedef simd_element_type<V> real_type;
  constexpr unsigned bits = 8 * sizeof(real_type);

  typedef typename boost::uint_t<bits>::exact uint_type;
  typedef simd_t<uint_type, simd_size<V>> uint_vector;

  uint_vector mask = simd_pack<uint_vector>(uint_type(-1) >> 1);
  return V(uint_vector(x) & mask);
}

#if (defined(__i386__) || defined(__x86_64__)) && defined(__SSSE3__)
template <>
inline v4si simd_abs_i(v4si x) {
  return v4si(_mm_abs_epi32(__m128i(x)));
}
template <>
inline v8ss simd_abs_i(v8ss x) {
  return v8ss(_mm_abs_epi16(__m128i(x)));
}
template <>
inline v16sb simd_abs_i(v16sb x) {
  return v16sb(_mm_abs_epi8(__m128i(x)));
}
#endif

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX__)
template <>
inline v8si simd_abs_i(v8si x) {
  return v8si(_mm256_abs_epi32(__m256i(x)));
}
template <>
inline v16ss simd_abs_i(v16ss x) {
  return v16ss(_mm256_abs_epi16(__m256i(x)));
}
template <>
inline v32sb simd_abs_i(v32sb x) {
  return v32sb(_mm256_abs_epi8(__m256i(x)));
}
#endif

template <class V>
inline V simd_abs_i(V x) {
  V neg = x < 0;
  return (neg & -x) | (~neg & x);
}

}  // namespace simd_detail

template <class V>
inline V simd_abs(V x) {
  return simd_detail::simd_abs_impl<V>()(x);
}

}  // namespace coreutil
