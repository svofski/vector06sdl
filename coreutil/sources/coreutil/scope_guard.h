#pragma once
#include <boost/preprocessor/cat.hpp>
#include <utility>

namespace coreutil {

#define scope(kw) COREUTIL_SCOPE_GUARD_##kw()

template <class F> class scope_guard {
  F f;
 public:
  inline scope_guard(scope_guard<F> &&o): f(std::move(o.f)) {}
  inline scope_guard(F f): f(std::move(f)) {}
  inline ~scope_guard() { f(); }
};

#define COREUTIL_SCOPE_GUARD_PP_UNIQUE(x) BOOST_PP_CAT(x, __LINE__)
#define COREUTIL_SCOPE_GUARD_exit()                                     \
  auto COREUTIL_SCOPE_GUARD_PP_UNIQUE(_scope_exit_local) =              \
    ::coreutil::scope_guard_detail::scope_guard_helper() << [&]

namespace scope_guard_detail {

struct scope_guard_helper {
  template <class F> inline scope_guard<F> operator<<(F &&f) {
    return scope_guard<F>(std::forward<F>(f));
  }
};

} // namespace scope_guard_detail

}  // namespace coreutil
