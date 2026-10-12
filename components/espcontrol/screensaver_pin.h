#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include "display_mode_controller.h"

namespace espcontrol {
// Local touchscreen policy. The web API exposes status and PIN length, never the PIN.
class ScreensaverPin {
  using Guard = std::lock_guard<std::mutex>;
 public:
  void setup();
  bool ready() const { Guard guard(mutex_); return ready_; }
  bool enabled() const { Guard guard(mutex_); return enabled_; }
  bool has_pin() const { Guard guard(mutex_); return !pin_.empty(); }
  std::size_t pin_length() const { Guard guard(mutex_); return pin_.size(); }
  bool randomize_keypad() const { Guard guard(mutex_); return randomize_keypad_; }
  bool locked() const { Guard guard(mutex_); return (locked_ || manual_locked_) && !pin_.empty(); }
  bool keypad_requested() const { Guard guard(mutex_); return (locked_ || manual_locked_) && !wait_for_touch_; }
  DisplayMode return_mode() const { Guard guard(mutex_); return return_mode_; }
  void dismiss_keypad() { Guard guard(mutex_); wait_for_touch_ = true; }
  void request_keypad() { Guard guard(mutex_); wait_for_touch_ = false; }
  bool lock(bool wait_for_touch = false) {
    Guard guard(mutex_);
    if (!ready_ || pin_.empty()) return false;
    manual_locked_ = true;
    wait_for_touch_ = wait_for_touch;
    return_mode_ = DisplayMode::ACTIVE;
    return true;
  }
  void arm(DisplayMode mode = DisplayMode::ACTIVE) {
    Guard guard(mutex_);
    sleeping_ = true;
    wait_for_touch_ = false;
    return_mode_ = mode;
    locked_ = enabled_ && !pin_.empty();
  }
  void wake() {
    Guard guard(mutex_);
    if (!sleeping_) return;
    sleeping_ = false;
    locked_ = enabled_ && !pin_.empty();
  }
  bool submit(const std::string &entry) {
    Guard guard(mutex_);
    if ((!locked_ && !manual_locked_) || pin_.empty()) return true;
    if (entry != pin_) return false;
    locked_ = false;
    manual_locked_ = false;
    wait_for_touch_ = false;
    return_mode_ = DisplayMode::ACTIVE;
    return true;
  }
  bool save(bool enabled, const std::string *pin, const bool *randomize_keypad = nullptr);
  static bool valid_pin(const std::string &pin) {
    if (pin.size() > 16) return false;
    for (char c : pin) if (c < '0' || c > '9') return false;
    return true;
  }
 private:
  // HTTP handlers run on the server task; PIN entry runs on the display loop.
  mutable std::mutex mutex_;
  std::string pin_;
  bool enabled_{false}, locked_{false}, sleeping_{false}, ready_{false};
  bool manual_locked_{false};
  bool wait_for_touch_{false};
  DisplayMode return_mode_{DisplayMode::ACTIVE};
  bool randomize_keypad_{false};
};
ScreensaverPin &screensaver_pin();
}  // namespace espcontrol
