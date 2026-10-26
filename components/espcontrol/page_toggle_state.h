#pragma once

#include <initializer_list>

#include "ha_state_limits.h"
#include "page_fan_mode.h"

namespace espcontrol::pages {
// Keep displayed toggle direction and queued confirmation policy identical.
inline bool page_toggle_on(std::string_view value) {
  value = value.substr(0, HA_SHORT_STATE_MAX_LEN);
  for (const auto state : {"on", "true", "1", "home", "playing", "open", "opened", "opening", "closing",
                           "unlocked", "unlocking", "jammed"})
    if (fan_mode_equal(value, state)) return true;
  return false;
}
}  // namespace espcontrol::pages
