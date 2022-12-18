#pragma once

#include "datetime.h"
#include "intrusive_ptr.h"

namespace core {

struct EventResetType {
  enum ResetMode {
    AUTO,
    MANUAL,
  };
};

class SystemEvent : public EventResetType {
 public:
  SystemEvent(ResetMode mode = MANUAL);
  SystemEvent(const SystemEvent& other) noexcept;
  SystemEvent& operator=(const SystemEvent& other) noexcept;

  ~SystemEvent();

  void reset() noexcept;
  void signal() noexcept;

  bool wait(Instant deadline) noexcept;

  inline auto wait(Duration timeout) noexcept { return wait(Time::toDeadLine(timeout)); }

  inline void wait() noexcept { wait(Time::infiniteFuture()); }

 private:
  class EvImpl;
  IntrusivePtr<EvImpl> impl_;
};

class AutoEvent : public SystemEvent {
 public:
  AutoEvent()
      : SystemEvent(SystemEvent::AUTO) {}
};

class ManualEvent {
 public:
  ManualEvent()
      : ev_(EventResetType::MANUAL) {}

  void reset() noexcept { SystemEvent{ev_}.reset(); }

  void signal() noexcept { SystemEvent{ev_}.signal(); }

  auto wait(Instant deadline) noexcept { return SystemEvent{ev_}.wait(deadline); }

  inline auto wait(Duration timeout) noexcept { return SystemEvent{ev_}.wait(timeout); }

  inline void wait() noexcept { SystemEvent{ev_}.wait(); }

 private:
  SystemEvent ev_;
};

}  // namespace core