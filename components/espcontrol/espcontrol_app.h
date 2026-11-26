#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <memory>

#include "esphome/core/component.h"

#include "espcontrol_app_core.h"
#include "panel_identity.h"
#include "home_assistant_endpoint_resolver.h"
namespace espcontrol {

namespace configuration {
class PanelConfigField;
}

// The main ESPHome component boundary for EspControl runtime services.
// PanelIdentity runs separately because network identity must be restored earlier.
// YAML remains a compatibility/wiring layer and accesses services through this
// owner while behaviour moves into compiled modules.
class EspControlApp : public esphome::Component {
 public:
  static constexpr size_t PANEL_CONFIG_STORAGE_SLOT_CAPACITY = 40 * 1024;
  static constexpr size_t PANEL_CONFIG_NVS_SLOT_CAPACITY = 16 * 1024;

  EspControlApp();
  ~EspControlApp();

  void setup() override;
  void loop() override;
  void on_safe_shutdown() override;
  void on_shutdown() override;
  float get_setup_priority() const override {
    // The native configuration wiring can refresh the display.
    // P4 display services are only
    // ready once Wifi setup has completed. Starting the owner earlier makes
    // P4 firmware reset before ESPHome can confirm a new OTA boot.
    return esphome::setup_priority::AFTER_WIFI;
  }

  DisplayModeController &display() { return core_.display(); }
  const DisplayModeController &display() const { return core_.display(); }
  AppLifecycleState lifecycle_state() const { return core_.lifecycle_state(); }
  HomeAssistantEndpointResolver &home_assistant_endpoint() {
    return home_assistant_endpoint_;
  }

  void set_panel_config_device_profile(const char *device_profile);
  void set_panel_config_button_order(configuration::PanelConfigField *button_order);
  void set_panel_config_button_on_color(configuration::PanelConfigField *button_on_color);
  void set_panel_config_button(
      uint8_t slot, configuration::PanelConfigField *button,
      configuration::PanelConfigField *subpage_0, configuration::PanelConfigField *subpage_1,
      configuration::PanelConfigField *subpage_2, configuration::PanelConfigField *subpage_3,
      configuration::PanelConfigField *subpage_4 = nullptr,
      configuration::PanelConfigField *subpage_5 = nullptr,
      configuration::PanelConfigField *subpage_6 = nullptr,
      configuration::PanelConfigField *subpage_7 = nullptr);
  // Codepoints added to font_icon_main beyond the built-in icons (from codegen).
  void set_custom_icons(std::initializer_list<uint32_t> codepoints);
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
  bool native_configuration_requested() const;
  bool create_native_configuration_runtime();

  struct PanelConfigTextSources {
    configuration::PanelConfigField *button{nullptr};
    std::array<configuration::PanelConfigField *, 8> subpages{};
  };

  const char *panel_config_device_profile_{nullptr};
  configuration::PanelConfigField *panel_config_button_order_{nullptr};
  configuration::PanelConfigField *panel_config_button_on_color_{nullptr};
  std::array<PanelConfigTextSources, 32> panel_config_button_texts_{};
  bool panel_config_card_images_storage_{false};
  // This owns every non-trivial configuration object. Keeping it separate
  // leaves the ESPHome-created application object safe to construct before
  // the framework's allocators and component setup are ready.
  std::unique_ptr<NativeConfigurationRuntime> native_configuration_runtime_;
  EspControlAppCore core_{};
  HomeAssistantEndpointResolver home_assistant_endpoint_{};
  bool native_configuration_initialized_{false};
  bool panel_config_http_context_bound_{false};
  const char *web_auth_username_{nullptr};
  const char *web_auth_password_{nullptr};
};

}  // namespace espcontrol
