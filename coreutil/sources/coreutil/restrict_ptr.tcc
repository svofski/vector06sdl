#pragma once
#include "restrict_ptr.h"

namespace coreutil {

template <class T> struct is_restrict<T *__restrict> {
  static constexpr bool value = true;
};
template <class T> struct is_restrict {
  static constexpr bool value = false;
};

template <class T> struct make_restrict<T *> {
  typedef T *__restrict type;
};

}  // namespace coreutil
