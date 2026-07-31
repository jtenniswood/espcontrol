#include "espcontrol_app_core.h"

namespace espcontrol {

bool EspControlAppCore::start() {
  if (lifecycle_state_ != AppLifecycleState::CONSTRUCTED) return false;
  lifecycle_state_ = AppLifecycleState::RUNNING;
  return true;
}

bool EspControlAppCore::run_once() {
  if (lifecycle_state_ != AppLifecycleState::RUNNING) return false;
  ++loop_count_;
  return true;
}

bool EspControlAppCore::stop() {
  if (lifecycle_state_ != AppLifecycleState::RUNNING) return false;
  lifecycle_state_ = AppLifecycleState::STOPPED;
  return true;
}

}  // namespace espcontrol
