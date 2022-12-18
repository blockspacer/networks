#include "datetime.h"

namespace core {

Instant Time::now() noexcept { return absl::Now(); }

Instant Time::toDeadLine(Duration duration) noexcept { return now() + duration; }

Duration Time::zeroDuration() noexcept { return absl::ZeroDuration(); }

Duration Time::infiniteDuration() noexcept { return absl::InfiniteDuration(); }

Instant Time::infiniteFuture() noexcept { return absl::InfiniteFuture(); }

Duration Time::Seconds(int64_t n) noexcept { return absl::Seconds(n); }

timespec Time::toTimespec(Instant time) noexcept { return absl::ToTimespec(time); }

}  // namespace core