#pragma once

#include "atomic.h"
#include "datetime.h"
#include "event.h"
#include "exception.h"
#include "fwd.h"
#include "guard.h"
#include "intrusive_ptr.h"
#include "macro.h"
#include "singleton.h"
#include "spinlock.h"

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>

namespace core {

struct FutureException : public Exception {};

template <class T>
Promise<T> NewPromise();
Promise<void> NewPromise();

template <class T>
Future<T> MakeFuture(const T& value);
template <class T>
Future<std::remove_reference_t<T>> MakeFuture(T&& value);
template <class T>
Future<T> MakeFuture();
template <class T>
Future<T> MakeErrorFuture(std::exception_ptr exception);
Future<void> MakeFuture();

core_warn_unused_result Future<void> WaitAll(const Future<void>& f1);
core_warn_unused_result Future<void> WaitAll(const Future<void>& f1, const Future<void>& f2);
template <class Container>
core_warn_unused_result Future<void> WaitAll(const Container& futures);

core_warn_unused_result Future<void> WaitExceptionOrAll(const Future<void>& f1);
core_warn_unused_result Future<void> WaitExceptionOrAll(const Future<void>& f1, const Future<void>& f2);
template <class Container>
core_warn_unused_result Future<void> WaitExceptionOrAll(const Container& futures);

core_warn_unused_result Future<void> WaitAny(const Future<void>& f1);
core_warn_unused_result Future<void> WaitAny(const Future<void>& f1, const Future<void>& f2);
template <class Container>
core_warn_unused_result Future<void> WaitAny(const Container& futures);

namespace detail {

template <class T>
class FutureState;

template <class T>
struct FutureType {
  using Type = T;
};

template <class T>
struct FutureType<Future<T>> {
  using Type = typename FutureType<T>::Type;
};

template <class F, class T>
struct FutureCallResult {
  using Type = decltype(std::declval<F&>()(std::declval<const Future<T>&>()));
};

}  // namespace detail

template <class F>
using FutureType = typename detail::FutureType<F>::Type;

template <class F, class T>
using FutureCallResult = typename detail::FutureCallResult<F, T>::Type;

class FutureStateId;

template <class T>
class Future {
  using FutureState = detail::FutureState<T>;

 public:
  using value_type = T;

  Future() noexcept = default;
  Future(const Future<T>& other) noexcept = default;
  Future(Future<T>&& other) noexcept = default;
  Future(const IntrusivePtr<FutureState>& state) noexcept;

  Future<T>& operator=(const Future<T>& other) noexcept = default;
  Future<T>& operator=(Future<T>&& other) noexcept = default;
  void swap(Future<T>& other);

  bool initialized() const;

  bool hasValue() const;
  const T& getValue(Duration timeout = Time::zeroDuration()) const;
  const T& getValueSync() const;
  T extractValue(Duration timeout = Time::zeroDuration());
  T extractValueSync();

  void tryRethrow() const;
  bool hasException() const;

  void wait() const;
  bool wait(Duration timeout) const;
  bool wait(Instant deadline) const;

  template <class F>
  const Future<T>& subscribe(F&& callback) const;

  template <class F>
  Future<FutureType<FutureCallResult<F, T>>> apply(F&& func) const;

  Future<void> ignoreResult() const;

  std::optional<FutureStateId> stateId() const noexcept;

 private:
  void ensureInitialized() const;

 private:
  IntrusivePtr<FutureState> state_;
};

template <>
class Future<void> {
  using FutureState = detail::FutureState<void>;

 public:
  using value_type = void;

  Future() noexcept = default;
  Future(const Future<void>& other) noexcept = default;
  Future(Future<void>&& other) noexcept = default;
  Future(const IntrusivePtr<FutureState>& state) noexcept;

  Future<void>& operator=(const Future<void>& other) noexcept = default;
  Future<void>& operator=(Future<void>&& other) noexcept = default;
  void swap(Future<void>& other);

  bool initialized() const;

  bool hasValue() const;
  void getValue(Duration timeout = Time::zeroDuration()) const;
  void getValueSync() const;

  void tryRethrow() const;
  bool hasException() const;

  void wait() const;
  bool wait(Duration timeout) const;
  bool wait(Instant deadline) const;

  template <class F>
  const Future<void>& subscribe(F&& callback) const;

  template <class F>
  Future<FutureType<FutureCallResult<F, void>>> apply(F&& func) const;

  template <class R>
  Future<R> withValue(const R& value) const;

  Future<void> ignoreResult() const { return *this; }

  std::optional<FutureStateId> stateId() const noexcept;

 private:
  void ensureInitialized() const;

 private:
  IntrusivePtr<FutureState> state_;
};

template <class T>
class Promise {
  using FutureState = detail::FutureState<T>;

 public:
  Promise() noexcept = default;
  Promise(const Promise<T>& other) noexcept = default;
  Promise(Promise<T>&& other) noexcept = default;
  Promise(const IntrusivePtr<FutureState>& state) noexcept;

  Promise<T>& operator=(const Promise<T>& other) noexcept = default;
  Promise<T>& operator=(Promise<T>&& other) noexcept = default;
  void swap(Promise<T>& other);

  bool initialized() const;

  bool hasValue() const;
  const T& getValue() const;
  T extractValue();

  void setValue(const T& value);
  void setValue(T&& value);

  bool trySetValue(const T& value);
  bool trySetValue(T&& value);

  void tryRethrow() const;
  bool hasException() const;
  void setException(const std::string& e);
  void setException(std::exception_ptr e);
  bool trySetException(std::exception_ptr e);

  Future<T> getFuture() const;
  operator Future<T>() const;

 private:
  void ensureInitialized() const;

 private:
  IntrusivePtr<FutureState> state_;
};

template <>
class Promise<void> {
  using FutureState = detail::FutureState<void>;

 public:
  Promise() noexcept = default;
  Promise(const Promise<void>& other) noexcept = default;
  Promise(Promise<void>&& other) noexcept = default;
  Promise(const IntrusivePtr<FutureState>& state) noexcept;

  Promise<void>& operator=(const Promise<void>& other) noexcept = default;
  Promise<void>& operator=(Promise<void>&& other) noexcept = default;
  void swap(Promise<void>& other);

  bool initialized() const;

  bool hasValue() const;
  void getValue() const;

  void setValue();
  bool trySetValue();

  void tryRethrow() const;
  bool hasException() const;
  void setException(const std::string& e);
  void setException(std::exception_ptr e);
  bool trySetException(std::exception_ptr e);

  Future<void> getFuture() const;
  operator Future<void>() const;

 private:
  void ensureInitialized() const;

 private:
  IntrusivePtr<FutureState> state_;
};

}  // namespace core

#define INCLUDE_FUTURE_INL_H
#include "future-inl.h"
#undef INCLUDE_FUTURE_INL_H