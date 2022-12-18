#pragma once

#include "macro.h"
#include "noncopyable.h"

namespace core {

template <class Lock>
struct LockTraits {
  static inline void acquire(Lock* l) noexcept { l->acquire(); }
  static inline void release(Lock* l) noexcept { l->release(); }
};

template <class Traits>
struct InverseLockTraits : public Traits {
  template <class Lock>
  static inline void acquire(Lock* l) noexcept {
    Traits::release(l);
  }

  template <class Lock>
  static inline void release(Lock* l) noexcept {
    Traits::acquire(l);
  }
};

template <class Lock, class Traits = LockTraits<Lock>>
class Guarded : public NonCopyable {
 public:
  inline Guarded(const Lock& lock) noexcept { init(&lock); }

  inline Guarded(const Lock* lock) noexcept { init(lock); }

  inline Guarded(Guarded&& g) noexcept
      : lock_(g.lock_) {
    g.lock_ = nullptr;
  }

  inline ~Guarded() noexcept { release(); }

  inline void release() noexcept {
    if (wasAcquired()) {
      Traits::release(lock_);
      lock_ = nullptr;
    }
  }

  explicit inline operator bool() const noexcept { return wasAcquired(); }

  core_warn_unused_result inline bool wasAcquired() const noexcept { return lock_ != nullptr; }

  inline Lock* getLock() const noexcept { return lock_; }

 private:
  inline void init(const Lock* lock) noexcept {
    lock_ = const_cast<Lock*>(lock);
    Traits::acquire(lock_);
  }

 private:
  Lock* lock_;
};

template <class Lock, class Traits = LockTraits<Lock>>
using InverseGuarded = Guarded<Lock, InverseLockTraits<Traits>>;

template <class T>
static inline Guarded<T> Guard(const T& t) noexcept {
  return {&t};
}

template <class T, class Traits>
static inline InverseGuarded<T, Traits> Unguard(const Guarded<T, Traits>& g) {
  return {g.getLock()};
}

template <class T>
static inline InverseGuarded<T> Unguard(const T& t) {
  return {&t};
}

}  // namespace core

#define core_with_lock(lock)                                \
  if (auto core_unique_id(__guard) = ::core::Guard(lock)) { \
    goto core_concat(GUARD_LABEL, __LINE__);                \
  } else                                                    \
    core_concat(GUARD_LABEL, __LINE__)                      \
        :
