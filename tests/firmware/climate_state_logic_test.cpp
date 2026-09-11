#include <cassert>

#include "climate_state_logic.h"

using espcontrol::climate::Status;

struct ClimateState {
  bool available;
  const char *mode;
  const char *action;
};

void assert_state(const ClimateState &state, Status status, bool active,
                  bool icon_enabled) {
  assert(espcontrol::climate::status(
           state.available, state.mode, state.action) == status);
  assert(espcontrol::climate::active(
           state.available, state.mode, state.action) == active);
  assert(espcontrol::climate::icon_enabled(state.available, state.mode) == icon_enabled);
}

void assert_parent_indicator(const ClimateState &state, bool active) {
  assert(espcontrol::climate::parent_indicator_active(
           state.available, state.mode, state.action) == active);
}

int main() {
  // The displayed state table, including stale actions while switched off.
  assert_state({false, "auto", "heating"}, Status::UNAVAILABLE, false, false);
  assert_state({true, "off", "heating"}, Status::OFF, false, false);
  assert_state({true, "auto", "heating"}, Status::HEATING, true, true);
  assert_state({true, "heat", "heating"}, Status::HEATING, true, true);
  assert_state({true, "auto", "idle"}, Status::IDLE, false, true);
  assert_state({true, "heat", "idle"}, Status::IDLE, false, true);
  assert_state({true, "auto", "off"}, Status::OFF, false, true);
  assert_state({true, "auto", ""}, Status::MODE_FALLBACK, true, true);
  assert_state({true, "heat", "unknown"}, Status::MODE_FALLBACK, true, true);
  assert_state({true, "cool", "unavailable"}, Status::MODE_FALLBACK, true, true);

  // Existing action handling remains unchanged for enabled modes.
  assert_state({true, "cool", "cooling"}, Status::COOLING, true, true);
  assert_state({true, "dry", "drying"}, Status::DRYING, true, true);
  assert_state({true, "fan_only", "fan"}, Status::FAN, true, true);
  assert_state({true, "heat", "preheating"}, Status::IDLE, true, true);

  // Update ordering and refresh delivery are exercised in climate_update_test.

  // Subpage parents reflect reported HVAC activity, while Off still wins.
  assert_parent_indicator({true, "auto", "heating"}, true);
  assert_parent_indicator({true, "cool", "cooling"}, true);
  assert_parent_indicator({true, "dry", "drying"}, true);
  assert_parent_indicator({true, "fan_only", "fan"}, true);
  assert_parent_indicator({true, "off", "heating"}, false);
  assert_parent_indicator({false, "auto", "heating"}, false);
  assert_parent_indicator({true, "auto", "idle"}, false);
  assert_parent_indicator({true, "auto", ""}, false);
  assert_parent_indicator({true, "auto", "unknown"}, false);
  assert_parent_indicator({true, "auto", "unavailable"}, false);
  assert_parent_indicator({true, "heat", "preheating"}, false);

  return 0;
}
