#pragma once

#include <cstdint>

#include "display_mode_controller.h"

namespace espcontrol {

enum class DisplayLifecycleState : uint8_t {
  CONSTRUCTED,
  RUNNING,
  STOPPED,
};

// Owns display-mode state independently of the ESPHome component lifecycle.
// The controller accessor remains available while YAML wiring migrates through
// this service boundary.
class DisplayLifecycleService {
 public:
  bool start() {
    if (state_ != DisplayLifecycleState::CONSTRUCTED) return false;
    state_ = DisplayLifecycleState::RUNNING;
    return true;
  }

  bool run_once() {
    if (state_ != DisplayLifecycleState::RUNNING) return false;
    ++loop_count_;
    return true;
  }

  bool stop() {
    if (state_ != DisplayLifecycleState::RUNNING) return false;
    state_ = DisplayLifecycleState::STOPPED;
    return true;
  }

  DisplayLifecycleState state() const { return state_; }
  uint32_t loop_count() const { return loop_count_; }
  DisplayModeController &controller() { return controller_; }
  const DisplayModeController &controller() const { return controller_; }

 private:
  DisplayLifecycleState state_{DisplayLifecycleState::CONSTRUCTED};
  uint32_t loop_count_{0};
  DisplayModeController controller_{};
};

}  // namespace espcontrol
