#include "thread_pool.h"
#include "atomic.h"
#include "condvar.h"
#include "event.h"
#include "guard.h"
#include "intrusive_list.h"
#include "intrusive_ptr.h"
#include "mutex.h"
#include "singleton.h"
#include "thread.h"
#include "thread_factory.h"

#include <iostream>
#include <memory>
#include <queue>
#include <vector>

namespace {

class ThreadNamer {
 public:
  ThreadNamer(const core::IThreadPool::Params& params)
      : thread_name_(params.thread_name)
      , enumerate_(params.enumerate_threads) {}

  explicit operator bool() const { return !thread_name_.empty(); }

  void setCurrentThreadName() {
    if (enumerate_) {
      set(thread_name_ + std::to_string(core::atomics::GetAndIncrement(index_)));
    } else {
      set(thread_name_);
    }
  }

 private:
  void set(const std::string& name) { core::Thread::setCurrentThreadName(name.c_str()); }

 private:
  std::string thread_name_;
  bool enumerate_ = false;
  core::Atomic index_{0};
};

}  // namespace

core::ThreadFactoryHolder::ThreadFactoryHolder() noexcept
    : factory_(core::SystemThreadFactory()) {}

class core::ThreadPool::Impl : public core::IntrusiveListItem<Impl>, public IThreadFactory::IThreadAble {
  using Tsr = IThreadPool::Tsr;
  using JobQueue = std::queue<IObjectInQueue*>;
  using ThreadRef = std::unique_ptr<IThreadFactory::IThread>;

 public:
  inline Impl(ThreadPool* parent, size_t thrnum, size_t maxque, Params p)
      : parent_(parent)
      , blocking_(p.blocking)
      , catching_(p.catching)
      , namer_(p)
      , should_terminate_(1)
      , max_queue_size_(0)
      , thread_count_expected_(0)
      , thread_count_real_(0)
      , forked_(false) {
    AtForkQueueRestarter::get().add(this);
    start(thrnum, maxque);
  }

  inline ~Impl() override {
    try {
      stop();
    } catch (...) {
    }

    AtForkQueueRestarter::get().remove(this);
    core_assert(thread_array_.empty());
  }

  inline auto add(IObjectInQueue* obj) {
    if (atomics::Load(should_terminate_)) {
      return false;
    }

    if (thread_array_.empty()) {
      Tsr tsr(parent_);
      obj->process(&tsr);
      return true;
    }

    core_with_lock(queue_mutex_) {
      while (max_queue_size_ > 0 && queue_.size() >= max_queue_size_ && !atomics::Load(should_terminate_)) {
        if (!blocking_) {
          return false;
        }
        queue_pop_cond_.wait(queue_mutex_);
      }

      if (atomics::Load(should_terminate_)) {
        return false;
      }

      queue_.push(obj);
    }

    queue_push_cond_.signal();

    return true;
  }

  inline auto size() const noexcept {
    core_with_lock(queue_mutex_) { return queue_.size(); }
  }

  inline auto maxQueueSize() const noexcept { return max_queue_size_; }

  inline auto threadCountExpected() const noexcept { return thread_count_expected_; }

  inline auto threadCountReal() const noexcept { return thread_count_real_; }

  inline void atForkAction() noexcept { forked_ = true; }

  inline auto needRestart() const noexcept { return forked_; }

 private:
  inline void start(size_t num, size_t queue) {
    atomics::Store(should_terminate_, 0);
    max_queue_size_ = queue;
    thread_count_expected_ = num;
    try {
      for (size_t i = 0; i < num; ++i) {
        thread_array_.push_back(parent_->factory()->run(this));
        ++thread_count_real_;
      }
    } catch (...) {
      stop();
      throw;
    }
  }

  inline void stop() {
    atomics::Store(should_terminate_, 1);
    core_with_lock(queue_mutex_) { queue_pop_cond_.broadCast(); }

    if (!needRestart()) {
      waitForComplete();
    }

    thread_array_.clear();
    thread_count_expected_ = 0;
    max_queue_size_ = 0;
  }

  inline void waitForComplete() noexcept {
    core_with_lock(stop_mutex_) {
      while (thread_count_real_) {
        core_with_lock(queue_mutex_) { queue_push_cond_.signal(); }
        stop_cond_.wait(stop_mutex_);
      }
    }
  }

  void doExecute() override {
    std::unique_ptr<Tsr> tsr(new Tsr(parent_));
    if (namer_) {
      namer_.setCurrentThreadName();
    }

    while (true) {
      IObjectInQueue* job = nullptr;

      core_with_lock(queue_mutex_) {
        while (queue_.empty() && !atomics::Load(should_terminate_)) {
          queue_push_cond_.wait(queue_mutex_);
        }

        if (atomics::Load(should_terminate_) && queue_.empty()) {
          tsr.reset();
          break;
        }

        job = queue_.front();
        queue_.pop();
      }

      queue_pop_cond_.signal();

      if (catching_) {
        try {
          try {
            job->process(*tsr);
          } catch (...) {
            std::cerr << "[thread pool] " << CurrentExceptionMessage() << std::endl;
          }
        } catch (...) {
        }
      } else {
        job->process(*tsr);
      }
    }

    finishOneThread();
  }

  inline void finishOneThread() noexcept {
    core_with_lock(stop_mutex_) {
      --thread_count_real_;
      stop_cond_.signal();
    }
  }

 private:
  class AtForkQueueRestarter {
   public:
    inline AtForkQueueRestarter() { pthread_atfork(nullptr, nullptr, processChildAction); }

    static AtForkQueueRestarter& get() { return *SingletonWithPriority<AtForkQueueRestarter, 256>(); }

    inline void add(Impl* obj) {
      core_with_lock(mutex_) { objects_.pushBack(obj); }
    }

    inline void remove(Impl* obj) {
      core_with_lock(mutex_) { obj->unlink(); }
    }

   private:
    void childAction() {
      core_with_lock(mutex_) {
        for (auto& object : objects_) {
          object.atForkAction();
        }
      }
    }

    static void processChildAction() { get().childAction(); }

   private:
    IntrusiveList<Impl> objects_;
    Mutex mutex_;
  };

 private:
  ThreadPool* parent_;
  const bool blocking_;
  const bool catching_;
  ThreadNamer namer_;

  mutable Mutex queue_mutex_{};
  mutable Mutex stop_mutex_{};

  CondVar queue_push_cond_{};
  CondVar queue_pop_cond_{};
  CondVar stop_cond_{};

  JobQueue queue_;
  std::vector<ThreadRef> thread_array_;

  Atomic should_terminate_;

  size_t max_queue_size_;
  size_t thread_count_expected_;
  size_t thread_count_real_;

  bool forked_;
};

core::ThreadPool::~ThreadPool() = default;

size_t core::ThreadPool::size() const noexcept { return (impl_.get() ? impl_->size() : 0); }

size_t core::ThreadPool::maxQueueSize() const noexcept { return (impl_.get() ? impl_->maxQueueSize() : 0); }

size_t core::ThreadPool::threadCountExpected() const noexcept {
  return (impl_.get() ? impl_->threadCountExpected() : 0);
}

size_t core::ThreadPool::threadCountReal() const noexcept { return (impl_.get() ? impl_->threadCountReal() : 0); }

bool core::ThreadPool::add(IObjectInQueue* obj) {
  core_ensure(impl_.get(), ThreadPoolException() << "thread pool not started");
  if (impl_->needRestart()) {
    start(impl_->threadCountExpected(), impl_->maxQueueSize());
  }
  return impl_->add(obj);
}

void core::ThreadPool::start(size_t thread_count, size_t queue_size_limit) {
  impl_ = std::make_unique<Impl>(this, thread_count, queue_size_limit, params_);
}

void core::ThreadPool::stop() noexcept { impl_.reset(); }

static core::Atomic thread_pool_counter = 0;

class core::AdaptiveThreadPool::Impl {
 public:
  class Thread : public IThreadFactory::IThreadAble {
   public:
    inline Thread(Impl* parent)
        : impl_(parent)
        , thread_(impl_->parent_->factory()->run(this)) {}

    inline ~Thread() override { impl_->decThreadCount(); }

   private:
    void doExecute() noexcept override {
      std::unique_ptr<Thread> self(this);

      if (impl_->namer_) {
        impl_->namer_.setCurrentThreadName();
      }

      {
        Tsr tsr(impl_->parent_);
        IObjectInQueue* obj;

        while ((obj = impl_->waitForJob()) != nullptr) {
          if (impl_->catching_) {
            try {
              try {
                obj->process(tsr);
              } catch (...) {
                std::cerr << impl_->name() << " " << CurrentExceptionMessage() << std::endl;
              }
            } catch (...) {
            }
          } else {
            obj->process(tsr);
          }
        }
      }
    }

   private:
    Impl* impl_;
    std::unique_ptr<IThreadFactory::IThread> thread_;
  };

  inline Impl(AdaptiveThreadPool* parent, Params p)
      : parent_(parent)
      , catching_(p.catching)
      , namer_(p)
      , thread_count_(0)
      , all_done_(false)
      , obj_(nullptr)
      , free_(0)
      , idle_time_(absl::InfiniteDuration()) {
    sprintf(name_, "[thread pool %ld]", (long)atomics::Increment(thread_pool_counter));
  }

  inline ~Impl() { stop(); }

  inline void setMaxIdleTime(Duration idle_time) { idle_time_ = idle_time; }

  core_warn_unused_result inline const char* name() const noexcept { return name_; }

  inline void add(IObjectInQueue* obj) {
    core_with_lock(mutex_) {
      while (obj_ != nullptr) {
        cond_free_.wait(mutex_);
      }

      if (free_ == 0) {
        addThreadNoLock();
      }

      obj_ = obj;

      core_ensure(!all_done_, ThreadPoolException() << "adding to a stopped queue");
    }

    cond_ready_.signal();
  }

  inline void addThreads(size_t n) {
    core_with_lock(mutex_) {
      while (n) {
        addThreadNoLock();
        --n;
      }
    }
  }

  core_warn_unused_result inline size_t size() const noexcept { return atomics::Load(thread_count_); }

 private:
  inline void incThreadCount() noexcept { atomics::Increment(thread_count_); }

  inline void decThreadCount() noexcept { atomics::Decrement(thread_count_); }

  inline void addThreadNoLock() {
    incThreadCount();
    try {
      new Thread(this);
    } catch (...) {
      decThreadCount();
      throw;
    }
  }

  inline void stop() noexcept {
    mutex_.acquire();
    all_done_ = true;

    while (atomics::Load(thread_count_)) {
      mutex_.release();
      cond_ready_.signal();
      mutex_.acquire();
    }

    mutex_.release();
  }

  inline IObjectInQueue* waitForJob() noexcept {
    mutex_.acquire();
    ++free_;

    while (!obj_ && !all_done_) {
      if (!cond_ready_.wait(mutex_, idle_time_)) {
        break;
      }
    }

    IObjectInQueue* ret = obj_;
    obj_ = nullptr;

    --free_;
    mutex_.release();
    cond_free_.signal();

    return ret;
  }

 private:
  AdaptiveThreadPool* parent_;
  const bool catching_;

  ThreadNamer namer_;
  Atomic thread_count_;

  Mutex mutex_;
  CondVar cond_ready_;
  CondVar cond_free_;

  bool all_done_;
  IObjectInQueue* obj_;

  size_t free_;
  char name_[64];
  Duration idle_time_;
};

core::ThreadPoolBase::ThreadPoolBase(Params p)
    : ThreadFactoryHolder(p.factory)
    , params_(std::move(p)) {}

#define DEFINE_THREAD_POOL_CTORS(type) \
  type::type(Params params)            \
      : ThreadPoolBase(std::move(params)) {}

namespace core {

DEFINE_THREAD_POOL_CTORS(ThreadPool)
DEFINE_THREAD_POOL_CTORS(AdaptiveThreadPool)
DEFINE_THREAD_POOL_CTORS(SimpleThreadPool)

}  // namespace core

core::AdaptiveThreadPool::~AdaptiveThreadPool() = default;

bool core::AdaptiveThreadPool::add(IObjectInQueue* obj) {
  core_ensure(impl_.get(), ThreadPoolException() << "thread pool not started");
  impl_->add(obj);
  return true;
}

void core::AdaptiveThreadPool::start(size_t, size_t) { impl_.reset(new Impl(this, params_)); }

void core::AdaptiveThreadPool::stop() noexcept { impl_.reset(); }

size_t core::AdaptiveThreadPool::size() const noexcept { return impl_.get() ? impl_->size() : 0; }

void core::AdaptiveThreadPool::setMaxIdleTime(Duration interval) {
  core_ensure(impl_.get(), ThreadPoolException() << "thread pool not started");
  impl_->setMaxIdleTime(interval);
}

core::SimpleThreadPool::~SimpleThreadPool() {
  try {
    stop();
  } catch (...) {
  }
}

bool core::SimpleThreadPool::add(IObjectInQueue* obj) {
  core_ensure(slave_.get(), ThreadPoolException() << "thread pool not started");
  return slave_->add(obj);
}

void core::SimpleThreadPool::start(size_t thr_num, size_t max_queque_size) {
  std::unique_ptr<IThreadPool> tmp;
  AdaptiveThreadPool* adaptive = nullptr;

  if (thr_num) {
    tmp = std::make_unique<ThreadPoolBinder<ThreadPool, SimpleThreadPool>>(this, params_);
  } else {
    adaptive = new ThreadPoolBinder<AdaptiveThreadPool, SimpleThreadPool>(this, params_);
    tmp.reset(adaptive);
  }

  tmp->start(thr_num, max_queque_size);

  if (adaptive) {
    adaptive->setMaxIdleTime(absl::Seconds(100));
  }

  slave_.swap(tmp);
}

void core::SimpleThreadPool::stop() noexcept { slave_.reset(); }

size_t core::SimpleThreadPool::size() const noexcept { return slave_.get() ? slave_->size() : 0; }

namespace {

class OwnedObjectInQueue : public core::IObjectInQueue {
 public:
  OwnedObjectInQueue(std::unique_ptr<IObjectInQueue> owned)
      : owned_(std::move(owned)) {}

  void process(void* data) override {
    std::unique_ptr<OwnedObjectInQueue> self(this);
    owned_->process(data);
  }

 private:
  std::unique_ptr<IObjectInQueue> owned_;
};

}  // namespace

void core::IThreadPool::safeAdd(IObjectInQueue* obj) {
  core_ensure(add(obj), ThreadPoolException() << "can not add object to queue");
}

void core::IThreadPool::safeAddAndOwn(std::unique_ptr<IObjectInQueue> obj) {
  core_ensure(addAndOwn(std::move(obj)), ThreadPoolException() << "can not add object to queue");
}

bool core::IThreadPool::addAndOwn(std::unique_ptr<IObjectInQueue> obj) {
  auto owner = std::make_unique<OwnedObjectInQueue>(std::move(obj));
  bool added = add(owner.get());
  if (added) {
    core_ignore_result(owner.release());
  }
  return added;
}

using IThread = core::IThreadFactory::IThread;
using IThreadAble = core::IThreadFactory::IThreadAble;

namespace {

class PoolThread : public IThread {
  class ThreadImpl : public core::IObjectInQueue, public core::AtomicRefCount<ThreadImpl> {
   public:
    inline ThreadImpl(IThreadAble* func)
        : func_(func) {}

    ~ThreadImpl() override = default;

    inline void waitForStart() noexcept { start_event_.wait(); }

    inline void waitForComplete() noexcept { complete_event_.wait(); }

   private:
    void process(void*) override {
      ThreadImplRef self(this);
      {
        start_event_.signal();
        try {
          func_->execute();
        } catch (...) {
        }
        complete_event_.signal();
      }
    }

   private:
    IThreadAble* func_;
    core::SystemEvent complete_event_;
    core::SystemEvent start_event_;
  };

  using ThreadImplRef = core::IntrusivePtr<ThreadImpl>;

 public:
  inline PoolThread(core::IThreadPool* parent)
      : parent_(parent) {}

  ~PoolThread() override {
    if (impl_) {
      impl_->waitForStart();
    }
  }

 private:
  void doRun(IThreadAble* func) override {
    ThreadImplRef impl(new ThreadImpl(func));
    parent_->safeAdd(impl.get());
    impl_.swap(impl);
  }

  void doJoin() noexcept override {
    if (impl_) {
      impl_->waitForComplete();
      impl_ = nullptr;
    }
  }

 private:
  core::IThreadPool* parent_;
  ThreadImplRef impl_;
};

}  // namespace

IThread* core::IThreadPool::doCreate() { return new PoolThread(this); }

std::unique_ptr<core::IThreadPool> core::CreateThreadPool(size_t thread_count, size_t queue_size_limit,
                                                          IThreadPool::Params params) {
  std::unique_ptr<IThreadPool> pool;
  if (thread_count > 1) {
    pool = std::make_unique<ThreadPool>(params);
  } else {
    pool = std::make_unique<FakeThreadPool>();
  }

  pool->start(thread_count, queue_size_limit);
  return pool;
}