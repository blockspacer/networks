#pragma once

#if !defined(INCLUDE_FUTURE_INL_H)
//#error "you should never include future-inl.h directly"
#endif  // INCLUDE_FUTURE_INL_H

#include "atomic.h"
#include <memory>

namespace core::detail {

template <class T>
using Callback = std::function<void(const Future<T>&)>;

template <class T>
using CallbackList = std::vector<Callback<T>>;

enum class Error { ERROR };

template <class T>
class FutureState : public AtomicRefCount<FutureState<T>> {
  enum : AtomicType { NOT_READY, EXCEPTION_SET, VALUE_MOVED, VALUE_SET, VALUE_READ };

 public:
  FutureState()
      : state_(NOT_READY)
      , null_value_(0) {}

  template <class U>
  FutureState(U&& value)
      : state_(VALUE_SET)
      , value_(std::forward<U>(value)) {}

  FutureState(std::exception_ptr e, Error)
      : state_(EXCEPTION_SET)
      , exception_(std::move(e))
      , null_value_(0) {}

  ~FutureState() {
    if (state_ >= VALUE_MOVED) {
      value_.~T();
    }
  }

  bool hasValue() const { return atomics::Load(state_) >= VALUE_MOVED; }

  void tryRethrow() const {
    int state = atomics::Load(state_);
    tryRethrowWithState(state);
  }

  bool hasException() const { return atomics::Load(state_) == EXCEPTION_SET; }

  const T& getValue(Duration timeout = Time::zeroDuration()) const {
    accessValue(timeout, VALUE_READ);
    return value_;
  }

  T extractValue(Duration timeout = Time::zeroDuration()) {
    accessValue(timeout, VALUE_MOVED);
    return std::move(value_);
  }

  template <class U>
  void setValue(U&& value) {
    bool success = trySetValue(std::forward<U>(value));
    if (core_unlikely(!success)) {
      core_throw FutureException() << "value already set";
    }
  }

  template <class U>
  bool trySetValue(U&& value) {
    SystemEvent* ready_event = nullptr;
    CallbackList<T> callbacks;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (core_unlikely(state != NOT_READY)) {
        return false;
      }

      new (&value_) T(std::forward<U>(value));

      ready_event = ready_event_.get();
      callbacks = std::move(callbacks_);

      atomics::Store(state_, VALUE_SET);
    }

    if (ready_event) {
      ready_event->signal();
    }

    if (!callbacks.empty()) {
      Future<T> temp(this);
      for (auto& callback : callbacks) {
        callback(temp);
      }
    }

    return true;
  }

  void setException(std::exception_ptr e) {
    bool success = trySetException(std::move(e));
    if (core_unlikely(!success)) {
      core_throw FutureException() << "value already set";
    }
  }

  bool trySetException(std::exception_ptr e) {
    SystemEvent* ready_event = nullptr;
    CallbackList<T> callbacks;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (core_unlikely(state != NOT_READY)) {
        return false;
      }

      exception_ = std::move(e);

      ready_event = ready_event_.get();
      callbacks = std::move(callbacks_);

      atomics::Store(state_, EXCEPTION_SET);
    }

    if (ready_event) {
      ready_event->signal();
    }

    if (!callbacks.empty()) {
      Future<T> temp(this);
      for (auto& callback : callbacks) {
        callback(temp);
      }
    }

    return true;
  }

  template <class F>
  bool subscribe(F&& func) {
    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (state == NOT_READY) {
        callbacks_.emplace_back(std::forward<F>(func));
        return true;
      }
    }
    return false;
  }

  void wait() const { wait(Time::infiniteFuture()); }

  bool wait(Duration timeout) const { return wait(Time::toDeadLine(timeout)); }

  bool wait(Instant deadline) const {
    SystemEvent* ready_event = nullptr;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (state != NOT_READY) {
        return true;
      }

      if (!ready_event_) {
        ready_event_ = std::make_unique<SystemEvent>();
      }
      ready_event = ready_event_.get();
    }

    core_assert(ready_event);
    return ready_event->wait(deadline);
  }

  void tryRethrowWithState(int state) const {
    if (core_unlikely(state == EXCEPTION_SET)) {
      core_assert(exception_);
      std::rethrow_exception(exception_);
    }
  }

 private:
  void accessValue(Duration timeout, int acquire_state) const {
    int state = atomics::Load(state_);
    if (core_unlikely(state == NOT_READY)) {
      if (timeout == Time::zeroDuration()) {
        core_throw FutureException() << "value not set";
      }
      if (!wait(timeout)) {
        core_throw FutureException() << "wait timeout";
      }
      state = atomics::Load(state_);
    }

    tryRethrowWithState(state);

    switch (atomics::GetAndCas(&state_, acquire_state, VALUE_SET)) {
      case VALUE_SET:
        break;
      case VALUE_READ:
        if (acquire_state != VALUE_READ) {
          core_throw FutureException() << "value being read";
        }
        break;
      case VALUE_MOVED:
        core_throw FutureException() << "value was moved";
      default:
        core_assert(state == VALUE_SET);
    }
  }

 private:
  mutable Atomic state_;
  AdaptiveLock lock_;

  CallbackList<T> callbacks_;
  mutable std::unique_ptr<SystemEvent> ready_event_;

  std::exception_ptr exception_;

  union {
    char null_value_;
    T value_;
  };
};

template <>
class FutureState<void> : public AtomicRefCount<FutureState<void>> {
  enum : AtomicType { NOT_READY, VALUE_SET, EXCEPTION_SET };

 public:
  FutureState(bool value_set = false)
      : state_(value_set ? VALUE_SET : NOT_READY) {}

  FutureState(std::exception_ptr e, Error)
      : state_(EXCEPTION_SET)
      , exception_(std::move(e)) {}

  bool hasValue() const { return atomics::Load(state_) == VALUE_SET; }

  void tryRethrow() const {
    int state = atomics::Load(state_);
    tryRethrowWithState(state);
  }

  bool hasException() const { return atomics::Load(state_) == EXCEPTION_SET; }

  void getValue(Duration timeout = Time::zeroDuration()) const {
    int state = atomics::Load(state_);
    if (core_unlikely(state == NOT_READY)) {
      if (timeout == Time::zeroDuration()) {
        core_throw FutureException() << "value not set";
      }
      if (!wait(timeout)) {
        core_throw FutureException() << "wait timeout";
      }
      state = atomics::Load(state_);
    }

    tryRethrowWithState(state);
    core_assert(state == VALUE_SET);
  }

  void setValue() {
    bool success = trySetValue();
    if (core_unlikely(!success)) {
      core_throw FutureException() << "value already set";
    }
  }

  bool trySetValue() {
    SystemEvent* ready_event = nullptr;
    CallbackList<void> callbacks;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (core_unlikely(state != NOT_READY)) {
        return false;
      }

      ready_event = ready_event_.get();
      callbacks = std::move(callbacks_);

      atomics::Store(state_, VALUE_SET);
    }

    if (ready_event) {
      ready_event->signal();
    }

    if (!callbacks.empty()) {
      Future<void> temp(this);
      for (auto& callback : callbacks) {
        callback(temp);
      }
    }

    return true;
  }

  void setException(std::exception_ptr e) {
    bool success = trySetException(std::move(e));
    if (core_unlikely(!success)) {
      core_throw FutureException() << "value already set";
    }
  }

  bool trySetException(std::exception_ptr e) {
    SystemEvent* ready_event = nullptr;
    CallbackList<void> callbacks;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (core_unlikely(state != NOT_READY)) {
        return false;
      }

      exception_ = std::move(e);

      ready_event = ready_event_.get();
      callbacks = std::move(callbacks_);

      atomics::Store(state_, EXCEPTION_SET);
    }

    if (ready_event) {
      ready_event->signal();
    }

    if (!callbacks.empty()) {
      Future<void> temp(this);
      for (auto& callback : callbacks) {
        callback(temp);
      }
    }

    return true;
  }

  template <class F>
  bool subscribe(F&& func) {
    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (state == NOT_READY) {
        callbacks_.emplace_back(std::forward<F>(func));
        return true;
      }
    }
    return false;
  }

  void wait() const { wait(absl::InfiniteFuture()); }

  bool wait(Duration timeout) const { return wait(Time::toDeadLine(timeout)); }

  bool wait(Instant deadline) const {
    SystemEvent* ready_event = nullptr;

    core_with_lock(lock_) {
      int state = atomics::Load(state_);
      if (state != NOT_READY) {
        return true;
      }

      if (!ready_event_) {
        ready_event_ = std::make_unique<SystemEvent>();
      }
      ready_event = ready_event_.get();
    }

    core_assert(ready_event);
    return ready_event->wait(deadline);
  }

  void tryRethrowWithState(int state) const {
    if (core_unlikely(state == EXCEPTION_SET)) {
      core_assert(exception_);
      std::rethrow_exception(exception_);
    }
  }

 private:
  Atomic state_;
  AdaptiveLock lock_;

  CallbackList<void> callbacks_;
  mutable std::unique_ptr<SystemEvent> ready_event_;

  std::exception_ptr exception_;
};

template <class T>
inline void SetValueImpl(Promise<T>& promise, const T& value) {
  promise.setValue(value);
}

template <class T>
inline void SetValueImpl(Promise<T>& promise, T&& value) {
  promise.setValue(std::move(value));
}

template <class T>
inline void SetValueImpl(Promise<T>& promise, const Future<T>& future,
                         std::enable_if_t<!std::is_void<T>::value, bool> = false) {
  future.subscribe([=](const Future<T>& f) mutable {
    T const* value;
    try {
      value = &f.getValue();
    } catch (...) {
      promise.setException(std::current_exception());
      return;
    }
    promise.setValue(*value);
  });
}

template <class T>
inline void SetValueImpl(Promise<void>& promise, const Future<T>& future) {
  future.subscribe([=](const Future<T>& f) mutable {
    try {
      f.tryRethrow();
    } catch (...) {
      promise.setException(std::current_exception());
      return;
    }
    promise.setValue();
  });
}

template <class T, class F>
inline void SetValue(Promise<T>& promise, F&& func) {
  try {
    SetValueImpl(promise, func());
  } catch (...) {
    const bool success = promise.trySetException(std::current_exception());
    if (core_unlikely(!success)) {
      throw;
    }
  }
}

template <class F>
inline void SetValue(Promise<void>& promise, F&& func,
                     std::enable_if_t<std::is_void<std::invoke_result_t<F>>::value, bool> = false) {
  try {
    func();
  } catch (...) {
    promise.setException(std::current_exception());
    return;
  }
  promise.setValue();
}

struct WaitExceptionOrAll : public AtomicRefCount<WaitExceptionOrAll> {
  WaitExceptionOrAll(size_t count)
      : promise(NewPromise())
      , count(count) {}

  template <class T>
  void set(const Future<T>& future) {
    auto guard = Guard(lock);
    try {
      future.tryRethrow();
      if (--count == 0) {
        promise.setValue();
      }
    } catch (...) {
      core_assert(!promise.hasValue());
      if (!promise.hasException()) {
        promise.setException(std::current_exception());
      }
    }
  }

  Promise<void> promise;
  size_t count;
  SpinLock lock;
};

struct WaitAll : public AtomicRefCount<WaitAll> {
  WaitAll(size_t count)
      : promise(NewPromise())
      , count(count)
      , exception() {}

  template <class T>
  void set(const Future<T>& future) {
    auto guard = Guard(lock);

    if (!exception) {
      try {
        future.tryRethrow();
      } catch (...) {
        exception = std::current_exception();
      }
    }

    if (--count == 0) {
      core_assert(!promise.hasValue() && !promise.hasException());
      if (exception) {
        promise.setException(std::move(exception));
      } else {
        promise.setValue();
      }
    }
  }

  Promise<void> promise;
  size_t count;
  SpinLock lock;
  std::exception_ptr exception;
};

struct WaitAny : public AtomicRefCount<WaitAny> {
  WaitAny()
      : promise(NewPromise()) {}

  template <class T>
  void set(const Future<T>& future) {
    if (lock.tryAcquire()) {
      try {
        future.tryRethrow();
      } catch (...) {
        core_assert(!promise.hasValue() && !promise.hasException());
        promise.setException(std::current_exception());
        return;
      }
      promise.setValue();
    }
  }

  Promise<void> promise;
  SpinLock lock;
};

}  // namespace core::detail

namespace core {

class FutureStateId {
 public:
  template <class T>
  explicit FutureStateId(const detail::FutureState<T>& state)
      : id_(&state) {}

  core_warn_unused_result auto value() const noexcept { return id_; }

 private:
  const void* id_;
};

inline auto operator==(const FutureStateId& l, const FutureStateId& r) -> bool { return l.value() == r.value(); }

inline auto operator!=(const FutureStateId& l, const FutureStateId& r) -> bool { return !(l == r); }

template <class T>
inline Future<T>::Future(const IntrusivePtr<FutureState>& state) noexcept
    : state_(state) {}

template <class T>
inline void Future<T>::swap(Future<T>& other) {
  state_.swap(other.state_);
}

template <class T>
inline bool Future<T>::hasValue() const {
  return state_ && state_->hasValue();
}

template <class T>
inline const T& Future<T>::getValue(Duration timeout) const {
  ensureInitialized();
  return state_->getValue(timeout);
}

template <class T>
inline T Future<T>::extractValue(Duration timeout) {
  ensureInitialized();
  return state_->extractValue(timeout);
}

template <class T>
inline const T& Future<T>::getValueSync() const {
  return getValue(Time::infiniteDuration());
}

template <class T>
inline T Future<T>::extractValueSync() {
  return extractValue(Time::infiniteDuration());
}

template <class T>
inline void Future<T>::tryRethrow() const {
  if (state_) {
    state_->tryRethrow();
  }
}

template <class T>
inline bool Future<T>::hasException() const {
  return state_ && state_->hasException();
}

template <class T>
inline void Future<T>::wait() const {
  ensureInitialized();
  state_->wait();
}

template <class T>
inline bool Future<T>::wait(Duration timeout) const {
  ensureInitialized();
  return state_->wait(timeout);
}

template <class T>
inline bool Future<T>::wait(Instant deadline) const {
  ensureInitialized();
  return state_->wait(deadline);
}

template <class T>
template <class F>
inline const Future<T>& Future<T>::subscribe(F&& callback) const {
  ensureInitialized();
  if (!state_->subscribe(std::forward<F>(callback))) {
    callback(*this);
  }
  return *this;
}

template <class T>
template <class F>
inline Future<FutureType<FutureCallResult<F, T>>> Future<T>::apply(F&& func) const {
  auto promise = NewPromise<FutureType<FutureCallResult<F, T>>>();
  subscribe([promise, func = std::forward<F>(func)](const Future<T>& future) mutable {
    detail::SetValue(promise, [&]() { return func(future); });
  });
  return promise;
}

template <class T>
inline Future<void> Future<T>::ignoreResult() const {
  auto promise = NewPromise();
  subscribe([=](const Future<T>& future) mutable { detail::SetValueImpl(promise, future); });
  return promise;
}

template <class T>
inline bool Future<T>::initialized() const {
  return bool(state_);
}

template <class T>
inline std::optional<FutureStateId> Future<T>::stateId() const noexcept {
  return (state_ != nullptr ? std::make_optional<FutureStateId>(*state_) : std::nullopt);
}

template <class T>
inline void Future<T>::ensureInitialized() const {
  if (!state_) {
    core_throw FutureException() << "state not initialized";
  }
}

inline Future<void>::Future(const IntrusivePtr<FutureState>& state) noexcept
    : state_(state) {}

inline void Future<void>::swap(core::Future<void>& other) { state_.swap(other.state_); }

inline bool Future<void>::hasValue() const { return state_ && state_->hasValue(); }

inline void Future<void>::getValue(Duration timeout) const {
  ensureInitialized();
  state_->getValue(timeout);
}

inline void Future<void>::getValueSync() const { getValue(Time::infiniteDuration()); }

inline void Future<void>::tryRethrow() const {
  if (state_) {
    state_->tryRethrow();
  }
}

inline bool Future<void>::hasException() const { return state_ && state_->hasException(); }

inline void Future<void>::wait() const {
  ensureInitialized();
  state_->wait();
}

inline bool Future<void>::wait(Duration timeout) const {
  ensureInitialized();
  return state_->wait(timeout);
}

inline bool Future<void>::wait(Instant deadline) const {
  ensureInitialized();
  return state_->wait(deadline);
}

template <class F>
inline const Future<void>& Future<void>::subscribe(F&& callback) const {
  ensureInitialized();
  if (!state_->subscribe(std::forward<F>(callback))) {
    callback(*this);
  }
  return *this;
}

template <class F>
inline Future<FutureType<FutureCallResult<F, void>>> Future<void>::apply(F&& func) const {
  auto promise = NewPromise<FutureType<FutureCallResult<F, void>>>();
  subscribe([promise, func = std::forward<F>(func)](const Future<void>& future) mutable {
    detail::SetValue(promise, [&]() { return func(future); });
  });
  return promise;
}

template <class R>
inline Future<R> Future<void>::withValue(const R& value) const {
  auto promise = NewPromise<R>();
  subscribe([=](const Future<void>& future) mutable {
    try {
      future.tryRethrow();
    } catch (...) {
      promise.setException(std::current_exception());
      return;
    }
    promise.setValue(value);
  });
  return promise;
}

inline bool Future<void>::initialized() const { return bool(state_); }

inline std::optional<FutureStateId> Future<void>::stateId() const noexcept {
  return (state_ != nullptr ? std::make_optional<FutureStateId>(*state_) : std::nullopt);
}

inline void Future<void>::ensureInitialized() const {
  if (!state_) {
    core_throw FutureException() << "state not initialized";
  }
}

template <class T>
inline Promise<T>::Promise(const IntrusivePtr<FutureState>& state) noexcept
    : state_(state) {}

template <class T>
inline void Promise<T>::swap(Promise<T>& other) {
  state_.swap(other.state_);
}

template <class T>
inline const T& Promise<T>::getValue() const {
  ensureInitialized();
  return state_->getValue();
}

template <class T>
inline T Promise<T>::extractValue() {
  ensureInitialized();
  return state_->extractValue();
}

template <class T>
inline bool Promise<T>::hasValue() const {
  return state_ && state_->hasValue();
}

template <class T>
inline void Promise<T>::setValue(const T& value) {
  ensureInitialized();
  state_->setValue(value);
}

template <class T>
inline void Promise<T>::setValue(T&& value) {
  ensureInitialized();
  state_->setValue(std::move(value));
}

template <class T>
inline bool Promise<T>::trySetValue(const T& value) {
  ensureInitialized();
  return state_->trySetValue(value);
}

template <class T>
inline bool Promise<T>::trySetValue(T&& value) {
  ensureInitialized();
  return state_->trySetValue(std::move(value));
}

template <class T>
inline void Promise<T>::tryRethrow() const {
  if (state_) {
    state_->tryRethrow();
  }
}

template <class T>
inline bool Promise<T>::hasException() const {
  return state_ && state_->hasException();
}

template <class T>
inline void Promise<T>::setException(const std::string& e) {
  ensureInitialized();
  state_->setException(std::make_exception_ptr(Exception() << e));
}

template <class T>
inline void Promise<T>::setException(std::exception_ptr e) {
  ensureInitialized();
  state_->setException(std::move(e));
}

template <class T>
inline bool Promise<T>::trySetException(std::exception_ptr e) {
  ensureInitialized();
  return state_->trySetException(std::move(e));
}

template <class T>
inline Future<T> Promise<T>::getFuture() const {
  ensureInitialized();
  return Future<T>(state_);
}

template <class T>
inline Promise<T>::operator Future<T>() const {
  return getFuture();
}

template <class T>
inline bool Promise<T>::initialized() const {
  return bool(state_);
}

template <class T>
inline void Promise<T>::ensureInitialized() const {
  if (!state_) {
    core_throw FutureException() << "state not initialized";
  }
}

inline Promise<void>::Promise(const IntrusivePtr<FutureState>& state) noexcept
    : state_(state) {}

inline void Promise<void>::swap(Promise<void>& other) { state_.swap(other.state_); }

inline void Promise<void>::getValue() const {
  ensureInitialized();
  state_->getValue();
}

inline bool Promise<void>::hasValue() const { return state_ && state_->hasValue(); }

inline void Promise<void>::setValue() {
  ensureInitialized();
  state_->setValue();
}

inline bool Promise<void>::trySetValue() {
  ensureInitialized();
  return state_->trySetValue();
}

inline void Promise<void>::tryRethrow() const {
  if (state_) {
    state_->tryRethrow();
  }
}

inline bool Promise<void>::hasException() const { return state_ && state_->hasException(); }

inline void Promise<void>::setException(const std::string& e) {
  ensureInitialized();
  state_->setException(std::make_exception_ptr(Exception() << e));
}

inline void Promise<void>::setException(std::exception_ptr e) {
  ensureInitialized();
  state_->setException(std::move(e));
}

inline bool Promise<void>::trySetException(std::exception_ptr e) {
  ensureInitialized();
  return state_->trySetException(std::move(e));
}

inline Future<void> Promise<void>::getFuture() const {
  ensureInitialized();
  return Future<void>(state_);
}

inline Promise<void>::operator Future<void>() const { return getFuture(); }

inline bool Promise<void>::initialized() const { return bool(state_); }

inline void Promise<void>::ensureInitialized() const {
  if (!state_) {
    core_throw FutureException() << "state not initialized";
  }
}

template <class T>
inline Promise<T> NewPromise() {
  return {new detail::FutureState<T>()};
}

inline Promise<void> NewPromise() { return {new detail::FutureState<void>()}; }

template <class T>
inline Future<T> MakeFuture(const T& value) {
  return {new detail::FutureState<T>(value)};
}

template <class T>
inline Future<std::remove_reference_t<T>> MakeFuture(T&& value) {
  return {new detail::FutureState<std::remove_reference_t<T>>(std::move(value))};
}

template <class T>
inline Future<T> MakeFuture() {
  struct Cache {
    Future<T> instance{new detail::FutureState<T>(Default<T>())};
  };
  return Singleton<Cache>()->instance;
}

template <class T>
inline Future<T> MakeErrorFuture(std::exception_ptr exception) {
  return {new detail::FutureState<T>(std::move(exception), detail::Error::ERROR)};
}

inline Future<void> MakeFuture() {
  struct Cache {
    Future<void> instance{new detail::FutureState<void>(true)};
  };
  return Singleton<Cache>()->instance;
}

core_warn_unused_result inline Future<void> WaitAll(const Future<void>& f1) { return f1; }

core_warn_unused_result inline Future<void> WaitAll(const Future<void>& f1, const Future<void>& f2) {
  using Callback = detail::Callback<void>;
  IntrusivePtr<detail::WaitAll> waiter = new detail::WaitAll(2);
  auto callback = Callback([=](const Future<void>& future) mutable { waiter->set(future); });

  f1.subscribe(callback);
  f2.subscribe(callback);

  return waiter->promise;
}

template <class Container>
core_warn_unused_result inline Future<void> WaitAll(const Container& futures) {
  if (futures.empty()) {
    return MakeFuture();
  }

  if (futures.size() == 1) {
    return futures.front().ignoreResult();
  }

  using Callback = detail::Callback<typename Container::value_type::value_type>;
  IntrusivePtr<detail::WaitAll> waiter = new detail::WaitAll(futures.size());
  auto callback = Callback([=](const auto& future) mutable { waiter->set(future); });

  for (auto& fut : futures) {
    fut.subscribe(callback);
  }

  return waiter->promise;
}

core_warn_unused_result inline Future<void> WaitExceptionOrAll(const Future<void>& f1) { return f1; }

core_warn_unused_result inline Future<void> WaitExceptionOrAll(const Future<void>& f1, const Future<void>& f2) {
  using Callback = detail::Callback<void>;
  IntrusivePtr<detail::WaitExceptionOrAll> waiter = new detail::WaitExceptionOrAll(2);
  auto callback = Callback([=](const Future<void>& future) mutable { waiter->set(future); });

  f1.subscribe(callback);
  f2.subscribe(callback);

  return waiter->promise;
}

template <class Container>
core_warn_unused_result inline Future<void> WaitExceptionOrAll(const Container& futures) {
  if (futures.empty()) {
    return MakeFuture();
  }

  if (futures.size() == 1) {
    return futures.front().ignoreResult();
  }

  using Callback = detail::Callback<typename Container::value_type::value_type>;
  IntrusivePtr<detail::WaitExceptionOrAll> waiter = new detail::WaitExceptionOrAll(futures.size());
  auto callback = Callback([=](const auto& future) mutable { waiter->set(future); });

  for (auto& fut : futures) {
    fut.subscribe(callback);
  }

  return waiter->promise;
}

core_warn_unused_result inline Future<void> WaitAny(const Future<void>& f1) { return f1; }

core_warn_unused_result inline Future<void> WaitAny(const Future<void>& f1, const Future<void>& f2) {
  using Callback = detail::Callback<void>;
  IntrusivePtr<detail::WaitAny> waiter = new detail::WaitAny();
  auto callback = Callback([=](const Future<void>& future) mutable { waiter->set(future); });

  f1.subscribe(callback);
  f2.subscribe(callback);

  return waiter->promise;
}

template <class Container>
core_warn_unused_result inline Future<void> WaitAny(const Container& futures) {
  if (futures.empty()) {
    return MakeFuture();
  }

  if (futures.size() == 1) {
    return futures.front().ignoreResult();
  }

  using Callback = detail::Callback<typename Container::value_type::value_type>;
  IntrusivePtr<detail::WaitAny> waiter = new detail::WaitAny();
  auto callback = Callback([=](const auto& future) mutable { waiter->set(future); });

  for (auto& fut : futures) {
    fut.subscribe(callback);
  }

  return waiter->promise;
}

}  // namespace core