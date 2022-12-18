#pragma once

#include "absl/time/clock.h"

namespace core {

using Instant = absl::Time;
using Duration = absl::Duration;

struct Time {
  static Instant now() noexcept;
  static Instant toDeadLine(Duration duration) noexcept;

  static Duration zeroDuration() noexcept;
  static Duration infiniteDuration() noexcept;

  static Instant infiniteFuture() noexcept;

  static Duration Seconds(int64_t n) noexcept;

  static timespec toTimespec(Instant time) noexcept;
};

}  // namespace core