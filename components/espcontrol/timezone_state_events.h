#pragma once
#include <ctime>
#include <string_view>
namespace espcontrol {
struct TimezoneStateEvents {
  void* owner{nullptr};
  bool (*update)(void*, bool, time_t, std::string_view, bool){nullptr};
};
inline TimezoneStateEvents& timezone_state_events() {
  static TimezoneStateEvents events;
  return events;
}
}  // namespace espcontrol
