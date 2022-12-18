#include "core/exception.h"
#include "core/noncopyable.h"
#include "core/sanitizers.h"
#include "core/thread.h"

#include "gtest/gtest.h"

#include <atomic>

struct IdTester {
  inline IdTester()
      : thr(nullptr)
      , cur(0)
      , real(0) {}

  static inline void* doRun(void* ptr) {
    ((IdTester*)ptr)->run();

    return nullptr;
  }

  inline void run() {
    cur = core::Thread::currentThreadId();
    real = thr->id();
  }

  core::Thread* thr;
  core::Thread::Id cur;
  core::Thread::Id real;
};

void* ThreadProc2(void*) {
  core::Thread::setCurrentThreadName("CurrentThreadSetNameTest");
  return nullptr;
}

void* ThreadProc(void*) { return nullptr; }

void* ThreadProc4(void*) {
  const std::string name = "ThreadName";
  core::Thread::setCurrentThreadName(name.c_str());

  const auto n = core::Thread::currentThreadName();

  EXPECT_EQ(name, n);

  return nullptr;
}

void* ThreadProcChild(void*) {
  const auto name = core::Thread::currentThreadName();
  const auto def = core::GetProgramName().substr(0, 15);
  EXPECT_EQ(name, def);
  return nullptr;
}

void* ThreadProcParent(void*) {
  const std::string name = "Parent";
  core::Thread::setCurrentThreadName(name.data());

  core::Thread thread(&ThreadProcChild, nullptr);

  thread.start();
  thread.join();

  const auto n = core::Thread::currentThreadName();
  EXPECT_EQ(n, name);
  return nullptr;
}

TEST(ThreadTest, TestId) {
  IdTester tst;
  core::Thread thr(tst.doRun, &tst);
  tst.thr = &thr;

  thr.start();
  thr.join();

  ASSERT_EQ(tst.cur, tst.real);
  ASSERT_TRUE(tst.cur != 0);
}

TEST(ThreadTest, TestDoubleJoin) {
  core::Thread ThreadTest(&ThreadProc, nullptr);

  ThreadTest.start();
  ThreadTest.join();

  ASSERT_EQ(ThreadTest.join(), nullptr);
}

TEST(ThreadTest, TestDoubleStart) {
  core::Thread ThreadTest(&ThreadProc, nullptr);

  ThreadTest.start();
  ASSERT_THROW(ThreadTest.start(), core::Exception);
  ThreadTest.join();
}

TEST(ThreadTest, TestNoStart) { core::Thread ThreadTest(&ThreadProc, nullptr); }

TEST(ThreadTest, TestNoStartJoin) {
  core::Thread ThreadTest(&ThreadProc, nullptr);

  ASSERT_EQ(ThreadTest.join(), nullptr);
}

TEST(ThreadTest, TestStackPointer) {
#if defined(core_thread_sanitizer)
  constexpr size_t stack_size = 400000;
#else
  constexpr size_t stack_size = 64000;
#endif
  std::unique_ptr<char[]> buf(new char[stack_size]);
  core::Thread thr(core::Thread::Params(ThreadProc, nullptr).setStackPointer(buf.get()).setStackSize(stack_size));

  thr.start();
  ASSERT_EQ(thr.join(), nullptr);
}

TEST(ThreadTest, TestFunc) {
  std::atomic_bool flag = {false};
  core::Thread ThreadTest([&flag]() { flag = true; });

  ThreadTest.start();
  ASSERT_EQ(ThreadTest.join(), nullptr);
  ASSERT_TRUE(flag);
}

TEST(ThreadTest, TestCopyFunc) {
  std::atomic_bool flag = {false};
  auto func = [&flag]() { flag = true; };

  core::Thread ThreadTest(func);
  ThreadTest.start();
  ASSERT_EQ(ThreadTest.join(), nullptr);

  core::Thread ThreadTest2(func);
  ThreadTest2.start();
  ASSERT_EQ(ThreadTest2.join(), nullptr);

  ASSERT_TRUE(flag);
}

TEST(ThreadTest, TestCallable) {
  std::atomic_bool flag = {false};

  struct Callable : core::MoveOnly {
    std::atomic_bool* flag_;

    Callable(std::atomic_bool* flag)
        : flag_(flag) {}

    void operator()() { *flag_ = true; }
  };

  Callable foo(&flag);
  core::Thread ThreadTest(std::move(foo));

  ThreadTest.start();
  ASSERT_EQ(ThreadTest.join(), nullptr);
  ASSERT_TRUE(flag);
}

TEST(ThreadTest, TestSetThreadName) {
  core::Thread thread(&ThreadProc2, nullptr);
  thread.start();
  thread.join();
}

TEST(ThreadTest, TestSetThreadName2) {
  core::Thread thread(core::Thread::Params(&ThreadProc2, nullptr, 0).setName("XXX"));
  thread.start();
  thread.join();
}

TEST(ThreadTest, TestSetGetThreadName) {
  core::Thread thread(&ThreadProc4, nullptr);
  thread.start();
  thread.join();
}

TEST(ThreadTest, TestSetGetThreadNameInChildThread) {
  core::Thread thread(&ThreadProcParent, nullptr);
  thread.start();
  thread.join();
}
