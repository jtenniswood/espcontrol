#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "esphome/components/text/text.h"
#include "esphome/core/component.h"

#include "card_asset_service.h"
#include "espcontrol_app_core.h"
#include "home_assistant_endpoint_resolver.h"
namespace espcontrol {

// The single ESPHome component boundary for EspControl-owned firmware state.
// YAML remains a compatibility/wiring layer and accesses services through this
// owner while behaviour moves into compiled modules.
class EspControlApp : public esphome::Component {
 public:
  static constexpr size_t PANEL_CONFIG_STORAGE_SLOT_CAPACITY = 40 * 1024;

  EspControlApp();
  ~EspControlApp();

  void setup() override;
  void loop() override;
  void on_shutdown() override;
  float get_setup_priority() const override {
    // The native configuration wiring binds restored ESPHome text entities.
    // Those entities, and the P4 display services they can refresh, are only
    // ready once Wi-Fi setup has completed. Starting the owner earlier makes
    // P4 firmware reset before ESPHome can confirm a new OTA boot.
    return esphome::setup_priority::AFTER_WIFI;
  }

  DisplayModeController &display() { return core_.display(); }
  const DisplayModeController &display() const { return core_.display(); }
  CardAssetService &card_assets() { return card_assets_; }
  const CardAssetService &card_assets() const { return card_assets_; }
  AppLifecycleState lifecycle_state() const { return core_.lifecycle_state(); }
  HomeAssistantEndpointResolver &home_assistant_endpoint() {
    return home_assistant_endpoint_;
  }

  void set_panel_config_device_profile(const char *device_profile);
  void set_panel_config_button_order(esphome::text::Text *button_order);
  void set_panel_config_button_on_color(esphome::text::Text *button_on_color);
  void set_panel_config_button(
      uint8_t slot, esphome::text::Text *button,
      esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
      esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
      esphome::text::Text *subpage_4, esphome::text::Text *subpage_5,
      esphome::text::Text *subpage_6, esphome::text::Text *subpage_7);
  void set_panel_config_card_images_storage(bool enabled) {
    panel_config_card_images_storage_ = enabled;
  }
  void set_web_auth_credentials(const char *username, const char *password) {
    web_auth_username_ = username;
    web_auth_password_ = password;
  }

 private:
  class NativeConfigurationRuntime;

  void register_panel_config_endpoints();
  void initialize_native_configuration();
  void apply_boot_configuration();
  static bool persist_card_asset_references(void *context);
  static CardAssetReferenceState check_recovery_references(void *context, const std::string &id);
  bool native_configuration_requested() const;
  bool create_native_configuration_runtime();

  struct PanelConfigTextSources {
    esphome::text::Text *button{nullptr};
    std::array<esphome::text::Text *, 8> subpages{};
  };

  const char *panel_config_device_profile_{nullptr};
  esphome::text::Text *panel_config_button_order_{nullptr};
  esphome::text::Text *panel_config_button_on_color_{nullptr};
  std::array<PanelConfigTextSources, 32> panel_config_button_texts_{};
  bool panel_config_card_images_storage_{false};
  // This owns every non-trivial configuration object. Keeping it separate
  // leaves the ESPHome-created application object safe to construct before
  // the framework's allocators and component setup are ready.
  std::unique_ptr<NativeConfigurationRuntime> native_configuration_runtime_;
  EspControlAppCore core_{};
  CardAssetService card_assets_{};
  HomeAssistantEndpointResolver home_assistant_endpoint_{};
  bool native_configuration_initialized_{false};
  bool native_configuration_applied_{false};
  bool panel_config_http_context_bound_{false};
  const char *web_auth_username_{nullptr};
  const char *web_auth_password_{nullptr};
};

}  // namespace espcontrol
