#pragma once

namespace coreutil {

template <class T> using restrict_ptr = T *__restrict;

template <class T> struct is_restrict;
template <class T> static constexpr bool is_restrict_v = is_restrict<T>::value;

template <class T> struct make_restrict;
template <class T> using make_restrict_t = typename make_restrict<T>::type;

}  // namespace coreutil

#include "restrict_ptr.tcc"
