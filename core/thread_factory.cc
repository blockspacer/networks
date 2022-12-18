#include "thread_factory.h"
#include "singleton.h"
#include "thread.h"

#include <memory>

using IThread = core::IThreadFactory::IThread;

namespace {

class SystemThreadFactory : public core::IThreadFactory {
 public:
  class PoolThread : public IThread {
   public:
    ~PoolThread() override {
      if (thread_) {
        thread_->detach();
      }
    }

    void doRun(IThreadAble* func) override {
      thread_ = std::make_unique<core::Thread>(threadProc, func);
      thread_->start();
    }

    void doJoin() noexcept override {
      if (!thread_) {
        return;
      }

      thread_->join();
      thread_.reset();
    }

   private:
    static void* threadProc(void* func) {
      ((IThreadAble*)func)->execute();
      return nullptr;
    }

   private:
    std::unique_ptr<core::Thread> thread_;
  };

  inline SystemThreadFactory() noexcept = default;

  IThread* doCreate() override { return new PoolThread; }
};

class ThreadFactoryFuncObj : public core::IThreadFactory::IThreadAble {
 public:
  ThreadFactoryFuncObj(std::function<void()> func)
      : func_(std::move(func)) {}

  void doExecute() override {
    std::unique_ptr<ThreadFactoryFuncObj> self(this);
    func_();
  }

 private:
  std::function<void()> func_;
};

}  // namespace

std::unique_ptr<IThread> core::IThreadFactory::run(std::function<void()> func) {
  std::unique_ptr<IThread> ret(doCreate());
  ret->run(new ::ThreadFactoryFuncObj(std::move(func)));
  return ret;
}

static core::IThreadFactory* system_factory = nullptr;

static core::IThreadFactory* SystemThreadFactoryImpl() { return core::Singleton<::SystemThreadFactory>(); }

core::IThreadFactory* core::SystemThreadFactory() {
  if (system_factory) {
    return system_factory;
  }
  return SystemThreadFactoryImpl();
}

void core::SetSystemThreadFactory(IThreadFactory* factory) { system_factory = factory; }