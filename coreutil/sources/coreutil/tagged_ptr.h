#pragma once
#include <utility>

namespace coreutil {

template <class T>
class tagged_ptr {
 public:
  typedef T *pointer;
  typedef T element_type;

  typedef int tag_type;
  static constexpr unsigned tag_bits = 2;
  static constexpr tag_type tag_min = 0, tag_max = (1 << tag_bits) - 1;

  tagged_ptr() noexcept;
  tagged_ptr(std::nullptr_t) noexcept;
  tagged_ptr(const tagged_ptr &other) noexcept;
  template <class U> tagged_ptr(const tagged_ptr<U> &other) noexcept;
  tagged_ptr(pointer ptr) noexcept;

  tagged_ptr &operator=(std::nullptr_t) noexcept;
  tagged_ptr &operator=(const tagged_ptr &other) noexcept;
  template <class U> tagged_ptr &operator=(const tagged_ptr<U> &other) noexcept;
  tagged_ptr &operator=(pointer ptr) noexcept;

  void reset(pointer ptr = pointer()) noexcept;
  pointer get() const noexcept;

  tag_type tag() const noexcept;
  void tag(tag_type t);

  operator pointer() const noexcept;
  explicit operator bool() const noexcept;

  element_type &operator*() const noexcept;
  pointer operator->() const noexcept;

 private:
  pointer ptr_ = nullptr;
};

}  // namespace coreutil

#include "tagged_ptr.tcc"
