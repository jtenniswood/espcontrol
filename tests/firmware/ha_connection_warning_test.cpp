#include <cassert>
#include <cstdint>
#include "ha_connection_warning.h"
#include "display_mode_controller.h"
using namespace espcontrol;
int main() {
  HaConnectionWarning warning;
  assert(!warning.update(0, true, true, false));
  assert(!warning.update(59999, true, true, false));
  assert(warning.update(60000, true, true, false));
  assert(warning.update(120000, true, true, false));
  // A successful HA subscription dismisses the warning and resets the grace.
  assert(!warning.update(120001, true, true, true));
  assert(!warning.update(120002, true, true, false));
  assert(!warning.update(180001, true, true, false));
  assert(warning.update(180002, true, true, false));
  // Network outages dismiss it; network recovery receives a fresh grace.
  assert(!warning.update(180003, false, true, false));
  assert(!warning.update(250000, true, true, false));
  assert(!warning.update(309999, true, true, false));
  assert(warning.update(310000, true, true, false));
  // First-time setup never gets a connection-repair warning.
  assert(!warning.update(400000, true, false, false));
  assert(!warning.update(500000, true, false, false));
  // The millis clock wraps after about 49 days.
  HaConnectionWarning wrapped;
  assert(!wrapped.update(UINT32_MAX - 10000, true, true, false));
  assert(!wrapped.update(49998, true, true, false));
  assert(wrapped.update(49999, true, true, false));

  // Warning stays visible above all ordinary sleep/screensaver policies.
  for (auto source : {DisplayRequestSource::MANUAL_SLEEP,
                      DisplayRequestSource::SCREEN_SCHEDULE,
                      DisplayRequestSource::IDLE_TIMER,
                      DisplayRequestSource::PRESENCE_SENSOR,
                      DisplayRequestSource::BOOT_GUARD}) {
    DisplayModeController display;
    assert(display.request(source, DisplayMode::DISPLAY_OFF));
    assert(display.request(DisplayRequestSource::HA_CONNECTION, DisplayMode::SETUP_DIMMED));
    assert(display.resolve().target_mode == DisplayMode::SETUP_DIMMED);
    assert(display.resolve().winning_source == DisplayRequestSource::HA_CONNECTION);
    // Safety alarms still take priority.
    display.begin_takeover(DisplayTakeoverKind::CRITICAL);
    assert(display.resolve().target_mode == DisplayMode::ACTIVE);
    display.end_takeover(DisplayTakeoverKind::CRITICAL);
    assert(display.resolve().target_mode == DisplayMode::SETUP_DIMMED);
    // Dismissal restores the saved sleep policy.
    display.clear(DisplayRequestSource::HA_CONNECTION);
    assert(display.resolve().target_mode == DisplayMode::DISPLAY_OFF);
  }
}
