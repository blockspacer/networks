#include "thread.h"
#include "assert.h"
#include "exception.h"
#include "macro.h"

#include <cstdint>
#include <limits>

#include <pthread.h>
#include <string.h>

#include <sys/prctl.h>

core_maybe_unused static constexpr auto IntHashImpl(uint8_t key8) noexcept {
  size_t key = key8;

  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);

  return static_cast<uint8_t>(key);
}

core_maybe_unused static constexpr auto IntHashImpl(uint16_t key16) noexcept {
  size_t key = key16;

  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);

  return static_cast<uint16_t>(key);
}

core_maybe_unused static constexpr auto IntHashImpl(uint32_t key) noexcept {
  key += ~(key << 15);
  key ^= (key >> 10);
  key += (key << 3);
  key ^= (key >> 6);
  key += ~(key << 11);
  key ^= (key >> 16);

  return key;
}

core_maybe_unused static constexpr auto IntHashImpl(uint64_t key) noexcept {
  key += ~(key << 32);
  key ^= (key >> 22);
  key += ~(key << 13);
  key ^= (key >> 8);
  key += (key << 3);
  key ^= (key >> 15);
  key += ~(key << 27);
  key ^= (key >> 31);

  return key;
}

template <class T>
static constexpr T intHash(T t) noexcept {
  if constexpr (sizeof(T) == 1) {
    return IntHashImpl(static_cast<uint8_t>(t));
  } else if constexpr (sizeof(T) == 2) {
    return IntHashImpl(static_cast<uint16_t>(t));
  } else if constexpr (sizeof(T) == 4) {
    return IntHashImpl(static_cast<uint32_t>(t));
  } else if constexpr (sizeof(T) == 8) {
    return IntHashImpl(static_cast<uint64_t>(t));
  }
  core_unreachable();
}

static inline auto SystemCurrentThreadIdImpl() noexcept { return static_cast<size_t>(pthread_self()); }

template <class T>
static inline auto ThreadIdHashFunction(T t) noexcept {
  return intHash(t);
}

static inline auto SystemCurrentThreadId() noexcept { return ThreadIdHashFunction(SystemCurrentThreadIdImpl()); }

namespace {

using Params = core::Thread::Params;
using Id = core::Thread::Id;

inline void SetThrName(const Params& p) {
  try {
    if (!p.name.empty()) {
      core::Thread::setCurrentThreadName(p.name.c_str());
    }
  } catch (...) {
  }
}

static inline auto FastCl2(uint64_t v) { return 1ULL << (sizeof(uint64_t) * 8 - __builtin_clzll(v)); }

inline size_t StackSize(const Params& p) noexcept {
  if (p.stack_size) {
    return static_cast<size_t>(FastCl2(p.stack_size));
  }
  return 0;
}

#define core_pcheck(x, y)                       \
  {                                             \
    const int err__ = x;                        \
    if (err__) {                                \
      core_throw core::SystemError(err__) << y; \
    }                                           \
  }

class PosixThread {
 public:
  inline PosixThread(const Params& p)
      : params_(new Params(p))
      , handle_() {}

  core_warn_unused_result inline auto systemThreadId() const noexcept { return static_cast<Id>(handle_); }

  inline auto join() {
    void* tec = nullptr;
    core_pcheck(pthread_join(handle_, &tec), "can not join thread");
    return tec;
  }

  inline void detach(){core_pcheck(pthread_detach(handle_), "can not detach thread")}

  core_warn_unused_result inline auto isRunning() const noexcept {
    return static_cast<bool>(handle_);
  }

  inline void start() {
    pthread_attr_t* pattrs = nullptr;
    pthread_attr_t attrs;

    if (params_->stack_size > 0) {
      memset(&attrs, 0, sizeof(pthread_attr_t));
      pthread_attr_init(&attrs);
      pattrs = &attrs;

      if (params_->stack_pointer) {
        pthread_attr_setstack(pattrs, params_->stack_pointer, params_->stack_size);
      } else {
        pthread_attr_setstacksize(pattrs, StackSize(*params_));
      }
    }

    {
      Params* hold = params_.release();
      int err = pthread_create(&handle_, pattrs, threadProxy, hold);
      if (err) {
        handle_ = {};
        params_.reset(hold);
        core_pcheck(err, "failed to create thread");
      }
    }
  }

 private:
  static void* threadProxy(void* arg) {
    std::unique_ptr<Params> p(static_cast<Params*>(arg));
    SetThrName(*p);
    return p->proc(p->data);
  }

 private:
  std::unique_ptr<Params> params_;
  pthread_t handle_;
};

template <class T>
static inline typename T::element_type* CheckedImpl(T& t, const char* op, bool check = true) {
  if (!t) {
    core_throw core::Exception() << "can not " << op << " dead thread";
  }

  if (t->isRunning() != check) {
    static const char* const msg[] = {"running", "not running"};
    core_throw core::Exception() << "can not " << op << " " << msg[check] << " thread";
  }

  return t.get();
}

#undef core_pcheck

}  // namespace

namespace core {

class Thread::Impl : public PosixThread {
 public:
  inline Impl(const Params& params, std::unique_ptr<CallableBase> callable = {})
      : PosixThread(params)
      , callable_(std::move(callable)) {}

  core_warn_unused_result inline auto id() const noexcept { return ThreadIdHashFunction(systemThreadId()); }

  static std::unique_ptr<Impl> create(std::unique_ptr<CallableBase> callable) {
    Params params(CallableBase::threadWorker, callable.get());
    return std::make_unique<Impl>(std::move(params), std::move(callable));
  }

 private:
  std::unique_ptr<CallableBase> callable_;
};

Thread::Thread(const Params& params)
    : impl_(new Impl(params)) {}

Thread::Thread(ThreadProc proc, void* param)
    : impl_(new Impl(Params(proc, param))) {}

Thread::Thread(PrivateCtor, std::unique_ptr<CallableBase> callable)
    : impl_(Impl::create(std::move(callable))) {}

Thread::~Thread() { join(); }

void Thread::start() { CheckedImpl(impl_, "start", false)->start(); }

void* Thread::join() {
  if (isRunning()) {
    void* r = impl_->join();
    impl_.reset();
    return r;
  }
  return nullptr;
}

void Thread::detach() {
  if (isRunning()) {
    impl_->detach();
    impl_.reset();
  }
}

bool Thread::isRunning() const noexcept { return impl_ && impl_->isRunning(); }

Thread::Id Thread::id() const noexcept {
  if (isRunning()) {
    return impl_->id();
  }
  return impossibleThreadId();
}

Id Thread::currentThreadId() noexcept { return SystemCurrentThreadId(); }

Id Thread::impossibleThreadId() noexcept { return std::numeric_limits<Id>::max(); }

void Thread::setCurrentThreadName(const char* name) noexcept {
  core_verify(prctl(PR_SET_NAME, name, 0, 0, 0) == 0, "prctl failed: %s", strerror(errno));
}

std::string Thread::currentThreadName() noexcept {
  char name[16];
  memset(name, 0, sizeof(name));
  core_verify(prctl(PR_GET_NAME, name, 0, 0, 0) == 0, "prctl failed: %s", strerror(errno));
  return name;
}

namespace {

template <class T>
static void* ThreadProcWrapper(void* param) {
  return reinterpret_cast<T*>(param)->threadProc();
}

}  // namespace

ISimpleThread::ISimpleThread(size_t stack_size)
    : Thread(Params(ThreadProcWrapper<ISimpleThread>, reinterpret_cast<void*>(this), stack_size)) {}

}  // namespace core