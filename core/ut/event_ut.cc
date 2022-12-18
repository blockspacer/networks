#include "core/atomic.h"
#include "core/event.h"
#include "core/thread_pool.h"

#include "gtest/gtest.h"

struct SharedData {
  SharedData()
      : counter(0)
      , failed(false) {}

  core::Atomic counter;
  core::ManualEvent event;
  bool failed;
};

struct ThreadTask : public core::IObjectInQueue {
 public:
  ThreadTask(SharedData& data, size_t id)
      : data_(data)
      , id_(id) {}

  void process(void*) override {
    std::unique_ptr<ThreadTask> self(this);

    if (id_ == 0) {
      usleep(100);
      bool cond = data_.counter == 0;
      if (!cond) {
        data_.failed = true;
      }
      data_.event.signal();
    } else {
      while (!data_.event.wait(absl::Seconds(100))) {
      }
      core::atomics::Add(data_.counter, id_);
    }
  }

 private:
  SharedData& data_;
  size_t id_;
};

class SignalTask : public core::IObjectInQueue {
 private:
  core::ManualEvent& barrier;
  core::ManualEvent& ev;

 public:
  SignalTask(core::ManualEvent& barrier, core::ManualEvent& ev)
      : barrier(barrier)
      , ev(ev) {}

  void process(void*) override {
    core_ignore_result(barrier);
    ev.signal();
  }
};

class OwnerTask : public core::IObjectInQueue {
 public:
  core::ManualEvent barrier;
  std::unique_ptr<core::ManualEvent> ev;

 public:
  OwnerTask()
      : ev(new core::ManualEvent) {}

  void process(void*) override {
    ev->wait();
    ev.reset();
  }
};

TEST(EventTest, WaitAndSignalTest) {
  SharedData data;
  core::ThreadPool queue;
  queue.start(5);
  for (size_t i = 0; i < 5; ++i) {
    ASSERT_TRUE(queue.add(new ThreadTask(data, i)));
  }
  queue.stop();
  ASSERT_EQ(data.counter, 10);
  ASSERT_FALSE(data.failed);
}

TEST(EventTest, ConcurrentSignalAndWaitTest) {
  const size_t limit = 200;
  core::ManualEvent event[limit];
  core::ManualEvent barrier;
  core::ThreadPool queue;
  queue.start(limit);
  std::vector<std::unique_ptr<core::IObjectInQueue>> tasks;
  for (auto& i : event) {
    tasks.emplace_back(std::make_unique<SignalTask>(barrier, i));
    ASSERT_TRUE(queue.add(tasks.back().get()));
  }
  for (size_t i = limit; i != 0; --i) {
    ASSERT_TRUE(event[i - 1].wait(absl::Seconds(90)));
  }
  queue.stop();
}

TEST(EventTest, DestructorBeforeSignalFinishTest) {
  std::vector<std::unique_ptr<core::IObjectInQueue>> tasks;
  for (size_t i = 0; i < 1000; ++i) {
    auto owner = std::make_unique<OwnerTask>();
    tasks.emplace_back(std::make_unique<SignalTask>(owner->barrier, *owner->ev));
    tasks.emplace_back(std::move(owner));
  }

  core::ThreadPool queue;
  queue.start(4);
  for (auto& task : tasks) {
    ASSERT_TRUE(queue.add(task.get()));
  }
  queue.stop();
}