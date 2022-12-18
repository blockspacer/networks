#include "core/thread_factory.h"
#include "core/thread_pool.h"

#include "gtest/gtest.h"

struct RunAble : public core::IThreadFactory::IThreadAble {
  inline RunAble()
      : done(false) {}

  ~RunAble() override = default;

  void doExecute() override { done = true; }

  bool done;
};

TEST(ThreadFactoryTest, TestSystemPool) {
  RunAble r;
  {
    std::unique_ptr<core::IThreadFactory::IThread> thr = core::SystemThreadFactory()->run(&r);
    thr->join();
  }
  ASSERT_TRUE(r.done);
}

TEST(ThreadFactoryTest, TestAdaptivePool) {
  RunAble r;
  {
    core::AdaptiveThreadPool pool;
    pool.start(0);
    std::unique_ptr<core::IThreadFactory::IThread> thr = pool.run(&r);
    thr->join();
  }
  ASSERT_TRUE(r.done);
}