#pragma once
#include "concat.h"
#include <iostream>
#include <sstream>

namespace coreutil {

inline std::ostream &concat_to_stream(std::ostream &o) {
  return o;
}

template <typename A0, typename... A>
inline std::ostream &concat_to_stream(std::ostream &o, const A0 &a, const A &... rest) {
  o << a;
  concat_to_stream(o, rest...);
  return o;
}

template <typename... A>
inline std::string concat(const A &... args) {
  std::ostringstream o;
  concat_to_stream(o, args...);
  return o.str();
}

}  // namespace coreutil
