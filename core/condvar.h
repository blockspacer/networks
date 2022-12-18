#pragma once

#include "datetime.h"
#include "mutex.h"

#include <utility>

namespace core {

class CondVar {
 public:
  CondVar();
  ~CondVar();

  void broadCast() noexcept;
  void signal() noexcept;

  bool wait(Mutex& m, Instant deadline) noexcept;

  template <class P>
  inline bool wait(Mutex& m, Instant deadline, P pred) noexcept {
    while (!pred()) {
      if (!wait(m, deadline)) {
        return pred();
      }
    }
    return true;
  }

  auto wait(Mutex& m, Duration timeout) noexcept { return wait(m, Time::toDeadLine(timeout)); }

  template <class P>
  inline auto wait(Mutex& m, Duration timeout, P pred) noexcept {
    return wait(m, Time::toDeadLine(timeout), std::move(pred));
  }

  inline void wait(Mutex& m) noexcept { wait(m, Time::infiniteFuture()); }

  template <class P>
  inline void wait(Mutex& m, P pred) noexcept {
    return wait(m, Time::infiniteFuture(), std::move(pred));
  }

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace core