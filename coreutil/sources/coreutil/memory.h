#pragma once
#include <cstddef>
#include <cstdint>

namespace coreutil {

// minimum alignment for memory allocators on most machines
static constexpr unsigned standard_alignment = 2 * sizeof(void *);

//------------------------------------------------------------------------------
// pointer bump allocator
//------------------------------------------------------------------------------
// note: not thread-safe
// note: the memory buffer must be preallocated and aligned, eg.
//   alignas(coreutil::standard_alignment) char my_memory[my_size];

void pbinit(void *mem, size_t memsize);
[[gnu::malloc]] void *pbmalloc(void *mem, size_t size) noexcept;
void pbclear(void *mem) noexcept;

//------------------------------------------------------------------------------
// aligned allocator
//------------------------------------------------------------------------------
[[gnu::malloc]] void *almalloc(size_t al, size_t size) noexcept;
void alfree(void *p);

struct aligned;
template <class T> struct aligned_delete;

}  // namespace coreutil

void *operator new(size_t size, coreutil::aligned al);
void *operator new[](size_t size, coreutil::aligned al);

#include "memory.tcc"
