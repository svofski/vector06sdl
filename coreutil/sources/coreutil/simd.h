#pragma once

namespace coreutil {

#if (defined(__i386__) || defined(__x86_64__)) && defined(__AVX__)
static constexpr unsigned simd_register_size = 32;
#else
static constexpr unsigned simd_register_size = 16;
#endif

static constexpr unsigned simd_alignment = simd_register_size;

template <class T>
static constexpr unsigned simd_default_size = simd_register_size / sizeof(T);

template <class T>
struct simd_traits;

template <class T, unsigned N = simd_default_size<T>>
struct simd_class;

template <class T, unsigned N = simd_default_size<T>>
using simd_t = typename simd_class<T, N>::type;

template <class V> using simd_element_type = typename simd_traits<V>::element_type;
template <class V> static constexpr unsigned simd_size = simd_traits<V>::size;

template <class V> constexpr V simd_pack(simd_element_type<V> x);

template <class V> V simd_abs(V x);

template <class V> simd_element_type<V> simd_sum(V x);
template <class V> simd_element_type<V> simd_product(V x);

template <class V> V simd_mul_add(V a, V b, V c);
template <class V> V simd_mul_neg_add(V a, V b, V c);
template <class V> V simd_mul_hi(V a, V b);

template <class V> V simd_loadu(const V *p);
template <class V> void simd_storeu(V *p, V x);

}  // namespace coreutil

#include "simd.tcc"

using namespace coreutil::simd_ios;
