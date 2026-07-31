#pragma once

#include <cstdint>

#include "display_mode_controller.h"

namespace espcontrol {

enum class AppLifecycleState : uint8_t {
  CONSTRUCTED,
  RUNNING,
  STOPPED,
};

// Framework-independent owner for EspControl's long-lived firmware services.
// Keeping this core free of ESPHome APIs makes lifecycle and ownership
// executable in host tests.
class EspControlAppCore {
 public:
  bool start();
  bool run_once();
  bool stop();

  AppLifecycleState lifecycle_state() const { return lifecycle_state_; }
  uint32_t loop_count() const { return loop_count_; }

  DisplayModeController &display() { return display_; }
  const DisplayModeController &display() const { return display_; }

 private:
  AppLifecycleState lifecycle_state_{AppLifecycleState::CONSTRUCTED};
  uint32_t loop_count_{0};
  DisplayModeController display_{};
};

}  // namespace espcontrol
