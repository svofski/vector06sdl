#pragma once
#include "math.h"
#include <boost/config.hpp>
#include <type_traits>

namespace std { template <class T> class complex; }
///

namespace coreutil {

BOOST_FORCEINLINE constexpr math_constant::math_constant(long double value)
    : value(value) {}

template <class T>
BOOST_FORCEINLINE constexpr math_constant::operator T() const {
  return value;
}

namespace math_constants_detail {

template <class T> struct is_compatible_operand;
template <class T> struct is_compatible_operand<std::complex<T>>
    : public is_compatible_operand<T> {};
template <class T> struct is_compatible_operand
    : public std::is_floating_point<T> {};

}  // namespace math_constants_detail

#define COREUTIL_MATH_TYPE_CHECK(op, t)                                 \
  static_assert(math_constants_detail::is_compatible_operand<t>(),      \
                "cannot perform '" #op "' between a math constant and non-floating point type " #t)
#define COREUTIL_MATH_DEFINE_UNOP(op)                                   \
  BOOST_FORCEINLINE constexpr math_constant operator op(math_constant a) { return math_constant(op a.value); }
#define COREUTIL_MATH_DEFINE_BINOP(op)                                  \
  BOOST_FORCEINLINE constexpr math_constant operator op(math_constant a, math_constant b) { return math_constant(a.value op b.value); } \
  template <class T> BOOST_FORCEINLINE constexpr T operator op(math_constant a, T b) { COREUTIL_MATH_TYPE_CHECK(op, T); return T(a.value) op b; } \
  template <class T> BOOST_FORCEINLINE constexpr T operator op(T a, math_constant b) { COREUTIL_MATH_TYPE_CHECK(op, T); return a op T(b.value); }
#define COREUTIL_MATH_DEFINE_OPASSIGN(op)   \
  template <class T> BOOST_FORCEINLINE constexpr T &operator op(T &a, math_constant b) { COREUTIL_MATH_TYPE_CHECK(op, T); return a op T(b.value); }
#define COREUTIL_MATH_DEFINE_COMPARISON(op)                             \
  BOOST_FORCEINLINE constexpr bool operator op(math_constant a, math_constant b) { return a.value op b.value; } \
  template <class T> BOOST_FORCEINLINE constexpr bool operator op(math_constant a, T b) { COREUTIL_MATH_TYPE_CHECK(op, T); return T(a.value) op b; } \
  template <class T> BOOST_FORCEINLINE constexpr bool operator op(T a, math_constant b) { COREUTIL_MATH_TYPE_CHECK(op, T); return a op T(b.value); }

COREUTIL_MATH_DEFINE_UNOP(+);
COREUTIL_MATH_DEFINE_UNOP(-);
COREUTIL_MATH_DEFINE_BINOP(+);
COREUTIL_MATH_DEFINE_BINOP(-);
COREUTIL_MATH_DEFINE_BINOP(*);
COREUTIL_MATH_DEFINE_BINOP(/);
COREUTIL_MATH_DEFINE_OPASSIGN(+=);
COREUTIL_MATH_DEFINE_OPASSIGN(-=);
COREUTIL_MATH_DEFINE_OPASSIGN(*=);
COREUTIL_MATH_DEFINE_OPASSIGN(/=);
COREUTIL_MATH_DEFINE_COMPARISON(==);
COREUTIL_MATH_DEFINE_COMPARISON(!=);
COREUTIL_MATH_DEFINE_COMPARISON(<);
COREUTIL_MATH_DEFINE_COMPARISON(>);
COREUTIL_MATH_DEFINE_COMPARISON(<=);
COREUTIL_MATH_DEFINE_COMPARISON(>=);

#undef COREUTIL_MATH_TYPE_CHECK
#undef COREUTIL_MATH_DEFINE_UNOP
#undef COREUTIL_MATH_DEFINE_BINOP
#undef COREUTIL_MATH_DEFINE_OPASSIGN
#undef COREUTIL_MATH_DEFINE_COMPARISON

}  // namespace coreutil
