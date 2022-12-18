#include "core/future.h"

#include "gtest/gtest.h"

#include <list>

using namespace core;

class CopyCounter {
 public:
  CopyCounter(size_t* num_copies)
      : num_copies_(num_copies) {}

  CopyCounter(const CopyCounter& that)
      : num_copies_(that.num_copies_) {
    ++*num_copies_;
  }

  CopyCounter& operator=(const CopyCounter& that) {
    num_copies_ = that.num_copies_;
    ++*num_copies_;
    return *this;
  }

  CopyCounter(CopyCounter&& that) = default;

  CopyCounter& operator=(CopyCounter&& that) = default;

 private:
  size_t* num_copies_ = nullptr;
};

template <class T>
auto MakePromise() {
  if constexpr (std::is_same_v<T, void>) {
    return NewPromise();
  }
  return NewPromise<T>();
}

template <class T>
void TestFutureStateId() {
  Future<T> empty;
  ASSERT_FALSE(empty.stateId().has_value());
  auto promise1 = MakePromise<T>();
  auto future11 = promise1.getFuture();
  ASSERT_TRUE(future11.stateId().has_value());
  auto future12 = promise1.getFuture();
  ASSERT_EQ(future11.stateId(), future11.stateId());
  ASSERT_EQ(future11.stateId(), future12.stateId());
  auto promise2 = MakePromise<T>();
  auto future2 = promise2.getFuture();
  ASSERT_TRUE(future2.stateId().has_value());
  ASSERT_NE(future11.stateId(), future2.stateId());
}

struct TestCallback {
  int value;

  TestCallback(int value)
      : value(value) {}

  void Callback(const Future<int>& future) { value += future.getValue(); }

  int Func(const Future<int>& future) { return (value += future.getValue()); }

  void VoidFunc(const Future<int>& future) { future.getValue(); }

  Future<int> FutureFunc(const Future<int>& future) { return MakeFuture(value += future.getValue()); }

  Promise<void> signal = NewPromise();
  Future<void> FutureVoidFunc(const Future<int>& future) {
    future.getValue();
    return signal;
  }
};

template <class T>
void TestApplyNoRvalueCopyImpl() {
  size_t num_copies = 0;
  CopyCounter copy_counter(&num_copies);

  auto promise = MakePromise<T>();

  const auto future = promise.getFuture().apply([copyCounter = std::move(copy_counter)](const auto&) {});

  if constexpr (std::is_same_v<T, void>) {
    promise.setValue();
  } else {
    promise.setValue(T());
  }

  future.getValueSync();

  ASSERT_EQ(num_copies, 0);
}

template <class T>
void TestApplyLvalueCopyImpl() {
  size_t num_copies = 0;
  CopyCounter copy_counter(&num_copies);

  auto promise = MakePromise<T>();

  auto func = [copyCounter = std::move(copy_counter)](const auto&) {};
  const auto future = promise.getFuture().apply(func);

  if constexpr (std::is_same_v<T, void>) {
    promise.setValue();
  } else {
    promise.setValue(T());
  }

  future.getValueSync();

  ASSERT_EQ(num_copies, 1);
}

class CustomException : public Exception {};

TEST(FutureTest, ShouldInitiallyHasNoValue) {
  Promise<int> promise;
  ASSERT_FALSE(promise.hasValue());

  promise = NewPromise<int>();
  ASSERT_FALSE(promise.hasValue());

  Future<int> future;
  ASSERT_FALSE(future.hasValue());

  future = promise.getFuture();
  ASSERT_FALSE(future.hasValue());
}

TEST(FutureTest, ShouldInitiallyHasNoValueVoid) {
  Promise<void> promise;
  ASSERT_FALSE(promise.hasValue());

  promise = NewPromise();
  ASSERT_FALSE(promise.hasValue());

  Future<void> future;
  ASSERT_FALSE(future.hasValue());

  future = promise.getFuture();
  ASSERT_FALSE(future.hasValue());
}

TEST(FutureTest, ShouldStoreValue) {
  Promise<int> promise = NewPromise<int>();
  promise.setValue(123);
  ASSERT_TRUE(promise.hasValue());
  ASSERT_EQ(promise.getValue(), 123);

  Future<int> future = promise.getFuture();
  ASSERT_TRUE(future.hasValue());
  ASSERT_EQ(future.getValue(), 123);

  future = MakeFuture(345);
  ASSERT_TRUE(future.hasValue());
  ASSERT_EQ(future.getValue(), 345);
}

TEST(FutureTest, ShouldStoreValueVoid) {
  Promise<void> promise = NewPromise();
  promise.setValue();
  ASSERT_TRUE(promise.hasValue());

  Future<void> future = promise.getFuture();
  ASSERT_TRUE(future.hasValue());

  future = MakeFuture();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldInvokeCallback) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<int> future = promise.getFuture().subscribe([&](const Future<int>& fut) { return callback.Callback(fut); });

  promise.setValue(456);
  ASSERT_EQ(future.getValue(), 456);
  ASSERT_EQ(callback.value, 123 + 456);
}

TEST(FutureTest, ShouldApplyFunc) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<int> future = promise.getFuture().apply([&](const auto& fut) { return callback.Func(fut); });

  promise.setValue(456);
  ASSERT_EQ(future.getValue(), 123 + 456);
  ASSERT_EQ(callback.value, 123 + 456);
}

TEST(FutureTest, ShouldApplyVoidFunc) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<void> future = promise.getFuture().apply([&](const auto& fut) { return callback.VoidFunc(fut); });

  promise.setValue(456);
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldApplyFutureFunc) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<int> future = promise.getFuture().apply([&](const auto& fut) { return callback.FutureFunc(fut); });

  promise.setValue(456);
  ASSERT_EQ(future.getValue(), 123 + 456);
  ASSERT_EQ(callback.value, 123 + 456);
}

TEST(FutureTest, ShouldApplyFutureVoidFunc) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<void> future = promise.getFuture().apply([&](const auto& fut) { return callback.FutureVoidFunc(fut); });

  promise.setValue(456);
  ASSERT_FALSE(future.hasValue());

  callback.signal.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldIgnoreResultIfAsked) {
  Promise<int> promise = NewPromise<int>();

  TestCallback callback(123);
  Future<int> future = promise.getFuture().ignoreResult().withValue(42);

  promise.setValue(456);
  ASSERT_EQ(future.getValue(), 42);
}

TEST(FutureTest, ShouldRethrowException) {
  Promise<int> promise = NewPromise<int>();
  try {
    core_throw CustomException();
  } catch (...) {
    promise.setException(std::current_exception());
  }

  ASSERT_FALSE(promise.hasValue());
  ASSERT_TRUE(promise.hasException());
  ASSERT_THROW(promise.getValue(), CustomException);
  ASSERT_THROW(promise.tryRethrow(), CustomException);
}

TEST(FutureTest, ShouldRethrowCallbackException) {
  Promise<int> promise = NewPromise<int>();
  Future<int> future = promise.getFuture();
  future.subscribe([](const Future<int>&) { core_throw CustomException(); });

  ASSERT_THROW(promise.setValue(123), CustomException);
}

TEST(FutureTest, ShouldRethrowCallbackExceptionIgnoreResult) {
  Promise<int> promise = NewPromise<int>();
  Future<void> future = promise.getFuture().ignoreResult();
  future.subscribe([](const Future<void>&) { core_throw CustomException(); });

  ASSERT_THROW(promise.setValue(123), CustomException);
}

TEST(FutureTest, ShouldWaitExceptionOrAll) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  Future<void> future = WaitExceptionOrAll(promise1, promise2);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_FALSE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitExceptionOrAllVector) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  std::vector<Future<void>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitExceptionOrAll(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_FALSE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitExceptionOrAllVectorWithValueType) {
  Promise<int> promise1 = NewPromise<int>();
  Promise<int> promise2 = NewPromise<int>();

  std::vector<Future<int>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitExceptionOrAll(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue(0);
  ASSERT_FALSE(future.hasValue());

  promise2.setValue(0);
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitExceptionOrAllList) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  std::list<Future<void>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitExceptionOrAll(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_FALSE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitExceptionOrAllVectorEmpty) {
  std::vector<Future<void>> promises;

  Future<void> future = WaitExceptionOrAll(promises);
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitAnyVector) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  std::vector<Future<void>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitAny(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_TRUE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitAnyVectorWithValueType) {
  Promise<int> promise1 = NewPromise<int>();
  Promise<int> promise2 = NewPromise<int>();

  std::vector<Future<int>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitAny(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue(0);
  ASSERT_TRUE(future.hasValue());

  promise2.setValue(0);
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitAnyList) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  std::list<Future<void>> promises;
  promises.push_back(promise1);
  promises.push_back(promise2);

  Future<void> future = WaitAny(promises);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_TRUE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitAnyVectorEmpty) {
  std::vector<Future<void>> promises;

  Future<void> future = WaitAny(promises);
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldWaitAny) {
  Promise<void> promise1 = NewPromise();
  Promise<void> promise2 = NewPromise();

  Future<void> future = WaitAny(promise1, promise2);
  ASSERT_FALSE(future.hasValue());

  promise1.setValue();
  ASSERT_TRUE(future.hasValue());

  promise2.setValue();
  ASSERT_TRUE(future.hasValue());
}

TEST(FutureTest, ShouldStoreTypesWithoutDefaultConstructor) {
  struct Rec {
    explicit Rec(int) {}
  };

  auto promise = NewPromise<Rec>();
  promise.setValue(Rec(1));

  auto future = MakeFuture(Rec(1));
  const auto& rec = future.getValue();
  core_ignore_result(rec);
}

TEST(FutureTest, ShouldStoreMovableTypes) {
  struct Rec : MoveOnly {
    explicit Rec(int) {}
  };

  auto promise = NewPromise<Rec>();
  promise.setValue(Rec(1));

  auto future = MakeFuture(Rec(1));
  const auto& rec = future.getValue();
  core_ignore_result(rec);
}

TEST(FutureTest, ShouldMoveMovableTypes) {
  struct Rec : MoveOnly {
    explicit Rec(int) {}
  };

  auto promise = NewPromise<Rec>();
  promise.setValue(Rec(1));

  auto future = MakeFuture(Rec(1));
  auto rec = future.extractValue();
  core_ignore_result(rec);
}

TEST(FutureTest, ShouldNotExtractAfterGet) {
  Promise<int> promise = NewPromise<int>();
  promise.setValue(123);
  ASSERT_TRUE(promise.hasValue());
  ASSERT_EQ(promise.getValue(), 123);
  ASSERT_THROW(promise.extractValue(), FutureException);
}

TEST(FutureTest, ShouldNotGetAfterExtract) {
  Promise<int> promise = NewPromise<int>();
  promise.setValue(123);
  ASSERT_TRUE(promise.hasValue());
  ASSERT_EQ(promise.extractValue(), 123);
  ASSERT_THROW(promise.getValue(), FutureException);
}

TEST(FutureTest, ShouldNotExtractAfterExtract) {
  Promise<int> promise = NewPromise<int>();
  promise.setValue(123);
  ASSERT_TRUE(promise.hasValue());
  ASSERT_EQ(promise.extractValue(), 123);
  ASSERT_THROW(promise.extractValue(), FutureException);
}

TEST(FutureTest, HandlingRepetitiveSet) {
  Promise<int> promise = NewPromise<int>();
  promise.setValue(42);
  ASSERT_THROW(promise.setValue(42), FutureException);
}

TEST(FutureTest, HandlingRepetitiveTrySet) {
  Promise<int> promise = NewPromise<int>();
  ASSERT_TRUE(promise.trySetValue(42));
  ASSERT_FALSE(promise.trySetValue(42));
}

TEST(FutureTest, HandlingRepetitiveSetException) {
  Promise<int> promise = NewPromise<int>();
  promise.setException("test");
  ASSERT_THROW(promise.setException("test"), FutureException);
}

TEST(FutureTest, HandlingRepetitiveTrySetException) {
  Promise<int> promise = NewPromise<int>();
  ASSERT_TRUE(promise.trySetException(std::make_exception_ptr("test")));
  ASSERT_FALSE(promise.trySetException(std::make_exception_ptr("test")));
}

TEST(FutureTest, ShouldAllowToMakeFutureWithException) {
  auto future1 = MakeErrorFuture<void>(std::make_exception_ptr(FutureException()));
  ASSERT_TRUE(future1.hasException());
  ASSERT_THROW(future1.getValue(), FutureException);

  auto future2 = MakeErrorFuture<int>(std::make_exception_ptr(FutureException()));
  ASSERT_TRUE(future2.hasException());
  ASSERT_THROW(future2.getValue(), FutureException);

  auto future3 = MakeFuture<std::exception_ptr>(std::make_exception_ptr(FutureException()));
  ASSERT_TRUE(future3.hasValue());
  ASSERT_NO_THROW(future3.getValue());

  auto future4 = MakeFuture<std::unique_ptr<int>>(nullptr);
  ASSERT_TRUE(future4.hasValue());
  ASSERT_NO_THROW(future4.getValue());
}

TEST(FutureTest, WaitAllowsExtract) {
  auto future = MakeFuture<int>(42);
  std::vector vec{future, future, future};
  WaitExceptionOrAll(vec).getValue();
  WaitAny(vec).getValue();

  ASSERT_EQ(future.extractValue(), 42);
}

TEST(FutureTest, IgnoreAllowsExtract) {
  auto future = MakeFuture<int>(42);
  future.ignoreResult().getValue();

  ASSERT_EQ(future.extractValue(), 42);
}

TEST(FutureTest, WaitExceptionOrAllException) {
  auto promise1 = NewPromise();
  auto promise2 = NewPromise();
  auto future1 = promise1.getFuture();
  auto future2 = promise2.getFuture();
  auto wait = WaitExceptionOrAll(future1, future2);
  promise2.setException("foo-exception");
  wait.wait();
  ASSERT_TRUE(future2.hasException());
  ASSERT_TRUE(!future1.hasValue() && !future1.hasException());
}

TEST(FutureTest, WaitAllException) {
  auto promise1 = NewPromise();
  auto promise2 = NewPromise();
  auto future1 = promise1.getFuture();
  auto future2 = promise2.getFuture();
  auto wait = WaitAll(future1, future2);
  promise2.setException("foo-exception");
  ASSERT_FALSE(wait.hasValue() && !wait.hasException());
  promise1.setValue();
  ASSERT_THROW(wait.getValueSync(), Exception);
}

TEST(FutureTest, FutureStateId) {
  TestFutureStateId<void>();
  TestFutureStateId<int>();
}

TEST(FutureTest, ApplyNoRvalueCopy) {
  TestApplyNoRvalueCopyImpl<void>();
  TestApplyNoRvalueCopyImpl<int>();
}

TEST(FutureTest, ApplyLvalueCopy) {
  TestApplyLvalueCopyImpl<void>();
  TestApplyLvalueCopyImpl<int>();
}