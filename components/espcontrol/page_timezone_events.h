#pragma once

// Include after sun_calc.h and the legacy date/time helpers. Application installs
// this only after retiring the legacy grid, before preparing its first document.
#include "page_timezone_runtime.h"
#include "timezone_state_events.h"
namespace espcontrol::pages {
inline PageTimezoneSelection page_timezone_resolve(void*, std::string_view option, time_t epoch) {
  const auto id = timezone_id_view(option);
  return {id, timezone_posix_view(id, epoch)};
}
inline PageTimezoneResolver page_timezone_resolver() {
  return {nullptr, page_timezone_resolve};
}
class PageTimezoneEvents {
 public:
  explicit PageTimezoneEvents(PageTimezoneRuntime& runtime) : runtime_(runtime) {}
  ~PageTimezoneEvents() {
    uninstall();
  }
  PageTimezoneEvents(const PageTimezoneEvents&) = delete;
  PageTimezoneEvents& operator=(const PageTimezoneEvents&) = delete;
  bool install(bool legacy_renderer_retired) {
    auto& events = timezone_state_events();
    if (!legacy_renderer_retired || events.owner || timezone_card_count() != 0) return false;
    const auto& state = timezone_card_state();
    if (!runtime_.seed(state.valid, state.epoch, state.active_timezone, state.use_12h)) return false;
    events = {this, update};
    return true;
  }
  void uninstall() {
    if (timezone_state_events().owner == this) timezone_state_events() = {};
  }

 private:
  static bool update(void* owner, bool valid, time_t epoch, std::string_view option, bool use_12h) {
    return static_cast<PageTimezoneEvents*>(owner)->runtime_.update(valid, epoch, option, use_12h);
  }
  PageTimezoneRuntime& runtime_;
};
}  // namespace espcontrol::pages
