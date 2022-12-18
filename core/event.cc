#include "event.h"
#include "atomic.h"
#include "condvar.h"
#include "guard.h"
#include "mutex.h"

class core::SystemEvent::EvImpl : public core::AtomicRefCount<EvImpl> {
 public:
  inline EvImpl(ResetMode mode)
      : manual_(mode == MANUAL) {}

  inline void signal() noexcept {
    if (manual_ && atomics::Load(signaled_)) {
      return;
    }

    core_with_lock(mutex_) { atomics::Store(signaled_, 1); }

    if (manual_) {
      cond_.broadCast();
    } else {
      cond_.signal();
    }
  }

  inline void reset() noexcept { atomics::Store(signaled_, 0); }

  inline auto wait(Instant deadline) noexcept {
    if (manual_ && atomics::Load(signaled_)) {
      return true;
    }

    bool res_signaled = true;

    core_with_lock(mutex_) {
      while (!atomics::Load(signaled_)) {
        if (!cond_.wait(mutex_, deadline)) {
          res_signaled = atomics::Load(signaled_);
          break;
        }
      }

      if (!manual_) {
        atomics::Store(signaled_, 0);
      }
    }

    return res_signaled;
  }

 private:
  CondVar cond_;
  Mutex mutex_;
  Atomic signaled_ = 0;
  bool manual_;
};

namespace core {

SystemEvent::SystemEvent(ResetMode mode)
    : impl_(new EvImpl(mode)) {}

SystemEvent::SystemEvent(const SystemEvent& other) noexcept
    : impl_(other.impl_) {}

auto SystemEvent::operator=(const SystemEvent& other) noexcept -> SystemEvent& {
  impl_ = other.impl_;
  return *this;
}

SystemEvent::~SystemEvent() = default;

void SystemEvent::reset() noexcept { impl_->reset(); }

void SystemEvent::signal() noexcept { impl_->signal(); }

bool SystemEvent::wait(Instant deadline) noexcept { return impl_->wait(deadline); }

}  // namespace core
