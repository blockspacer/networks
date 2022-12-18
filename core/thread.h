#pragma once

#include "macro.h"
#include "progname.h"

#include <memory>
#include <string>

namespace core {

class Thread {
  template <class Callable>
  class CallableParams;
  struct PrivateCtor {};

 public:
  using ThreadProc = void* (*)(void*);
  using Id = size_t;

  struct Params {
    inline Params()
        : proc(nullptr)
        , data(nullptr)
        , stack_size(0)
        , stack_pointer(nullptr) {}

    inline Params(ThreadProc p, void* d)
        : proc(p)
        , data(d)
        , stack_size(0)
        , stack_pointer(nullptr) {}

    inline Params(ThreadProc p, void* d, size_t s)
        : proc(p)
        , data(d)
        , stack_size(s)
        , stack_pointer(nullptr) {}

    inline Params& setName(const std::string& n) noexcept {
      name = n;
      return *this;
    }

    inline Params& setStackSize(size_t size) noexcept {
      stack_size = size;
      return *this;
    }

    inline Params& setStackPointer(void* ptr) noexcept {
      stack_pointer = ptr;
      return *this;
    }

    ThreadProc proc;
    void* data;
    size_t stack_size;
    void* stack_pointer;
    std::string name = GetProgramName();
  };

  Thread(const Params& params);
  Thread(ThreadProc proc, void* param);

  template <class Callable>
  Thread(Callable&& callable)
      : Thread(PrivateCtor{}, std::make_unique<CallableParams<Callable>>(std::forward<Callable>(callable))) {}

  Thread(Params&& params)
      : Thread((const Params&)params) {}

  Thread(Params& params)
      : Thread((const Params&)params) {}

  ~Thread();

  void start();
  void* join();
  void detach();
  core_warn_unused_result bool isRunning() const noexcept;
  core_warn_unused_result Id id() const noexcept;

  static Id impossibleThreadId() noexcept;
  static Id currentThreadId() noexcept;

  static void setCurrentThreadName(const char* name) noexcept;
  static std::string currentThreadName() noexcept;

 private:
  struct CallableBase {
    virtual ~CallableBase() = default;
    virtual void run() = 0;

    static void* threadWorker(void* arg) {
      static_cast<CallableBase*>(arg)->run();
      return nullptr;
    }
  };

  template <class Callable>
  class CallableParams : public CallableBase {
   public:
    CallableParams(Callable&& callable)
        : c(std::forward<Callable>(callable)) {}

    void run() override { c(); }

   private:
    Callable c;
  };

  Thread(PrivateCtor, std::unique_ptr<CallableBase> callable);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

struct ISimpleThread : public Thread {
  ISimpleThread(size_t stack_size = 0);
  virtual ~ISimpleThread() = default;
  virtual void* threadProc() = 0;
};

}  // namespace core