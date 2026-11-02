#include "gpio_relays.h"
#include <cstring>
#include <nvs.h>
#include <driver/gpio.h>
#include "esphome/core/hal.h"

namespace espcontrol {
GpioRelays &gpio_relays() { static GpioRelays service; return service; }
bool GpioRelays::valid(const GpioRelaySettings &settings) {
  if (settings.version != 1 || settings.enabled > 1) return false;
  unsigned used = 0;
  for (const auto &control : settings.controls) {
    if (control.inverted > 1 || std::memchr(control.name, 0, sizeof(control.name)) == nullptr) return false;
    if (control.pin == -1) continue;
    // Only explicitly supported expansion pins on the JC1060P470. Never accept
    // arbitrary chip GPIOs: display, storage and network pins are already owned.
    if (control.pin < 1 || control.pin > 3 || (used & (1u << control.pin))) return false;
    used |= 1u << control.pin;
  }
  return !settings.enabled || used != 0;
}
void GpioRelays::setup(const char *profile) {
  std::lock_guard<std::mutex> guard(mutex_);
  supported_ = profile && (std::strcmp(profile, "guition-esp32-p4-jc1060p470") == 0 ||
                          std::strcmp(profile, "guition-esp32-p4-jc1060p470-v2") == 0);
  if (!supported_) return;
  nvs_handle_t handle;
  if (nvs_open("espcontrol_gpio", NVS_READWRITE, &handle) != ESP_OK) return;
  GpioRelaySettings loaded;
  size_t size = sizeof(loaded);
  const auto result = nvs_get_blob(handle, "settings", &loaded, &size);
  nvs_close(handle);
  if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND &&
      result != ESP_ERR_NVS_INVALID_LENGTH && result != ESP_ERR_NVS_TYPE_MISMATCH) return;
  settings_ = result == ESP_OK && size == sizeof(loaded) && valid(loaded) ? loaded : GpioRelaySettings{};
  ready_ = true;
  pending_ = true;
}
bool GpioRelays::save(const GpioRelaySettings &settings) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (!supported_ || !ready_ || !valid(settings)) return false;
  nvs_handle_t handle;
  if (nvs_open("espcontrol_gpio", NVS_READWRITE, &handle) != ESP_OK) return false;
  auto result = nvs_set_blob(handle, "settings", &settings, sizeof(settings));
  if (result == ESP_OK) result = nvs_commit(handle);
  nvs_close(handle);
  if (result != ESP_OK) return false;
  settings_ = settings;
  pending_ = true; // GPIO and LVGL work must run on the application loop.
  return true;
}
void GpioRelays::release_() {
  for (size_t i = 0; i < pins_.size(); ++i) {
    if (pins_[i] >= 0) {
      gpio_set_level(static_cast<gpio_num_t>(pins_[i]), inverted_[i] ? 1 : 0);
      gpio_reset_pin(static_cast<gpio_num_t>(pins_[i]));
    }
    pins_[i] = -1;
    states_[i] = pulsing_[i] = false;
    dirty_[i] = true;
  }
}
void GpioRelays::shutdown() { std::lock_guard<std::mutex> guard(mutex_); release_(); }
void GpioRelays::set(size_t index, bool on, bool pulse) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (!ready_ || pending_ || !settings_.enabled || index >= pins_.size() || pins_[index] < 0) return;
  if (gpio_set_level(static_cast<gpio_num_t>(pins_[index]), on != inverted_[index]) != ESP_OK) return;
  states_[index] = on;
  pulsing_[index] = on && pulse;
  pulse_started_[index] = esphome::millis();
  dirty_[index] = true;
}
bool GpioRelays::state(size_t index) const {
  std::lock_guard<std::mutex> guard(mutex_);
  return !pending_ && settings_.enabled && index < states_.size() && states_[index];
}
void GpioRelays::loop(uint32_t now, const std::function<void(size_t, bool)> &notify) {
  std::array<bool, 3> dirty, states;
  {
    std::lock_guard<std::mutex> guard(mutex_);
    if (!ready_) return;
    if (pending_) {
      release_();
      if (settings_.enabled) {
        for (size_t i = 0; i < pins_.size(); ++i) {
          const auto &control = settings_.controls[i];
          if (control.pin < 0) continue;
          const auto pin = static_cast<gpio_num_t>(control.pin);
          // Set the inactive latch before enabling output, including active-low relays.
          if (gpio_set_level(pin, control.inverted ? 1 : 0) == ESP_OK &&
              gpio_set_direction(pin, GPIO_MODE_OUTPUT) == ESP_OK) {
            pins_[i] = control.pin;
            inverted_[i] = control.inverted;
          } else { gpio_reset_pin(pin); }
        }
      }
      pending_ = false;
    }
    for (size_t i = 0; i < pins_.size(); ++i) {
      if (pulsing_[i] && static_cast<uint32_t>(now - pulse_started_[i]) >= 200) {
        if (gpio_set_level(static_cast<gpio_num_t>(pins_[i]), inverted_[i] ? 1 : 0) == ESP_OK) {
          states_[i] = pulsing_[i] = false;
          dirty_[i] = true;
        }
      }
    }
    dirty = dirty_; states = states_; dirty_.fill(false);
  }
  for (size_t i = 0; i < dirty.size(); ++i) if (dirty[i]) notify(i, states[i]);
}
}  // namespace espcontrol
