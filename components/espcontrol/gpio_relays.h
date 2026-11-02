#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace espcontrol {
// GPIO assignments belong to this physical panel, never to a portable card backup.
struct GpioRelaySettings {
  uint32_t version{1};
  uint8_t enabled{0};
  struct Control {
    int8_t pin{-1};
    uint8_t inverted{0};
    char name[33]{};
  };
  std::array<Control, 3> controls{};
};
class GpioRelays {
 public:
  void setup(const char *profile);
  bool supported() const { std::lock_guard<std::mutex> guard(mutex_); return supported_; }
  bool ready() const { std::lock_guard<std::mutex> guard(mutex_); return ready_; }
  GpioRelaySettings settings() const { std::lock_guard<std::mutex> guard(mutex_); return settings_; }
  static bool valid(const GpioRelaySettings &settings);
  bool save(const GpioRelaySettings &settings);
  void set(size_t index, bool on, bool pulse = false);
  bool state(size_t index) const;
  void loop(uint32_t now, const std::function<void(size_t, bool)> &notify);
  void shutdown();
 private:
  void release_();
  mutable std::mutex mutex_;
  GpioRelaySettings settings_{};
  std::array<int8_t, 3> pins_{{-1, -1, -1}};
  std::array<bool, 3> inverted_{}, states_{}, pulsing_{}, dirty_{};
  std::array<uint32_t, 3> pulse_started_{};
  bool supported_{false}, ready_{false}, pending_{false};
};
GpioRelays &gpio_relays();
}  // namespace espcontrol
