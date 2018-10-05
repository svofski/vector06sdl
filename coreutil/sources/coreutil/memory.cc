#include "memory.h"
#include <stdexcept>
#include <cassert>

namespace coreutil {

//------------------------------------------------------------------------------
// pointer bump allocator
//------------------------------------------------------------------------------

struct pbheader {
  char *ptr {};
  size_t capacity {};
  alignas(standard_alignment) char data[];
};

void pbinit(void *mem, size_t memsize) {
  if (memsize < sizeof(pbheader))
    throw std::bad_alloc();
  pbheader *hdr = reinterpret_cast<pbheader *>(mem);
  hdr->ptr = hdr->data;
  hdr->capacity = memsize - sizeof(pbheader);
}

void *pbmalloc(void *mem, size_t size) noexcept {
  pbheader *hdr = reinterpret_cast<pbheader *>(mem);

  // overallocate to preserve alignment
  const unsigned mask = standard_alignment - 1;
  size = (size + mask) & ~mask;
  assert(!(((uintptr_t)hdr->ptr) & mask) && "memory alignment anomaly");

  char *p = hdr->ptr;
  bool oom = (size > hdr->capacity) ||
             (p > hdr->data + (hdr->capacity - size));
  if (oom)
    return nullptr;

  hdr->ptr += size;
  return p;
}

void pbclear(void *mem) noexcept {
  pbheader *hdr = reinterpret_cast<pbheader *>(mem);
  hdr->ptr = hdr->data;
}

}  // namespace coreutil
