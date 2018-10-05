#pragma once
#include "simd.h"
#include <iosfwd>
#include <cstdint>

namespace coreutil {

namespace simd_detail { template <class T, unsigned N> struct pack_impl; }
///

#define SIMD_FOREACH_VECTOR_SIZE(F, ...)         \
  F(2, ##__VA_ARGS__) F(4,  ##__VA_ARGS__)       \
  F(8, ##__VA_ARGS__) F(16, ##__VA_ARGS__)       \
  F(32, ##__VA_ARGS__)
#define SIMD_FOREACH_TYPE_PAIR(F, ...)                         \
  F(float,   sf, ##__VA_ARGS__) F(double,   sd, ##__VA_ARGS__) \
  F(int8_t,  sb, ##__VA_ARGS__) F(uint8_t,  ub, ##__VA_ARGS__) \
  F(int16_t, ss, ##__VA_ARGS__) F(uint16_t, us, ##__VA_ARGS__) \
  F(int32_t, si, ##__VA_ARGS__) F(uint32_t, ui, ##__VA_ARGS__) \
  F(int64_t, sl, ##__VA_ARGS__) F(uint64_t, ul, ##__VA_ARGS__)

#define SIMD_DEFINE(typ, sfx, siz)                                      \
  typedef typ v##siz##sfx [[gnu::vector_size(siz * sizeof(typ))]];      \
  template <> struct simd_traits<v##siz##sfx> {                         \
    typedef v##siz##sfx type;                                           \
    typedef typ element_type;                                           \
    static constexpr unsigned size = siz;                               \
  };                                                                    \
  template <> struct simd_class<typ, siz> {                             \
    typedef v##siz##sfx type;                                           \
    typedef typ element_type;                                           \
    static constexpr unsigned size = siz;                               \
  };                                                                    \
  namespace simd_ios {                                                  \
    template <class Ch> std::basic_ostream<Ch> &operator<<(             \
        std::basic_ostream<Ch> &o, v##siz##sfx v) {                     \
      o << '{' << v[0];                                                 \
      for (unsigned i = 1; i < siz; ++i)                                \
        o << ',' << ' ' << v[i];                                        \
      return o << '}';                                                  \
    }                                                                   \
  }
#define SIMD_DEFINE_FOR_SIZE(size)              \
  SIMD_FOREACH_TYPE_PAIR(SIMD_DEFINE, size)

SIMD_FOREACH_VECTOR_SIZE(SIMD_DEFINE_FOR_SIZE);

template <class V>
inline constexpr V simd_pack(simd_element_type<V> x) {
  simd_detail::pack_impl<simd_element_type<V>, simd_size<V>> impl;
  return impl(x);
}

template <class V>
inline simd_element_type<V> simd_sum(V x) {
  simd_element_type<V> r = x[0];
  for (unsigned i = 1; i < simd_size<V>; ++i)
    r += x[i];
  return r;
}

template <class V>
inline simd_element_type<V> simd_product(V x) {
  simd_element_type<V> r = x[0];
  for (unsigned i = 1; i < simd_size<V>; ++i)
    r *= x[i];
  return r;
}

}  // namespace coreutil

#include "simd/pack.tcc"
#include "simd/load.tcc"
#include "simd/store.tcc"
#include "simd/abs.tcc"
#include "simd/mul_add.tcc"
#include "simd/mul_hi.tcc"

#undef SIMD_FOREACH_VECTOR_SIZE
#undef SIMD_FOREACH_TYPE_PAIR
#undef SIMD_DEFINE
#undef SIMD_DEFINE_FOR_SIZE
