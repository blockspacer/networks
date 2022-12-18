#include "core/atomic.h"
#include "core/condvar.h"
#include "core/guard.h"
#include "core/mutex.h"
#include "core/thread_pool.h"

#include "gtest/gtest.h"

using namespace core;

struct SharedData {
  SharedData()
      : stop_waiting(false)
      , in(0)
      , out(0)
      , waited(0)
      , failed(false) {}

  Mutex mutex;
  CondVar cond_var1;
  CondVar cond_var2;

  Atomic stop_waiting;

  Atomic in;
  Atomic out;

  Atomic waited;

  bool failed;
};

class ThreadTask : public IObjectInQueue {
 public:
  using PFunc = void (ThreadTask::*)();

  ThreadTask(PFunc func, size_t id, size_t totalIds, SharedData& data)
      : func_(func)
      , id_(id)
      , total_ids_(totalIds)
      , data_(data) {}

  void process(void*) override {
    std::unique_ptr<ThreadTask> self(this);

    (this->*func_)();
  }

#define FAIL_ASSERT(cond) \
  if (!(cond)) {          \
    data_.failed = true;  \
  }
  void RunBasics() {
    core_assert(total_ids_ == 3);

    if (id_ < 2) {
      Guarded<Mutex> guard(data_.mutex);
      while (!atomics::Load(data_.stop_waiting)) {
        bool res = data_.cond_var1.wait(data_.mutex, absl::Seconds(1));
        FAIL_ASSERT(res == true);
      }
    } else {
      usleep(100000);
      atomics::Store(data_.stop_waiting, true);

      Guarded<Mutex> guard(data_.mutex);
      data_.cond_var1.signal();
      data_.cond_var1.signal();
    }
  }

  void RunBasicsWithPredicate() {
    core_assert(total_ids_ == 3);

    if (id_ < 2) {
      Guarded<Mutex> guard(data_.mutex);
      const auto res =
          data_.cond_var1.wait(data_.mutex, absl::Seconds(1), [&] { return atomics::Load(data_.stop_waiting); });
      FAIL_ASSERT(res == true);
    } else {
      usleep(100000);
      atomics::Store(data_.stop_waiting, true);

      Guarded<Mutex> guard(data_.mutex);
      data_.cond_var1.signal();
      data_.cond_var1.signal();
    }
  }

  void RunSyncronize() {
    for (size_t i = 0; i < 10; ++i) {
      Guarded<Mutex> guard(data_.mutex);
      atomics::Increment(data_.in);
      if (atomics::Load(data_.in) == total_ids_) {
        atomics::Store(data_.out, 0);
        data_.cond_var1.broadCast();
      } else {
        atomics::Increment(data_.waited);
        while (atomics::Load(data_.in) < total_ids_) {
          bool res = data_.cond_var1.wait(data_.mutex, absl::Seconds(1));
          FAIL_ASSERT(res == true);
        }
      }

      atomics::Increment(data_.out);
      if (atomics::Load(data_.out) == total_ids_) {
        atomics::Store(data_.in, 0);
        data_.cond_var2.broadCast();
      } else {
        while (atomics::Load(data_.out) < total_ids_) {
          bool res = data_.cond_var2.wait(data_.mutex, absl::Seconds(1));
          FAIL_ASSERT(res == true);
        }
      }
    }

    FAIL_ASSERT(atomics::Load(data_.waited) == (total_ids_ - 1) * 10);
  }

  void RunSyncronizeWithPredicate() {
    for (size_t i = 0; i < 10; ++i) {
      Guarded<Mutex> guard(data_.mutex);
      atomics::Increment(data_.in);
      if (atomics::Load(data_.in) == total_ids_) {
        atomics::Store(data_.out, 0);
        data_.cond_var1.broadCast();
      } else {
        atomics::Increment(data_.waited);
        const auto res =
            data_.cond_var1.wait(data_.mutex, absl::Seconds(1), [&] { return atomics::Load(data_.in) >= total_ids_; });
        FAIL_ASSERT(res == true);
      }

      atomics::Increment(data_.out);
      if (atomics::Load(data_.out) == total_ids_) {
        atomics::Store(data_.in, 0);
        data_.cond_var2.broadCast();
      } else {
        const auto res =
            data_.cond_var2.wait(data_.mutex, absl::Seconds(1), [&] { return atomics::Load(data_.out) >= total_ids_; });
        FAIL_ASSERT(res == true);
      }
    }

    FAIL_ASSERT(data_.waited == (total_ids_ - 1) * 10);
  }
#undef FAIL_ASSERT

 private:
  PFunc func_;
  size_t id_;
  AtomicType total_ids_;
  SharedData& data_;
};

class CondVarTest : public testing::Test {
 private:
#define RUN_CYCLE(what, count)                                               \
  Q_.start(count);                                                           \
  for (size_t i = 0; i < count; ++i) {                                       \
    ASSERT_TRUE(Q_.add(new ThreadTask(&ThreadTask::what, i, count, Data_))); \
  }                                                                          \
  Q_.stop();                                                                 \
  bool b = Data_.failed;                                                     \
  Data_.failed = false;                                                      \
  ASSERT_FALSE(b);

 protected:
  inline void TestBasics() { RUN_CYCLE(RunBasics, 3); }

  inline void TestBasicsWithPredicate() { RUN_CYCLE(RunBasicsWithPredicate, 3); }

  inline void TestSyncronize() { RUN_CYCLE(RunSyncronize, 6); }

  inline void TestSyncronizeWithPredicate() { RUN_CYCLE(RunSyncronizeWithPredicate, 6); }

#undef RUN_CYCLE

 private:
  SharedData Data_;
  ThreadPool Q_;
};

TEST_F(CondVarTest, TestBasics) { this->TestBasics(); }

TEST_F(CondVarTest, TestBasicsWithPredicate) { this->TestBasicsWithPredicate(); }

TEST_F(CondVarTest, TestSyncronize) { this->TestSyncronize(); }

TEST_F(CondVarTest, TestSyncronizeWithPredicate) { this->TestSyncronizeWithPredicate(); }