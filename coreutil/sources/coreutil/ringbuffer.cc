#include "ringbuffer.h"
#include <algorithm>

namespace coreutil {

// initialization and cleanup

ringbuffer::ringbuffer(uint32_t capacity)
    : cap_(capacity) {
  rbdata_.reset(new char[capacity]);
}

ringbuffer::~ringbuffer() {
}

// read operations

uint32_t ringbuffer::used_byte_count() const {
  const uint32_t rp = rp_, wp = wp_, cap = cap_;
  return wp + ((wp < rp) ? cap : 0) - rp;
}

inline bool ringbuffer::read_impl_(void *data, uint32_t len, bool advp) {
  if (used_byte_count() < len)
    return false;
  const uint32_t rp = rp_, cap = cap_;
  char *dst = reinterpret_cast<char *>(data);
  if (data) {
    const unsigned taillen = std::min(len, cap - rp);
    std::copy_n(&rbdata_[rp], taillen, dst);
    std::copy_n(&rbdata_[0], len - taillen, dst + taillen);
  }
  if (advp)
    rp_ = (rp + len < cap) ? (rp + len) : (rp + len - cap);
  return true;
}

bool ringbuffer::get(void *data, uint32_t len) {
  return read_impl_(data, len, true);
}

bool ringbuffer::peek(void *data, uint32_t len) const {
  return const_cast<ringbuffer *>(this)->read_impl_(data, len, false);
}

bool ringbuffer::discard(uint32_t len) {
  return read_impl_(nullptr, len, true);
}

// write operations

uint32_t ringbuffer::free_byte_count() const {
  const uint32_t rp = rp_, wp = wp_, cap = cap_;
  return rp + ((rp <= wp) ? cap : 0) - wp - 1;
}

bool ringbuffer::put(const void *data, uint32_t len) {
  if (free_byte_count() < len)
    return false;
  const uint32_t wp = wp_, cap = cap_;
  const char *src = reinterpret_cast<const char *>(data);
  const unsigned taillen = std::min(len, cap - wp);
  std::copy_n(src, taillen, &rbdata_[wp]);
  std::copy_n(src + taillen, len - taillen, &rbdata_[0]);
  wp_ = (wp + len < cap) ? (wp + len) : (wp + len - cap);
  return true;
}

}  // namespace coreutil
