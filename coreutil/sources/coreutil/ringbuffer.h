#pragma once
#include <memory>
#include <cstdint>

namespace coreutil {

class ringbuffer final {
 public:
  // initialization and cleanup
  explicit ringbuffer(uint32_t capacity);
  ~ringbuffer();

  // read operations
  uint32_t used_byte_count() const;
  bool get(void *data, uint32_t len);
  bool peek(void *data, uint32_t len) const;
  bool discard(uint32_t len);

  // write operations
  uint32_t free_byte_count() const;
  bool put(const void *data, uint32_t len);

 private:
  const uint32_t cap_;
  volatile uint32_t rp_ = 0, wp_ = 0;
  std::unique_ptr<char[]> rbdata_ {};
  bool read_impl_(void *data, uint32_t len, bool advp);
};

}  // namespace coreutil
