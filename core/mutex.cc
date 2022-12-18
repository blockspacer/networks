#include "mutex.h"
#include "assert.h"
#include "error.h"
#include "exception.h"

#include <cstring>
#include <pthread.h>

class core::Mutex::Impl {
 public:
  inline Impl() {
    struct T {
      inline T() {
        memset(&attr, 0, sizeof(attr));
        int res = pthread_mutexattr_init(&attr);

        if (res != 0) {
          core_throw Exception() << "mutexattr init failed(" << LastSystemErrorText(res) << ")";
        }

        res = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (res != 0) {
          core_throw Exception() << "mutexattr set type failed(" << LastSystemErrorText(res) << ")";
        }
      }

      inline ~T() {
        int res = pthread_mutexattr_destroy(&attr);
        core_verify(res == 0, "mutexattr destroy (%s)", LastSystemErrorText(res));
      }

      pthread_mutexattr_t attr;
    } pma;

    int res = pthread_mutex_init(&mutex_, &pma.attr);
    if (res != 0) {
      core_throw Exception() << "mutex init failed(" << LastSystemErrorText(res) << ")";
    }
  }

  inline ~Impl() {
    int r = pthread_mutex_destroy(&mutex_);
    core_verify(r == 0, "mutex destroy failure (%s)", LastSystemErrorText(r));
  }

  inline void acquire() noexcept {
    int r = pthread_mutex_lock(&mutex_);
    core_verify(r == 0, "mutex lock failure (%s)", LastSystemErrorText(r));
  }

  inline auto tryAcquire() noexcept {
    int r = pthread_mutex_trylock(&mutex_);
    if (r == 0 || r == EBUSY) {
      return r == 0;
    }
    core_fail("mutex trylock failure (%s)", LastSystemErrorText(r));
  }

  inline void release() noexcept {
    int r = pthread_mutex_unlock(&mutex_);
    core_verify(r == 0, "mutex unlock failure (%s)", LastSystemErrorText(r));
  }

  core_warn_unused_result inline auto handle() const noexcept {
    return const_cast<void*>(static_cast<const void*>(&mutex_));
  }

 private:
  pthread_mutex_t mutex_;
};

core::Mutex::Mutex()
    : impl_(new Impl()) {}

core::Mutex::Mutex(Mutex&&) noexcept = default;

core::Mutex::~Mutex() = default;

void core::Mutex::acquire() noexcept { impl_->acquire(); }

auto core::Mutex::tryAcquire() noexcept -> bool { return impl_->tryAcquire(); }

void core::Mutex::release() noexcept { impl_->release(); }

auto core::Mutex::handle() const noexcept -> void* { return impl_->handle(); }