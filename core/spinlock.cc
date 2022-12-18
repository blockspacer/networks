#include "spinlock.h"
#include "yield.h"

#include "absl/hash/hash.h"
#include "atomic.h"

#include <algorithm>

#include <unistd.h>

template <class T>
static inline T RandomizeSleepTime(T t) noexcept {
  static core::Atomic counter = 0;
  static absl::Hash<T> hash;
  const auto rnd = static_cast<T>(hash(static_cast<T>(core::atomics::Increment(counter))));
  return (t * static_cast<T>(4) + (rnd % t) * static_cast<T>(2)) / static_cast<T>(5);
}

static const unsigned kMinSleepTime = 500;
static const unsigned kMaxSpinCount = 0x7FF;

core::SpinWait::SpinWait() noexcept
    : t_(kMinSleepTime)
    , c_(0) {}

void core::SpinWait::sleep() noexcept {
  ++c_;
  if (c_ == kMaxSpinCount) {
    ThreadYield();
  } else if ((c_ & kMaxSpinCount) == 0) {
    usleep(RandomizeSleepTime(t_));
    t_ = std::min<unsigned>((t_ * 3) / 2, 20000);
  } else {
    SpinLockBase::spinLockPause();
  }
}