#pragma once

#include "future.h"
#include "thread_pool.h"

namespace core {

template <class Func>
auto Async(Func&& func, IThreadPool& pool) {
  auto promise = NewPromise<FutureType<std::invoke_result_t<Func>>>();
  auto lambda = [promise, func = std::forward<Func>(func)]() mutable { detail::SetValue(promise, func); };
  pool.safeAddFunc(std::move(lambda));
  return promise.getFuture();
}

}  // namespace core