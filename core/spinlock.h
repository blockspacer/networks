#pragma once

#include "atomic.h"
#include "macro.h"

namespace core {

class SpinWait {
 public:
  SpinWait() noexcept;
  void sleep() noexcept;

 private:
  unsigned t_;
  unsigned c_;
};

class SpinLockBase {
 public:
  core_warn_unused_result inline bool isLocked() const noexcept { return atomics::Load(value_); }

  inline bool tryAcquire() noexcept { return atomics::TryLock(&value_); }

  static inline void spinLockPause() noexcept { __asm __volatile("pause"); }

 protected:
  SpinLockBase() noexcept { atomics::Store(value_, 0); }

 protected:
  Atomic value_;
};

static inline void AcquireAdaptiveLock(Atomic* l) noexcept {
  if (!atomics::TryLock(l)) {
    SpinWait sw;
    while (!atomics::TryAndTryLock(l)) {
      sw.sleep();
    }
  }
}

static inline void ReleaseAdaptiveLock(Atomic* l) noexcept { atomics::Unlock(l); }

class SpinLock : public SpinLockBase {
 public:
  inline void release() noexcept { atomics::Unlock(&value_); }

  inline void acquire() noexcept {
    if (!atomics::TryLock(&value_)) {
      do {
        spinLockPause();
      } while (!atomics::TryAndTryLock(&value_));
    }
  }
};

class AdaptiveLock : public SpinLockBase {
 public:
  inline void acquire() noexcept { AcquireAdaptiveLock(&value_); }

  inline void release() noexcept { ReleaseAdaptiveLock(&value_); }
};

}  // namespace core

#include "guard.h"

template <>
struct core::LockTraits<core::Atomic> {
  static inline void acquire(Atomic* l) noexcept { AcquireAdaptiveLock(l); }
  static inline void release(Atomic* l) noexcept { ReleaseAdaptiveLock(l); }
};