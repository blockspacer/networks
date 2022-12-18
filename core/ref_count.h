#pragma once

#include "assert.h"
#include "atomic.h"
#include "fwd.h"

namespace core {

class AtomicCounter {
 public:
  inline AtomicCounter(long initial = 0) noexcept
      : counter_(initial) {}

  inline ~AtomicCounter() = default;

  inline AtomicType add(AtomicType d) noexcept { return atomics::Add(counter_, d); }

  inline AtomicType inc() noexcept { return add(1); }

  inline AtomicType sub(AtomicType d) noexcept { return atomics::Sub(counter_, d); }

  inline AtomicType dec() noexcept { return sub(1); }

  core_warn_unused_result inline AtomicType value() const noexcept { return atomics::Load(counter_); }

 private:
  Atomic counter_;
};

template <class T, class C, class D>
class RefCounted {
 public:
  inline RefCounted(long init_val = 0) noexcept
      : counter_(init_val) {}

  inline ~RefCounted() = default;

  inline void ref(AtomicType d) noexcept {
    auto result_count = counter_.add(d);
    core_assert(result_count >= d);
    core_ignore_result(result_count);
  }

  inline void ref() noexcept {
    auto result_count = counter_.inc();
    core_assert(result_count != 0);
    core_ignore_result(result_count);
  }

  inline void unRef(AtomicType d) noexcept {
    auto result_count = counter_.sub(d);
    core_assert(result_count >= 0);
    if (result_count == 0) {
      D::destroy(static_cast<T*>(this));
    }
  }

  inline void unRef() noexcept { unRef(1); }

  core_warn_unused_result inline AtomicType refCount() const noexcept { return counter_.value(); }

  inline void decRef() noexcept {
    auto result_count = counter_.dec();
    core_assert(result_count >= 0);
    core_ignore_result(result_count);
  }

  RefCounted(const RefCounted&)
      : counter_(0) {}

  RefCounted& operator=(const RefCounted&) { return *this; }

 private:
  C counter_;
};

}  // namespace core