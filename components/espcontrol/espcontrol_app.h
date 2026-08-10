#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

#include "esphome/components/text/text.h"
#include "esphome/core/component.h"

#include "espcontrol_app_core.h"
#include "configuration_service.h"
#include "configuration_store.h"
#include "panel_config_espidf_storage.h"
#include "panel_config_esphome_text.h"
#include "panel_config_legacy_adapter.h"
#include "panel_config_service_validator.h"
#include "panel_config_storage_backend.h"

namespace espcontrol {

// The single ESPHome component boundary for EspControl-owned firmware state.
// YAML remains a compatibility/wiring layer and accesses services through this
// owner while behaviour moves into compiled modules.
class EspControlApp : public esphome::Component {
 public:
  static constexpr size_t PANEL_CONFIG_STORAGE_SLOT_CAPACITY = 40 * 1024;

  void setup() override;
  void loop() override;
  void on_shutdown() override;
  float get_setup_priority() const override {
    // Cover-art boot automation resets Home Assistant subscriptions at 250.
    // Start the core just before WiFi (251) so it always owns that state.
    return esphome::setup_priority::WIFI + 1.0f;
  }

  DisplayModeController &display() { return core_.display(); }
  const DisplayModeController &display() const { return core_.display(); }
  AppLifecycleState lifecycle_state() const { return core_.lifecycle_state(); }

  void set_panel_config_device_profile(const char *device_profile);
  void set_panel_config_button_order(esphome::text::Text *button_order);
  void set_panel_config_button_on_color(esphome::text::Text *button_on_color);
  void set_panel_config_button(
      uint8_t slot, esphome::text::Text *button,
      esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
      esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
      esphome::text::Text *subpage_4, esphome::text::Text *subpage_5,
      esphome::text::Text *subpage_6, esphome::text::Text *subpage_7);
  void set_web_auth_credentials(const char *username, const char *password) {
    web_auth_username_ = username == nullptr ? "" : username;
    web_auth_password_ = password == nullptr ? "" : password;
  }

 private:
  void register_panel_config_endpoints();
  void apply_boot_configuration();

  struct LegacyButtonTextSources {
    configuration::EspHomeLegacyTextValue button;
    std::array<configuration::EspHomeLegacyTextValue,
               configuration::PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
        subpages{};
  };

  configuration::PanelConfigLegacyAdapter legacy_config_{};
  configuration::PanelConfigDocumentValidator panel_config_validator_{};
  configuration::EspIdfPanelConfigBlobStorage panel_config_blobs_{};
  configuration::BufferedBlobStorageBackend<PANEL_CONFIG_STORAGE_SLOT_CAPACITY>
      panel_config_backend_{panel_config_blobs_};
  configuration::ConfigurationStore panel_config_store_{panel_config_backend_};
  EspControlAppCore core_{};
  uint8_t *panel_config_memory_{nullptr};
  uint8_t *panel_config_document_buffer_{nullptr};
  uint8_t *boot_configuration_buffer_{nullptr};
  bool boot_configuration_pending_{false};
  std::string web_auth_username_;
  std::string web_auth_password_;
  configuration::EspHomeLegacyTextValue button_order_text_{};
  configuration::EspHomeLegacyTextValue button_on_color_text_{};
  std::array<LegacyButtonTextSources, configuration::PANEL_CONFIG_MAX_SLOT_COUNT>
      legacy_button_texts_{};
};

}  // namespace espcontrol
