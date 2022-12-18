#include "core/async.h"

#include "gtest/gtest.h"

#include <vector>

struct MySuperTaskQueue {
};

namespace core {

template <class Func>
auto Async(Func func, MySuperTaskQueue&) {
  return MakeFuture(func());
}

}  // namespace core

TEST(AsyncTest, TestExtension) {
  MySuperTaskQueue queue;
  auto future = core::Async([]() { return 5; }, queue);
  future.wait();
  ASSERT_EQ(future.getValue(), 5);
}

TEST(AsyncTest, TestWorksWithThreadPool) {
  auto pool = std::make_unique<core::ThreadPool>();
  pool->start(1);

  auto future = core::Async([]() { return 5; }, *pool);
  future.wait();
  ASSERT_EQ(future.getValue(), 5);
}

TEST(AsyncTest, TestProperlyDeducesFutureType) {
  auto pool = core::CreateThreadPool(1);

  core::Future<void> f1 = core::Async([]() {}, *pool);
  core::Future<int> f2 = core::Async([]() { return 5; }, *pool);
  core::Future<double> f3 = core::Async([]() { return 5.0; }, *pool);
  core::Future<std::vector<int>> f4 = core::Async([]() { return std::vector<int>(); }, *pool);
  core::Future<int> f5 = core::Async([]() { return core::MakeFuture(5); }, *pool);
}