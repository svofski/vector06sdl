#pragma once

namespace coreutil {

//------------------------------------------------------------------------------
// math constants
//------------------------------------------------------------------------------

// This file provides math constants computed with 36 significant digits,
// which provides the maximum precision of IEEE quad floating-point numbers.

// The constants are defined more strictly than by usual C standards. The result
// type of a use expression is determined by its other operand, which must be a
// float. This enforces greater programming discipline wrt. implicit promotions.
//  eg. `2.f * K_PI` is float, `2. * K_PI` is double, `2 * K_PI` is invalid

#define K_E            ::coreutil::math_constant(2.7182818284590452353602874713526625L)      // e
#define K_LOG2E        ::coreutil::math_constant(1.44269504088896340735992468100189214L)     // log2(e)
#define K_LOG10E       ::coreutil::math_constant(0.434294481903251827651128918916605082L)    // log10(e)
#define K_LN2          ::coreutil::math_constant(0.693147180559945309417232121458176568L)    // log(2)
#define K_LN10         ::coreutil::math_constant(2.30258509299404568401799145468436421L)     // log(10)
#define K_PI           ::coreutil::math_constant(3.14159265358979323846264338327950288L)     // pi
#define K_2PI          ::coreutil::math_constant(6.28318530717958647692528676655900577L)     // 2*pi
#define K_4PI          ::coreutil::math_constant(12.5663706143591729538505735331180115L)     // 4*pi
#define K_PI_2         ::coreutil::math_constant(1.57079632679489661923132169163975144L)     // pi/2
#define K_PI_4         ::coreutil::math_constant(0.785398163397448309615660845819875721L)    // pi/4
#define K_1_PI         ::coreutil::math_constant(0.318309886183790671537767526745028724L)    // 1/pi
#define K_2_PI         ::coreutil::math_constant(0.636619772367581343075535053490057448L)    // 2/pi
#define K_4_PI         ::coreutil::math_constant(1.2732395447351626861510701069801149L)      // 4/pi
#define K_1_SQRTPI     ::coreutil::math_constant(0.564189583547756286948079451560772586L)    // 1/sqrt(pi)
#define K_2_SQRTPI     ::coreutil::math_constant(1.12837916709551257389615890312154517L)     // 2/sqrt(pi)
#define K_SQRT2        ::coreutil::math_constant(1.41421356237309504880168872420969808L)     // sqrt(2)
#define K_1_SQRT2      ::coreutil::math_constant(0.707106781186547524400844362104849039L)    // 1/sqrt(2)

struct math_constant {
  const long double value;
  explicit constexpr math_constant(long double value);
  template <class T> constexpr /* explicit */ operator T() const;
};

}  // namespace coreutil

#include "math.tcc"
