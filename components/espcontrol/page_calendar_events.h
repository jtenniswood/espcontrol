#pragma once

// Include after the legacy calendar helpers. Central producers keep their
// existing entry points; the application installs this only after retiring the legacy grid.
#include "calendar_state_events.h"
#include "page_calendar_runtime.h"
namespace espcontrol::pages {
class PageCalendarEvents {
 public:
  explicit PageCalendarEvents(PageCalendarRuntime& runtime) : runtime_(runtime) {}
  ~PageCalendarEvents() {
    uninstall();
  }
  PageCalendarEvents(const PageCalendarEvents&) = delete;
  PageCalendarEvents& operator=(const PageCalendarEvents&) = delete;
  bool install(bool legacy_renderer_retired) {
    auto& events = calendar_state_events();
    if (!legacy_renderer_retired || events.owner || calendar_card_count() != 0 || runtime_.has_publisher())
      return false;
    const auto& state = calendar_date_state();
    if (!runtime_.seed({state.date_valid, state.day, state.month, state.time_valid, state.hour, state.minute,
                        state.use_12h}))
      return false;
    if (!runtime_.attach_publisher(this, publish)) return false;
    events = {this, date, time};
    return true;
  }
  void uninstall() {
    if (calendar_state_events().owner == this) calendar_state_events() = {};
    runtime_.detach_publisher(this);
  }

 private:
  static void date(void* owner, bool valid, int day, int month) {
    auto& runtime = static_cast<PageCalendarEvents*>(owner)->runtime_;
    // The clock's invalid-date fallback is conditional on the active global date.
    // A redundant invalidation must not erase a valid candidate HA date.
    if (!valid && !runtime.state().date_valid) return;
    runtime.date(valid, day, month);
  }
  static void time(void* owner, bool valid, int day, int month, int hour, int minute, bool use_12h) {
    static_cast<PageCalendarEvents*>(owner)->runtime_.time(valid, day, month, hour, minute, use_12h);
  }
  static void publish(void*, const PageCalendarState& state) {
    // Keep the existing time producer's calendar_date_valid() fallback authoritative.
    // Direct assignment avoids treating an HA report as a producer event in a staged configuration.
    auto& current = calendar_date_state();
    current.date_valid = state.date_valid;
    current.day = state.day;
    current.month = state.month;
    current.time_valid = state.time_valid;
    current.hour = state.hour;
    current.minute = state.minute;
    current.use_12h = state.use_12h;
  }
  PageCalendarRuntime& runtime_;
};
}  // namespace espcontrol::pages
