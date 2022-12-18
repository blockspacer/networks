#include "core/condvar.h"
#include "core/mutex.h"
#include "core/spinlock.h"
#include "core/thread.h"
#include "core/thread_pool.h"

#include "gtest/gtest.h"

#include <random>
#include <unordered_set>

struct ThreadPoolTest {
  core::SpinLock lock;
  long r = -1;

  struct Task : public core::IObjectInQueue {
    ThreadPoolTest* test = nullptr;
    long value = 0;

    Task(ThreadPoolTest* test, int value)
        : test(test)
        , value(value) {}

    void process(void*) override {
      std::unique_ptr<Task> self(this);

      core::Guarded<core::SpinLock> guard(test->lock);
      test->r -= value;
    }
  };

  struct OwnedTask : public core::IObjectInQueue {
    bool& processed;
    bool& destructed;

    OwnedTask(bool& processed, bool& destructed)
        : processed(processed)
        , destructed(destructed) {}

    ~OwnedTask() override { destructed = true; }

    void process(void*) override { processed = true; }
  };

  inline void TestAnyQueue(core::IThreadPool* queue, size_t queueSize = 1000) {
    const size_t cnt = 1000;
    std::vector<long> rands(cnt);

    for (size_t i = 0; i < cnt; ++i) {
      rands[i] = (long)i;
    }

    r = 0;

    for (size_t i = 0; i < cnt; ++i) {
      r += rands[i];
    }

    queue->start(10, queueSize);

    for (size_t i = 0; i < cnt; ++i) {
      ASSERT_TRUE(queue->add(new Task(this, rands[i])));
    }

    queue->stop();

    ASSERT_EQ(0, r);
  }
};

class FailAddQueue : public core::IThreadPool {
 public:
  core_warn_unused_result auto add(core::IObjectInQueue* /*obj*/) -> bool override { return false; }

  void start(size_t, size_t) override {}

  void stop() noexcept override {}

  core_warn_unused_result auto size() const noexcept -> size_t override { return 0; }
};

TEST(ThreadPoolTest, TestThreadPool) {
  ThreadPoolTest t;
  core::ThreadPool q;
  t.TestAnyQueue(&q);
}

TEST(ThreadPoolTest, TestThreadPoolBlocking) {
  ThreadPoolTest t;
  core::ThreadPool q(core::ThreadPool::Params().setBlocking(true));
  t.TestAnyQueue(&q, 100);
}

TEST(ThreadPoolTest, TestAdaptivePool) {
  ThreadPoolTest t;
  core::AdaptiveThreadPool q;
  t.TestAnyQueue(&q);
}

TEST(ThreadPoolTest, TestAddAndOwn) {
  core::ThreadPool q;
  q.start(2);
  bool processed = false;
  bool destructed = false;
  q.safeAddAndOwn(std::make_unique<ThreadPoolTest::OwnedTask>(processed, destructed));
  q.stop();

  ASSERT_TRUE(processed) << "Not processed";
  ASSERT_TRUE(destructed) << "Not destructed";
}

TEST(ThreadPoolTest, TestAddFunc) {
  FailAddQueue queue;
  bool added = queue.addFunc([]() {});
  ASSERT_EQ(added, false);
}

TEST(ThreadPoolTest, TestSafeAddFuncThrows) {
  FailAddQueue queue;
  ASSERT_THROW(queue.safeAddFunc([] {}), core::ThreadPoolException);
}

TEST(ThreadPoolTest, TestFunctionNotCopied) {
  struct FailOnCopy {
    FailOnCopy() {}

    FailOnCopy(FailOnCopy&&) {}

    FailOnCopy(const FailOnCopy&) { EXPECT_TRUE(false) << "Don't copy std::function inside ThreadPool"; }
  };

  core::ThreadPool queue(core::ThreadPool::Params().setBlocking(false).setBlocking(true));
  queue.start(2);

  queue.safeAddFunc([data = FailOnCopy()]() {});

  queue.stop();
}

TEST(ThreadPoolTest, TestInfoGetters) {
  core::ThreadPool queue;

  queue.start(2, 7);

  ASSERT_EQ(queue.threadCountExpected(), 2);
  ASSERT_EQ(queue.threadCountReal(), 2);
  ASSERT_EQ(queue.maxQueueSize(), 7);

  queue.stop();

  queue.start(4, 1);

  ASSERT_EQ(queue.threadCountExpected(), 4);
  ASSERT_EQ(queue.threadCountReal(), 4);
  ASSERT_EQ(queue.maxQueueSize(), 1);

  queue.stop();
}

void TestFixedThreadNameImpl(core::IThreadPool& pool, const std::string& expected_name) {
  pool.start(1);
  std::string name;
  pool.safeAddFunc([&name]() { name = core::Thread::currentThreadName(); });
  pool.stop();
  ASSERT_EQ(name, expected_name);
  ASSERT_NE(core::Thread::currentThreadName(), expected_name);
}

TEST(ThreadPoolTest, TestFixedThreadName) {
  const std::string expected_name = "HelloWorld";
  {
    core::ThreadPool pool(core::ThreadPool::Params().setBlocking(true).setBlocking(false).setThreadName(expected_name));
    TestFixedThreadNameImpl(pool, expected_name);
  }
  {
    core::AdaptiveThreadPool pool(core::ThreadPool::Params().setThreadName(expected_name));
    TestFixedThreadNameImpl(pool, expected_name);
  }
}

void TestEnumeratedThreadNameImpl(core::IThreadPool& pool, const std::unordered_set<std::string>& expected_names) {
  pool.start(expected_names.size());
  core::Mutex lock;
  core::CondVar all_ready;
  size_t ready_count = 0;
  std::unordered_set<std::string> names;
  for (size_t i = 0; i < expected_names.size(); ++i) {
    pool.safeAddFunc([&]() {
      core_with_lock(lock) {
        if (++ready_count == expected_names.size()) {
          all_ready.broadCast();
        } else {
          while (ready_count != expected_names.size()) {
            all_ready.wait(lock);
          }
        }
        names.insert(core::Thread::currentThreadName());
      }
    });
  }
  pool.stop();
  ASSERT_EQ(names, expected_names);
}

TEST(ThreadPoolTest, TestEnumerateThreadName) {
  const std::string prefix = "HelloWorld";
  const std::unordered_set<std::string> expected_names = {"HelloWorld0", "HelloWorld1", "HelloWorld2", "HelloWorld3",
                                                          "HelloWorld4", "HelloWorld5", "HelloWorld6", "HelloWorld7",
                                                          "HelloWorld8", "HelloWorld9", "HelloWorld10"};

  {
    core::ThreadPool pool(core::ThreadPool::Params().setBlocking(true).setBlocking(false).setThreadNamePrefix(prefix));
    TestEnumeratedThreadNameImpl(pool, expected_names);
  }
  {
    core::AdaptiveThreadPool pool(core::ThreadPool::Params().setThreadNamePrefix(prefix));
    TestEnumeratedThreadNameImpl(pool, expected_names);
  }
}