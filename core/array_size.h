#pragma once

#include <cstddef>

namespace core {

template <class T>
struct ArraySize;

template <class T, size_t N>
struct ArraySize<T[N]> {
  enum { RESULT = N };
};

template <class T, size_t N>
struct ArraySize<T (&)[N]> {
  enum { RESULT = N };
};

}  // namespace core

#define core_array_size(arr) (static_cast<size_t>(::core::ArraySize<decltype(arr)>::RESULT))