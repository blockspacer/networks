#pragma once

#include "assert.h"

#include <exception>
#include <iterator>

namespace core {

template <class T>
class ArrayRef {
 public:
  using iterator = T*;
  using const_iterator = const T*;
  using reference = T&;
  using const_reference = const T&;
  using value_type = T;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  constexpr inline ArrayRef() noexcept
      : data_(nullptr)
      , size_(0) {}

  constexpr inline ArrayRef(T* data, size_t len) noexcept
      : data_(data)
      , size_(len) {}

  constexpr inline ArrayRef(T* begin, T* end) noexcept
      : data_(begin)
      , size_(end - begin) {}

  constexpr inline ArrayRef(std::initializer_list<T> list) noexcept
      : data_(list.begin())
      , size_(list.size()) {}

  template <class Container>
  constexpr inline ArrayRef(Container&& container,
                            decltype(std::declval<T*&>() = container.data(), nullptr) = nullptr) noexcept
      : data_(container.data())
      , size_(container.size()) {}

  template <size_t N>
  constexpr inline ArrayRef(T (&array)[N]) noexcept
      : data_(array)
      , size_(N) {}

  template <class TT, typename = std::enable_if_t<std::is_same<std::remove_const_t<T>, std::remove_const_t<TT>>::value>>
  bool operator==(const ArrayRef<TT>& other) const noexcept {
    return (size_ == other.size()) && std::equal(begin(), end(), other.begin());
  }

  constexpr inline T* data() const noexcept { return data_; }

  [[nodiscard]] constexpr inline size_t size() const noexcept { return size_; }

  [[nodiscard]] constexpr size_t sizeBytes() const noexcept { return (size() * sizeof(T)); }

  [[nodiscard]] constexpr inline bool empty() const noexcept { return (size_ == 0); }

  [[nodiscard]] constexpr inline iterator begin() const noexcept { return data_; }

  [[nodiscard]] constexpr inline iterator end() const noexcept { return (data_ + size_); }

  [[nodiscard]] constexpr inline const_iterator cbegin() const noexcept { return data_; }

  [[nodiscard]] constexpr inline const_iterator cend() const noexcept { return (data_ + size_); }

  [[nodiscard]] constexpr inline reverse_iterator rbegin() const noexcept { return reverse_iterator(data_ + size_); }

  [[nodiscard]] constexpr inline reverse_iterator rend() const noexcept { return reverse_iterator(data_); }

  [[nodiscard]] constexpr inline const_reverse_iterator crbegin() const noexcept {
    return const_reverse_iterator(data_ + size_);
  }

  [[nodiscard]] constexpr inline const_reverse_iterator crend() const noexcept { return const_reverse_iterator(data_); }

  [[nodiscard]] constexpr inline reference front() const noexcept { return *data_; }

  inline reference back() const noexcept {
    core_assert(size_ > 0);
    return *(end() - 1);
  }

  inline reference operator[](size_t n) const noexcept {
    core_assert(n < size_);
    return *(data_ + n);
  }

  inline reference at(size_t n) const {
    if (n >= size_) {
      throw std::out_of_range("array ref range error");
    }

    return (*this)[n];
  }

  constexpr inline explicit operator bool() const noexcept { return (size_ > 0); }

  [[nodiscard]] ArrayRef first(size_t count) const {
    core_assert(count <= size());
    return ArrayRef(data(), count);
  }

  [[nodiscard]] ArrayRef last(size_t count) const {
    core_assert(count <= size());
    return ArrayRef(end() - count, end());
  }

  [[nodiscard]] ArrayRef subSpan(size_t offset) const {
    core_assert(offset <= size());
    return ArrayRef(data() + offset, size() - offset);
  }

  [[nodiscard]] ArrayRef subSpan(size_t offset, size_t count) const {
    core_assert(offset + count <= size());
    return ArrayRef(data() + offset, count);
  }

  [[nodiscard]] ArrayRef subRegion(size_t offset, size_t size) const {
    if (size == 0 || offset >= size_) {
      return ArrayRef();
    }

    if (size > size_ - offset) {
      size = size_ - offset;
    }

    return ArrayRef(data_ + offset, size);
  }

 private:
  T* data_;
  size_t size_;
};

template <class T>
ArrayRef<const char> AsBytes(ArrayRef<T> span) noexcept {
  return ArrayRef<const char>(reinterpret_cast<const char*>(span.data()), span.sizeBytes());
}

template <class T>
ArrayRef<char> AsWritableBytes(ArrayRef<T> span) noexcept {
  return ArrayRef<char>(reinterpret_cast<char*>(span.data()), span.sizeBytes());
}

template <class Range>
constexpr ArrayRef<const typename Range::value_type> MakeArrayRef(const Range& range) {
  return ArrayRef<const typename Range::value_type>(range);
}

template <class Range>
constexpr ArrayRef<typename Range::value_type> MakeArrayRef(Range& range) {
  return ArrayRef<typename Range::value_type>(range);
}

template <class Range>
constexpr ArrayRef<const typename Range::value_type> MakeConstArrayRef(const Range& range) {
  return ArrayRef<const typename Range::value_type>(range);
}

template <class Range>
constexpr ArrayRef<const typename Range::value_type> MakeConstArrayRef(Range& range) {
  return ArrayRef<const typename Range::value_type>(range);
}

template <class T>
constexpr ArrayRef<T> MakeArrayRef(T* data, size_t size) {
  return ArrayRef<T>(data, size);
}

template <class T>
constexpr ArrayRef<T> MakeArrayRef(T* begin, T* end) {
  return ArrayRef<T>(begin, end);
}

template <class T>
using ConstArrayRef = ArrayRef<const T>;

}  // namespace core