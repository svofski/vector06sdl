#pragma once
#include <boost/preprocessor/repeat.hpp>
#include "../simd.h"

namespace coreutil {

namespace simd_detail {

#define SIMD_INITIALIZER_ITEM(z, n, d) x,
#define SIMD_IMPL_PACK(siz)                                    \
  template <class T>                                           \
  struct pack_impl<T, siz> {                                   \
    typedef simd_t<T, siz> V;                                  \
    inline constexpr V operator()(T x) {                       \
      return V{BOOST_PP_REPEAT(siz, SIMD_INITIALIZER_ITEM,)};  \
    }                                                          \
  };

SIMD_FOREACH_VECTOR_SIZE(SIMD_IMPL_PACK);

#undef SIMD_INITIALIZER_ITEM
#undef SIMD_IMPL_PACK

}  // namespace simd_detail

}  // namespace coreutil
