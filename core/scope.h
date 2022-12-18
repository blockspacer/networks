#pragma once

#include "macro.h"

#include <utility>

namespace core {

template <class T>
class ScopeGuard {
 public:
  ScopeGuard(const T& f) noexcept
      : function_(f) {}

  ScopeGuard(T&& f) noexcept
      : function_(std::move(f)) {}

  ScopeGuard(ScopeGuard&&) noexcept = default;
  ScopeGuard(const ScopeGuard&) = default;

  ~ScopeGuard() { function_(); }

 private:
  T function_;
};

struct MakeGuardHelper {
  template <class T>
  ScopeGuard<T> operator|(T&& func) const {
    return std::forward<T>(func);
  }
};

}  // namespace core

#define core_scope_exit(...) \
  [[maybe_unused]] const auto core_unique_id(scope_guard) = ::core::MakeGuardHelper{} | [__VA_ARGS__]() mutable -> void

#define core_defer core_scope_exit(&)