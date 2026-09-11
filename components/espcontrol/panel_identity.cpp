#include "panel_identity.h"

#include <cstring>
#include <nvs.h>
#include "esphome/core/application.h"
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#include "esphome/core/version.h"

// This adapter relies on 2026.8.2's non-const Application StringRef members,
// exposed as const references by the public getters. Review at ESPHome upgrades.
static_assert(ESPHOME_VERSION_CODE == VERSION_CODE(2026, 8, 2),
              "Review panel identity startup adapter for this ESPHome version");
namespace espcontrol {
PanelIdentity *panel_identity = nullptr;
namespace {
constexpr char NAMESPACE[] = "espcontrol_id";
constexpr char KEY[] = "identity";
struct IdentityRecord {
  uint32_t version{1};
  char name[PANEL_NAME_MAX_BYTES + 1]{};
};
}

void PanelIdentity::setup() {
  panel_identity = this;
  default_hostname_ = esphome::App.get_name().c_str();
  default_friendly_ = esphome::App.get_friendly_name().c_str();
  if (default_friendly_.empty()) default_friendly_ = default_hostname_;
  char mac[esphome::MAC_ADDRESS_BUFFER_SIZE];
  esphome::get_mac_address_into_buffer(mac);
  suffix_ = std::string(mac + 6, 6);
  nvs_handle_t handle;
  if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
    ESP_LOGE("espcontrol.identity", "Name storage unavailable; using firmware defaults");
    return;
  }
  IdentityRecord record;
  size_t length = sizeof(record);
  const esp_err_t result = nvs_get_blob(handle, KEY, &record, &length);
  nvs_close(handle);
  if (result == ESP_OK && length == sizeof(record) && record.version == 1 &&
      std::memchr(record.name, 0, sizeof(record.name)) != nullptr) {
    if (!normalize_panel_name(record.name, saved_name_)) saved_name_.clear();
  } else if (result != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGW("espcontrol.identity", "Invalid saved name; using firmware defaults");
  }
  boot_name_ = saved_name_;
  if (!boot_name_.empty()) {
    running_hostname_ = target_hostname();
    running_friendly_ = target_name();
    // Entity registration has already copied its original names/hashes. Rebind
    // only the application references, before any network/API component starts.
    // The underlying members are mutable; do not cast/write the string buffers.
    const_cast<esphome::StringRef &>(esphome::App.get_name()) =
        esphome::StringRef(running_hostname_.c_str(), running_hostname_.size());
    const_cast<esphome::StringRef &>(esphome::App.get_friendly_name()) =
        esphome::StringRef(running_friendly_.c_str(), running_friendly_.size());
  }
  ready_ = true;
}

bool PanelIdentity::save(const std::string &name) {
  storage_error_ = ESP_OK;
  std::string normalized;
  if (!ready_ || !normalize_panel_name(name, normalized)) return false;
  if (normalized == saved_name_) return true;
  IdentityRecord record;
  std::memcpy(record.name, normalized.data(), normalized.size());
  nvs_handle_t handle;
  esp_err_t result = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
  if (result != ESP_OK) {
    storage_error_ = result;
    ESP_LOGE("espcontrol.identity", "Name storage open failed: %d", result);
    return false;
  }
  result = nvs_set_blob(handle, KEY, &record, sizeof(record));
  if (result == ESP_OK) result = nvs_commit(handle);
  nvs_close(handle);
  if (result != ESP_OK) {
    storage_error_ = result;
    ESP_LOGE("espcontrol.identity", "Name storage write failed: %d", result);
    return false;
  }
  saved_name_ = normalized;
  return true;
}
}  // namespace espcontrol
