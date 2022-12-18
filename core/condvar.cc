#include "condvar.h"
#include "assert.h"
#include "error.h"
#include "exception.h"

#include <cerrno>

#include <pthread.h>

class core::CondVar::Impl {
 public:
  inline Impl() {
    if (pthread_cond_init(&cond_, nullptr)) {
      core_throw Exception() << "can not create condvar(" << LastSystemErrorText() << ")";
    }
  }

  inline ~Impl() {
    int ret = pthread_cond_destroy(&cond_);
    core_verify(ret == 0, "pthread_cond_destroy failed: %s", LastSystemErrorText(ret));
  }

  inline void signal() noexcept {
    int ret = pthread_cond_signal(&cond_);
    core_verify(ret == 0, "pthread_cond_signal failed: %s", LastSystemErrorText(ret));
  }

  inline void broadCast() noexcept {
    int ret = pthread_cond_broadcast(&cond_);
    core_verify(ret == 0, "pthread_cond_broadcast failed: %s", LastSystemErrorText(ret));
  }

  inline auto wait(Mutex& m, Instant deadline) noexcept {
    if (deadline == Time::infiniteFuture()) {
      int ret = pthread_cond_wait(&cond_, (pthread_mutex_t*)m.handle());
      core_verify(ret == 0, "pthread_cond_wait failed: %s", LastSystemErrorText(ret));
      return true;
    } else {
      auto spec = Time::toTimespec(deadline);
      int ret = pthread_cond_timedwait(&cond_, (pthread_mutex_t*)m.handle(), &spec);
      core_verify(ret == 0 || ret == ETIMEDOUT, "pthread_cond_timedwait failed: %s", LastSystemErrorText(ret));
      return ret == 0;
    }
  }

 private:
  pthread_cond_t cond_;
};

core::CondVar::CondVar()
    : impl_(new Impl) {}

core::CondVar::~CondVar() = default;

void core::CondVar::signal() noexcept { impl_->signal(); }

void core::CondVar::broadCast() noexcept { impl_->broadCast(); }

bool core::CondVar::wait(Mutex& m, Instant deadline) noexcept { return impl_->wait(m, deadline); }