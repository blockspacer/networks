#include "core/atomic.h"
#include "core/mutex.h"
#include "core/thread_pool.h"

#include "gtest/gtest.h"

struct SharedData {
  SharedData()
      : shared_counter(0)
      , failed(false) {}

  volatile uint32_t shared_counter;
  core::Mutex mutex;
  core::FakeMutex fake_mutex;

  bool failed;
};

class ThreadTask : public core::IObjectInQueue {
 public:
  using PFunc = void (ThreadTask::*)();

  ThreadTask(PFunc func, SharedData& data, size_t id)
      : func_(func)
      , data_(data)
      , id_(id) {}

  void process(void*) override {
    std::unique_ptr<ThreadTask> self(this);

    (this->*func_)();
  }

#define FAIL_ASSERT(cond) \
  if (!(cond)) {          \
    data_.failed = true;  \
  }

  void RunBasics() {
    data_.mutex.acquire();

    uint32_t old_counter = uint32_t(data_.shared_counter + id_);
    data_.shared_counter = old_counter;
    usleep(10 + random() % 10);
    FAIL_ASSERT(data_.shared_counter == old_counter);

    data_.mutex.release();
  }

  void RunFakeMutex() {
    bool res = data_.fake_mutex.tryAcquire();
    FAIL_ASSERT(res);
  }

  void RunRecursiveMutex() {
    for (size_t i = 0; i < id_ + 1; ++i) {
      data_.mutex.acquire();
      ++data_.shared_counter;
      usleep(1);
    }
    FAIL_ASSERT(data_.shared_counter == id_ + 1);

    bool res = data_.mutex.tryAcquire();
    FAIL_ASSERT(res);
    data_.mutex.release();

    for (size_t i = 0; i < id_; ++i) {
      --data_.shared_counter;
      data_.mutex.release();
    }
    FAIL_ASSERT(data_.shared_counter == 1);
    --data_.shared_counter;
    data_.mutex.release();
  }

#undef FAIL_ASSERT

 private:
  PFunc func_;
  SharedData& data_;
  size_t id_;
};

class MutexTest : public testing::Test {
 private:
#define RUN_CYCLE(what, count)                                        \
  q_.start(count);                                                    \
  for (size_t i = 0; i < count; ++i) {                                \
    ASSERT_TRUE(q_.add(new ThreadTask(&ThreadTask::what, data_, i))); \
  }                                                                   \
  q_.stop();                                                          \
  bool b = data_.failed;                                              \
  data_.failed = false;                                               \
  ASSERT_FALSE(b);

 protected:
  void TestBasics() {
    RUN_CYCLE(RunBasics, 5);

    ASSERT_EQ(data_.shared_counter, 10);
    data_.shared_counter = 0;
  }

  void TestFake() { RUN_CYCLE(RunFakeMutex, 3); }

  void TestRecursive() { RUN_CYCLE(RunRecursiveMutex, 4); }

 private:
  SharedData data_;
  core::ThreadPool q_;
};

TEST_F(MutexTest, TestBasics) { this->TestBasics(); }

TEST_F(MutexTest, TestFake) { this->TestFake(); }

TEST_F(MutexTest, TestRecursive) { this->TestRecursive(); }