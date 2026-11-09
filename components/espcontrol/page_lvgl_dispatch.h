#pragma once

#include "lvgl.h"
// LVGL has no public event/timer-dispatch query. This read-only bridge is pinned
// to the exact library shipped by the repository's ESPHome version. Audit the
// fields and event/timer lifetime tests before upgrading that dependency.
#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 5 || LVGL_VERSION_PATCH != 0
#error "Audit pooled-modal dispatch guards for the selected LVGL version"
#endif
#include "src/core/lv_global.h"
namespace espcontrol::pages {
inline bool page_lvgl_dispatching() {
  const auto *state=LV_GLOBAL_DEFAULT();
  return state&&(state->event_header!=nullptr||state->timer_state.already_running);
}
}  // namespace espcontrol::pages
