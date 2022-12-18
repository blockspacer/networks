#pragma once

#include "assert.h"
#include "datetime.h"
#include "exception.h"
#include "fwd.h"
#include "noncopyable.h"
#include "thread_factory.h"

#include <utility>

namespace core {

struct IObjectInQueue {
  virtual ~IObjectInQueue() = default;

  virtual void process(void* tsr) = 0;
};

class ThreadFactoryHolder {
 public:
  ThreadFactoryHolder() noexcept;

  inline ThreadFactoryHolder(IThreadFactory* factory) noexcept
      : factory_(factory) {}

  inline ~ThreadFactoryHolder() = default;

  core_warn_unused_result inline auto factory() const noexcept { return factory_; }

 private:
  IThreadFactory* factory_;
};

class ThreadPoolException : public Exception {};

template <class T>
class ThrFuncObj : public IObjectInQueue {
 public:
  ThrFuncObj(const T& func)
      : func_(func) {}

  ThrFuncObj(T&& func)
      : func_(std::move(func)) {}

  void process(void*) override {
    std::unique_ptr<ThrFuncObj> self(this);
    func_();
  }

 private:
  T func_;
};

template <class T>
IObjectInQueue* MakeThrFuncObj(T&& func) {
  return new ThrFuncObj<std::remove_cv_t<std::remove_reference_t<T>>>(std::forward<T>(func));
}

struct ThreadPoolParams {
  ThreadPoolParams() = default;

  ThreadPoolParams(IThreadFactory* f)
      : factory(f) {}

  ThreadPoolParams(const std::string name)
      : thread_name(name) {}

  ThreadPoolParams& setCatching(bool val) {
    catching = val;
    return *this;
  }

  ThreadPoolParams& setBlocking(bool val) {
    blocking = val;
    return *this;
  }

  ThreadPoolParams& setFactory(IThreadFactory* f) {
    factory = f;
    return *this;
  }

  ThreadPoolParams& setThreadName(const std::string& name) {
    thread_name = name;
    enumerate_threads = false;
    return *this;
  }

  ThreadPoolParams& setThreadNamePrefix(const std::string& name) {
    thread_name = name;
    enumerate_threads = true;
    return *this;
  }

  bool catching = true;
  bool blocking = false;

  IThreadFactory* factory = SystemThreadFactory();
  std::string thread_name;
  bool enumerate_threads = false;
};

class IThreadPool : public IThreadFactory, public NonCopyable {
 public:
  using Params = ThreadPoolParams;

  ~IThreadPool() override = default;

  void safeAdd(IObjectInQueue* obj);

  template <class T>
  void safeAddFunc(T&& func) {
    core_ensure(addFunc(std::forward<T>(func)), ThreadPoolException() << "can not add function to queue");
  }

  void safeAddAndOwn(std::unique_ptr<IObjectInQueue> obj);

  core_warn_unused_result virtual bool add(IObjectInQueue* obj) = 0;

  template <class T>
  core_warn_unused_result bool addFunc(T&& func) {
    std::unique_ptr<IObjectInQueue> wrapper(MakeThrFuncObj(std::forward<T>(func)));
    bool added = add(wrapper.get());
    if (added) {
      core_ignore_result(wrapper.release());
    }
    return added;
  }

  core_warn_unused_result bool addAndOwn(std::unique_ptr<IObjectInQueue> obj);

  virtual void start(size_t thread_count, size_t queue_limit_size = 0) = 0;
  virtual void stop() noexcept = 0;

  core_warn_unused_result virtual size_t size() const noexcept = 0;

  class Tsr {
   public:
    inline Tsr(IThreadPool* p)
        : pool_(p)
        , data_(p->createThreadSpecificResource()) {}

    inline ~Tsr() {
      try {
        pool_->destroyThreadSpecificResource(data_);
      } catch (...) {
      }
    }

    inline operator void*() noexcept { return data_; }

   private:
    IThreadPool* pool_;
    void* data_;
  };

  virtual void* createThreadSpecificResource() { return nullptr; }

  virtual void destroyThreadSpecificResource(void* r) {
    if (r != nullptr) {
      core_assert(r == nullptr);
    }
  }

 private:
  IThread* doCreate() override;
};

class FakeThreadPool : public IThreadPool {
 public:
  core_warn_unused_result bool add(IObjectInQueue* obj) override {
    Tsr tsr(this);
    obj->process(tsr);
    return true;
  }

  void start(size_t, size_t = 0) override {}

  void stop() noexcept override {}

  core_warn_unused_result size_t size() const noexcept override { return 0; }
};

class ThreadPoolBase : public IThreadPool, public ThreadFactoryHolder {
 public:
  ThreadPoolBase(Params p = {});

 protected:
  Params params_;
};

class ThreadPool : public ThreadPoolBase {
 public:
  ThreadPool(Params p = {});
  ~ThreadPool() override;

  core_warn_unused_result bool add(IObjectInQueue* obj) override;

  void start(size_t thread_count, size_t queue_size_limit = 0) override;
  void stop() noexcept override;

  core_warn_unused_result size_t size() const noexcept override;
  core_warn_unused_result size_t threadCountExpected() const noexcept;
  core_warn_unused_result size_t threadCountReal() const noexcept;
  core_warn_unused_result size_t maxQueueSize() const noexcept;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class AdaptiveThreadPool : public ThreadPoolBase {
 public:
  AdaptiveThreadPool(Params params = {});
  ~AdaptiveThreadPool() override;

  void setMaxIdleTime(Duration interval);

  core_warn_unused_result bool add(IObjectInQueue* obj) override;

  void start(size_t thread_count = 0, size_t queue_size_limit = 0) override;
  void stop() noexcept override;

  core_warn_unused_result size_t size() const noexcept override;

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

class SimpleThreadPool : public ThreadPoolBase {
 public:
  SimpleThreadPool(Params params = {});
  ~SimpleThreadPool() override;

  core_warn_unused_result bool add(IObjectInQueue* obj) override;

  void start(size_t thr_num, size_t max_queque_size = 0) override;
  void stop() noexcept override;
  core_warn_unused_result size_t size() const noexcept override;

 private:
  std::unique_ptr<IThreadPool> slave_;
};

template <class QueueType, class Slave>
class ThreadPoolBinder : public QueueType {
 public:
  inline ThreadPoolBinder(Slave* slave)
      : slave_(slave) {}

  template <class... Args>
  inline ThreadPoolBinder(Slave* slave, Args&&... args)
      : QueueType(std::forward<Args>(args)...)
      , slave_(slave) {}

  inline ThreadPoolBinder(Slave& slave)
      : slave_(&slave) {}

  ~ThreadPoolBinder() override {
    try {
      this->stop();
    } catch (...) {
      // ¯\_(ツ)_/¯
    }
  }

  void* createThreadSpecificResource() override { return slave_->createThreadSpecificResource(); }

  void destroyThreadSpecificResource(void* resource) override { slave_->destroyThreadSpecificResource(resource); }

 private:
  Slave* slave_;
};

std::unique_ptr<IThreadPool> CreateThreadPool(size_t thread_count, size_t queue_size_limit = 0,
                                              IThreadPool::Params params = {});

}  // namespace core