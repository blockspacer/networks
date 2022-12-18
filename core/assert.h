#pragma once

#include "backtrace.h"
#include "macro.h"

#include <cassert>

#if !defined(NDEBUG)
#define core_assert(x)                            \
  do {                                            \
    try {                                         \
      if (core_unlikely(!(x))) {                  \
        core::PrintBackTrace();                   \
        assert(false && (x));                     \
      }                                           \
    } catch (...) {                               \
      core::PrintBackTrace();                     \
      assert(false && "Exception during assert"); \
    }                                             \
  } while (false)
#else
#define core_assert(x)                   \
  do {                                   \
    if (false) {                         \
      auto __xxx = static_cast<bool>(x); \
      core_ignore_result(__xxx);         \
    }                                    \
  } while (false)
#endif

namespace core::detail {

[[noreturn]] void Panic(const char* file, int line, const char* function, const char* expr, const char* format, ...)
    core_printf_like(5, 6);

}  // namespace core::detail

#define core_verify(expr, ...)                                                         \
  do {                                                                                 \
    if (core_unlikely(!(expr))) {                                                      \
      ::core::detail::Panic(__FILE__, __LINE__, __FUNCTION__, #expr, " " __VA_ARGS__); \
    }                                                                                  \
  } while (false)

#define core_fail(...)                                                                 \
  do {                                                                                 \
    ::core::detail::Panic(__FILE__, __LINE__, __FUNCTION__, nullptr, " " __VA_ARGS__); \
  } while (false)
