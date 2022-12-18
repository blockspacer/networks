#pragma once

#include "macro.h"
#include "noncopyable.h"

#include <memory>

namespace core {

class FakeMutex : public NonCopyable {
 public:
  inline void acquire() noexcept {}

  inline auto tryAcquire() noexcept { return true; }

  inline void release() noexcept {}

  ~FakeMutex() = default;
};

class Mutex {
 public:
  Mutex();
  Mutex(Mutex&&) noexcept;
  ~Mutex();

  void acquire() noexcept;
  bool tryAcquire() noexcept;
  void release() noexcept;

  core_warn_unused_result void* handle() const noexcept;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace core
