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
  assert(espcontrol::climate::icon_enabled(
           state.available, state.mode, state.action) == icon_enabled);
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

  // Heating -> Off retains the action because Home Assistant may not resend it.
  ClimateState state{true, "heat", "heating"};
  assert_state(state, Status::HEATING, true, true);
  state.mode = "off";
  assert_state(state, Status::OFF, false, false);
  state.action = "heating";  // A delayed action must not override Off.
  assert_state(state, Status::OFF, false, false);

  // Auto follows action changes and remains idle without another update.
  state = {true, "auto", "heating"};
  assert_state(state, Status::HEATING, true, true);
  state.action = "idle";
  assert_state(state, Status::IDLE, false, true);
  assert_state(state, Status::IDLE, false, true);

  // Off -> Auto reuses the retained action until Home Assistant changes it.
  state = {true, "off", "idle"};
  assert_state(state, Status::OFF, false, false);
  state.mode = "auto";
  assert_state(state, Status::IDLE, false, true);

  // State and attribute callbacks may arrive in either order.
  state = {true, "heat", "heating"};
  state.mode = "off";
  assert_state(state, Status::OFF, false, false);
  state.action = "off";
  assert_state(state, Status::OFF, false, false);

  state = {true, "heat", "heating"};
  state.action = "off";
  assert_state(state, Status::OFF, false, true);
  state.mode = "off";
  assert_state(state, Status::OFF, false, false);

  // Availability always wins, then the retained values resume when restored.
  state = {true, "auto", "heating"};
  state.available = false;
  assert_state(state, Status::UNAVAILABLE, false, false);
  state.available = true;
  assert_state(state, Status::HEATING, true, true);

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
