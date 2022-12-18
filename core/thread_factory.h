#pragma once

#include "fwd.h"

#include <memory>

namespace core {

class IThreadFactory {
 public:
  class IThreadAble {
   public:
    inline IThreadAble() noexcept = default;

    virtual ~IThreadAble() = default;

    inline void execute() { doExecute(); }

   private:
    virtual void doExecute() = 0;
  };

  class IThread {
    friend class IThreadFactory;

   public:
    inline IThread() noexcept = default;

    virtual ~IThread() = default;

    inline void join() noexcept { doJoin(); }

   private:
    inline void run(IThreadAble* func) { doRun(func); }

    virtual void doRun(IThreadAble* func) = 0;
    virtual void doJoin() noexcept = 0;
  };

  inline IThreadFactory() noexcept = default;

  virtual ~IThreadFactory() = default;

  inline std::unique_ptr<IThread> run(IThreadAble* func) {
    std::unique_ptr<IThread> ret(doCreate());
    ret->run(func);
    return ret;
  }

  std::unique_ptr<IThread> run(std::function<void()> func);

 private:
  virtual IThread* doCreate() = 0;
};

IThreadFactory* SystemThreadFactory();
void SetSystemThreadFactory(IThreadFactory* factory);

}  // namespace core