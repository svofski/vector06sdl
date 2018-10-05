#pragma once
#include <string>
#include <iosfwd>

namespace coreutil {

template <typename A0, typename... A>
std::ostream &concat_to_stream(std::ostream &o, const A0 &a, const A &... rest);

template <typename... A>
std::string concat(const A &... args);

}  // namespace coreutil

#include "concat.tcc"
