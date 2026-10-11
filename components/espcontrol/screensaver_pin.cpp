#include "screensaver_pin.h"
#include <cstring>
#include <nvs.h>

namespace espcontrol {
namespace {
struct PinRecord {
  uint32_t version{1};
  char pin[17]{};
  uint8_t enabled{0};
};
constexpr char PIN_NAMESPACE[] = "espcontrol_pin";
}
ScreensaverPin &screensaver_pin() {
  static ScreensaverPin settings;
  return settings;
}
void ScreensaverPin::setup() {
  Guard guard(mutex_);
  nvs_handle_t handle;
  if (nvs_open(PIN_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;
  PinRecord record;
  size_t size = sizeof(record);
  const auto result = nvs_get_blob(handle, "settings", &record, &size);
  nvs_close(handle);
  if (result == ESP_OK) {
    if (size != sizeof(record) || record.version != 1 || record.enabled > 1 ||
        std::memchr(record.pin, 0, sizeof(record.pin)) == nullptr ||
        !valid_pin(record.pin)) return;
    pin_ = record.pin;
    enabled_ = record.enabled;
  } else if (result != ESP_ERR_NVS_NOT_FOUND) {
    return;
  }
  ready_ = true;
  locked_ = enabled_ && !pin_.empty();
}
bool ScreensaverPin::save(bool enabled, const std::string *pin) {
  Guard guard(mutex_);
  if (!ready_ || (pin && !valid_pin(*pin))) return false;
  const auto &next_pin = pin ? *pin : pin_;
  PinRecord record;
  std::memcpy(record.pin, next_pin.data(), next_pin.size());
  record.enabled = enabled;
  nvs_handle_t handle;
  if (nvs_open(PIN_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
  auto result = nvs_set_blob(handle, "settings", &record, sizeof(record));
  if (result == ESP_OK) result = nvs_commit(handle);
  nvs_close(handle);
  if (result != ESP_OK) return false;
  pin_ = next_pin;
  enabled_ = enabled;
  if (!enabled_ || pin_.empty()) locked_ = false;
  else if (sleeping_) locked_ = true;
  return true;
}
}  // namespace espcontrol
