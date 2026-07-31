#include <cstdlib>

#include "espcontrol_app_core.h"

using espcontrol::AppLifecycleState;
using espcontrol::DisplayMode;
using espcontrol::DisplayRequestSource;
using espcontrol::EspControlAppCore;

int main() {
  EspControlAppCore app;
  if (app.lifecycle_state() != AppLifecycleState::CONSTRUCTED) return EXIT_FAILURE;
  if (app.run_once() || app.stop()) return EXIT_FAILURE;
  if (!app.start() || app.start()) return EXIT_FAILURE;
  if (app.lifecycle_state() != AppLifecycleState::RUNNING) return EXIT_FAILURE;

  auto &display = app.display();
  if (!display.request(DisplayRequestSource::MANUAL_SLEEP,
                       DisplayMode::DISPLAY_OFF)) {
    return EXIT_FAILURE;
  }
  if (!app.display().target_mode_is(DisplayMode::DISPLAY_OFF)) {
    return EXIT_FAILURE;
  }

  if (!app.run_once() || !app.run_once() || app.loop_count() != 2) {
    return EXIT_FAILURE;
  }
  if (!app.stop() || app.stop() || app.run_once()) return EXIT_FAILURE;
  if (app.lifecycle_state() != AppLifecycleState::STOPPED) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
