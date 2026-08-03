#if defined(USE_ESP32_VARIANT_ESP32P4)

#include "c6_recovery.h"

#include <algorithm>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <tuple>

#include "esphome/components/sha256/sha256.h"
#include "esphome/components/watchdog/watchdog.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"

#include <esp_hosted.h>
#include <esp_ota_ops.h>

extern "C" {
#include <esp_hosted_ota.h>
}

namespace esphome::c6_recovery {

static const char *const TAG = "c6_recovery";
static constexpr size_t CHUNK_SIZE = 1500;

static bool parse_version(const std::string &value, std::tuple<int, int, int> &version) {
  int major = 0;
  int minor = 0;
  int patch = 0;
  char trailing = '\0';
  if (sscanf(value.c_str(), "%d.%d.%d%c", &major, &minor, &patch, &trailing) != 3) {
    return false;
  }
  version = {major, minor, patch};
  return true;
}

void C6RecoveryComponent::setup() {
  const esp_err_t connection_error = esp_hosted_connect_to_slave();  // NOLINT
  if (connection_error != ESP_OK) {
    this->mark_failed();
    ESP_LOGE(TAG,
             "Could not connect to the ESP32-C6 before WiFi startup: %s; "
             "reconnect USB and run recovery again",
             esp_err_to_name(connection_error));
    return;
  }

  esp_hosted_coprocessor_fwver_t version_info;
  std::string current = "unknown";
  if (esp_hosted_get_coprocessor_fwversion(&version_info) == ESP_OK) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%" PRIu32 ".%" PRIu32 ".%" PRIu32,
             version_info.major1, version_info.minor1, version_info.patch1);
    current = buffer;
  }

  ESP_LOGI(TAG, "Current ESP32-C6 firmware: %s", current.c_str());
  ESP_LOGI(TAG, "Recovery ESP32-C6 firmware: %s", this->target_version_.c_str());

  if (!this->should_install_(current)) {
    ESP_LOGI(TAG, "ESP32-C6 recovery is not required");
    return;
  }

  ESP_LOGW(TAG, "Installing offline ESP32-C6 recovery firmware");
  if (!this->verify_firmware_() || !this->install_firmware_()) {
    this->mark_failed();
    ESP_LOGE(TAG, "ESP32-C6 recovery failed; reconnect USB and run recovery again");
    return;
  }

  ESP_LOGI(TAG, "ESP32-C6 recovery completed successfully");
#ifdef USE_OTA_ROLLBACK
  esp_ota_mark_app_valid_cancel_rollback();
#endif
  this->set_timeout(1000, []() { App.safe_reboot(); });
}

void C6RecoveryComponent::dump_config() {
  ESP_LOGCONFIG(TAG,
                "ESP32-C6 Offline Recovery:\n"
                "  Target version: %s\n"
                "  Embedded firmware size: %zu bytes",
                this->target_version_.c_str(), this->firmware_size_);
}

bool C6RecoveryComponent::should_install_(const std::string &current) const {
  std::tuple<int, int, int> current_version;
  std::tuple<int, int, int> target_version;
  if (!parse_version(this->target_version_, target_version)) {
    ESP_LOGE(TAG, "Invalid recovery target version: %s", this->target_version_.c_str());
    return false;
  }
  if (!parse_version(current, current_version)) {
    ESP_LOGW(TAG, "Could not read the ESP32-C6 version; attempting recovery");
    return true;
  }
  if (current_version > target_version) {
    ESP_LOGI(TAG, "Installed ESP32-C6 firmware is newer; recovery will not downgrade it");
    return false;
  }
  return current_version < target_version;
}

bool C6RecoveryComponent::verify_firmware_() {
  if (this->firmware_data_ == nullptr || this->firmware_size_ == 0) {
    ESP_LOGE(TAG, "Embedded ESP32-C6 firmware is missing");
    return false;
  }
  sha256::SHA256 hasher;
  hasher.init();
  hasher.add(this->firmware_data_, this->firmware_size_);
  hasher.calculate();
  if (!hasher.equals_bytes(this->firmware_sha256_.data())) {
    ESP_LOGE(TAG, "Embedded ESP32-C6 firmware checksum verification failed");
    return false;
  }
  return true;
}

bool C6RecoveryComponent::install_firmware_() {
  watchdog::WatchdogManager watchdog(60000);
  esp_err_t error = esp_hosted_slave_ota_begin();  // NOLINT
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Could not start ESP32-C6 recovery: %s", esp_err_to_name(error));
    return false;
  }

  uint8_t chunk[CHUNK_SIZE];
  const uint8_t *source = this->firmware_data_;
  size_t remaining = this->firmware_size_;
  size_t written = 0;
  unsigned last_progress = 0;
  while (remaining > 0) {
    const size_t chunk_size = std::min(remaining, CHUNK_SIZE);
    memcpy(chunk, source, chunk_size);
    error = esp_hosted_slave_ota_write(chunk, chunk_size);  // NOLINT
    if (error != ESP_OK) {
      ESP_LOGE(TAG, "ESP32-C6 recovery write failed: %s", esp_err_to_name(error));
      esp_hosted_slave_ota_end();  // NOLINT
      return false;
    }
    source += chunk_size;
    remaining -= chunk_size;
    written += chunk_size;
    const unsigned progress =
        static_cast<unsigned>((written * 100U) / this->firmware_size_);
    if (progress >= last_progress + 10 || progress == 100) {
      ESP_LOGI(TAG, "ESP32-C6 recovery progress: %u%%", progress);
      last_progress = progress;
    }
    App.feed_wdt();
    yield();
  }

  error = esp_hosted_slave_ota_end();  // NOLINT
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Could not finalise ESP32-C6 recovery: %s", esp_err_to_name(error));
    return false;
  }
  error = esp_hosted_slave_ota_activate();  // NOLINT
  if (error != ESP_OK) {
    ESP_LOGE(TAG, "Could not activate ESP32-C6 recovery firmware: %s",
             esp_err_to_name(error));
    return false;
  }
  return true;
}

}  // namespace esphome::c6_recovery

#endif
