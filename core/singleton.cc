#include "singleton.h"
#include "assert.h"
#include "atomic.h"
#include "sanitizers.h"
#include "spinlock.h"
#include "thread.h"

#include <cstring>

namespace {

using namespace core;

static inline bool MyAtomicTryLock(Atomic& a, AtomicType v) noexcept { return atomics::Cas(&a, v, 0); }

static inline bool MyAtomicTryAndTryLock(Atomic& a, AtomicType v) noexcept {
  return (atomics::Load(a) == 0) && MyAtomicTryLock(a, v);
}

static inline AtomicType MyThreadId() noexcept {
  const AtomicType ret = Thread::currentThreadId();
  if (ret) {
    return ret;
  }
  return 1;
}

}  // namespace

void core::detail::FillWithTrash(void* ptr, size_t len) {
#if defined(NDEBUG) || defined(core_thread_sanitizer)
  core_ignore_result(ptr, len);
#else
  memset(ptr, 0xBA, len);
#endif
}

void core::detail::LockRecursive(Atomic& lock) noexcept {
  const AtomicType id = MyThreadId();
  core_verify(atomics::Load(lock) != id, "recursive singleton initialization");
  if (!MyAtomicTryLock(lock, id)) {
    SpinWait sw;
    do {
      sw.sleep();
    } while (!MyAtomicTryAndTryLock(lock, id));
  }
}

void core::detail::UnlockRecursive(Atomic& lock) noexcept {
  core_verify(atomics::Load(lock) == MyThreadId(), "unlock from another thread?!?!");
  atomics::Unlock(&lock);
}