#pragma once
#include "memory.h"
#include <stdexcept>
#if defined(_WIN32)
# include <malloc.h>
#else
# include <cstdlib>
#endif
#include <new>

namespace coreutil {

#if defined(_WIN32)

inline void *almalloc(size_t al, size_t size) noexcept {
  return ::_aligned_malloc(size, al);
}

inline void alfree(void *p) {
  ::_aligned_free(p);
}

#else

inline void *almalloc(size_t al, size_t size) noexcept {
  void *p;
  int ret = ::posix_memalign(&p, al, size);
  if (ret != 0)
    return nullptr;
  return p;
}

inline void alfree(void *p) {
  ::free(p);
}

#endif

template <class T> struct aligned_delete {
  static_assert(std::is_trivial<T>(),
                "aligned_delete only accepts trivial types");
  void operator()(void *p) { alfree(p); }
};

struct aligned {
  explicit aligned(size_t al) : alignment(al) {}
  const size_t alignment {};
};

}  // namespace coreutil

inline void *operator new(size_t size, coreutil::aligned al) {
  void *p = ::coreutil::almalloc(al.alignment, size);
  if (!p)
    throw std::bad_alloc();
  return p;
}

inline void *operator new[](size_t size, coreutil::aligned al) {
  void *p = ::coreutil::almalloc(al.alignment, size);
  if (!p)
    throw std::bad_alloc();
  return p;
}
