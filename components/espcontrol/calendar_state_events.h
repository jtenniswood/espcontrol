#pragma once
namespace espcontrol {
// Optional application-lifetime consumer. The legacy renderer leaves this empty.
struct CalendarStateEvents {
  void* owner{nullptr};
  void (*date)(void*, bool, int, int){nullptr};
  void (*time)(void*, bool, int, int, int, int, bool){nullptr};
};
inline CalendarStateEvents& calendar_state_events() {
  static CalendarStateEvents events;
  return events;
}
}  // namespace espcontrol
