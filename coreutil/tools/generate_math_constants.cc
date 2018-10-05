#include <boost/multiprecision/gmp.hpp>
#include <boost/format.hpp>
#include <iostream>
namespace mp = boost::multiprecision;

typedef mp::mpf_float_1000 MPFloat;

struct MPConstant {
  const char *name {};
  const char *expr {};
  MPFloat value;
};

int main() {
  const MPFloat one(1);
  const MPFloat two(2);
  const MPFloat ten(10);
  const MPFloat e = mp::exp(one);
  const MPFloat ln2 = mp::log(two);
  const MPFloat ln10 = mp::log(ten);
  const MPFloat pi = 4 * mp::atan(one);
  const MPFloat sqrtpi = mp::sqrt(pi);
  const MPFloat sqrt2 = mp::sqrt(two);

  const MPConstant constants[] = {
    {"E", "e", e},
    {"LOG2E", "log2(e)", mp::log2(e)},
    {"LOG10E", "log10(e)", mp::log10(e)},
    {"LN2", "log(2)", mp::log(two)},
    {"LN10", "log(10)", mp::log(ten)},
    {"PI", "pi", pi},
    {"2PI", "2*pi", 2 * pi},
    {"4PI", "4*pi", 4 * pi},
    {"PI_2", "pi/2", pi / 2},
    {"PI_4", "pi/4", pi / 4},
    {"1_PI", "1/pi", 1 / pi},
    {"2_PI", "2/pi", 2 / pi},
    {"4_PI", "4/pi", 4 / pi},
    {"1_SQRTPI", "1/sqrt(pi)", 1 / sqrtpi},
    {"2_SQRTPI", "2/sqrt(pi)", 2 / sqrtpi},
    {"SQRT2", "sqrt(2)", sqrt2},
    {"1_SQRT2", "1/sqrt(2)", 1 / sqrt2},
  };

  for (const MPConstant &c: constants) {
    std::cout << "#define " <<
        boost::format("K_%s%|15t|::coreutil::math_constant(%s)%|85t|// %s\n")
        % c.name % (c.value.str(36) + 'L') % c.expr;
  }

  return 0;
}
