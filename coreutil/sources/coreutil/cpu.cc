#include "cpu.h"

namespace coreutil {

#if defined(__arm__)
static inline int get_fpscr() {
  int x;
  asm volatile("vmrs %[x], FPSCR" : [x] "=r" (x));
  return x;
}

static inline void set_fpscr(int x) {
  asm volatile("vmsr FPSCR, %[x]" : : [x] "r" (x));
}
#endif

#if defined(__aarch64__)
static inline int get_fpscr() {
  int x;
  asm volatile("mrs %[x], FPCR" : [x] "=r" (x));
  return x;
}

static inline void set_fpscr(int x) {
  asm volatile("msr FPCR, %[x]" : : [x] "r" (x));
}
#endif

#if defined(__i386__) || defined(__x86_64__)
static inline int get_csr() {
  int x;
  asm volatile("stmxcsr %0" : "=m" (x));
  return x;
}

static inline void set_csr(int x) {
  asm volatile("ldmxcsr %0" : : "m" (x));
}
#endif

void enable_denormals() {
#if defined(__arm__) || defined(__aarch64__)
  set_fpscr(get_fpscr() & ~(1 << 24));
#elif defined(__i386__) || defined(__x86_64__)
  set_csr(get_csr() & ~0x8040);
#else
#error not implemented: enable_denormals
#endif
}

void disable_denormals() {
#if defined(__arm__) || defined(__aarch64__)
  set_fpscr(get_fpscr() | (1 << 24));
#elif defined(__i386__) || defined(__x86_64__)
  set_csr(get_csr() | 0x8040);
#else
#error not implemented: disable_denormals
#endif
}

}  // namespace coreutil
