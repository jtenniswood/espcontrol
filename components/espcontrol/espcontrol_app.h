#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>

#include "application_scene_runtime.h"
#include "configuration_buffer.h"
#include "espcontrol_app_core.h"
#include "esphome/components/text/text.h"
#include "esphome/core/component.h"
#include "home_assistant_endpoint_resolver.h"
#include "panel_config_text_storage.h"
#include "panel_identity.h"
namespace esphome::template_ {
class TemplateText;
}

namespace espcontrol {

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
    // The native configuration wiring binds restored ESPHome text entities.
    // Those entities, and the P4 display services they can refresh, are only
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

  // The scene belongs to the firmware application's UI owner and outlives this
  // component. Installation cannot switch a running legacy grid underneath it.
  bool install_application_scene(ApplicationSceneRuntime& scene);
  // Selected device startup calls this before reserving native UI resources.
  // A failed allocation then refuses writes instead of silently using legacy
  // UI.
  bool require_application_scene();
  // Existing text-entity automations debounce edits before requesting this.
  // The application loop performs all model/storage work at a safe boundary.
  void request_configuration_text_refresh() {
    configuration_text_refresh_pending_ = true;
  }
  void set_panel_config_device_profile(const char *device_profile);
  void set_panel_config_button_order(esphome::text::Text *button_order);
  void set_panel_config_button_on_color(esphome::text::Text *button_on_color);
  // Code generation marks only restore-enabled TemplateText fields whose
  // configured initial value is empty. Missing preferences are equivalent to
  // empty values for those fields, including after a firmware downgrade.
  void set_panel_config_text_empty_default(esphome::text::Text *text);
  void set_panel_config_button(
      uint8_t slot, esphome::text::Text *button,
      esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
      esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
      esphome::text::Text *subpage_4 = nullptr,
      esphome::text::Text *subpage_5 = nullptr,
      esphome::text::Text *subpage_6 = nullptr,
      esphome::text::Text *subpage_7 = nullptr);
  // Preserve the concrete type supplied by ESPHome code generation. Only
  // TemplateText uses the SDK preference format; custom Text platforms retain
  // their normal control path.
  template <typename Text> void set_panel_config_button_order(Text *value) {
    set_panel_config_button_order(static_cast<esphome::text::Text *>(value));
    panel_config_order_template_ =
        std::is_same_v<Text, esphome::template_::TemplateText>;
  }
  template <typename Text> void set_panel_config_button_on_color(Text *value) {
    set_panel_config_button_on_color(static_cast<esphome::text::Text *>(value));
    panel_config_color_template_ =
        std::is_same_v<Text, esphome::template_::TemplateText>;
  }
  template <typename Text, typename... Subpages>
  void set_panel_config_button(uint8_t slot, Text *button,
                               Subpages... subpages) {
    static_assert(sizeof...(Subpages) >= 4 &&
                  sizeof...(Subpages) <=
                      configuration::PANEL_CONFIG_TEXT_CHUNK_COUNT);
    set_panel_config_button(slot, static_cast<esphome::text::Text *>(button),
                            static_cast<esphome::text::Text *>(subpages)...);
    if (slot == 0 || slot > panel_config_button_texts_.size())
      return;
    auto &sources = panel_config_button_texts_[slot - 1];
    sources.template_button =
        std::is_same_v<Text, esphome::template_::TemplateText>;
    sources.template_subpages = {
        std::is_same_v<std::remove_pointer_t<Subpages>,
                       esphome::template_::TemplateText>...};
  }
  size_t template_text_preference_count() const {
    size_t count =
        (panel_config_button_order_ && panel_config_order_template_) +
        (panel_config_button_on_color_ && panel_config_color_template_);
    for (const auto &sources : panel_config_button_texts_) {
      count += sources.button && sources.template_button;
      for (size_t i = 0; i < sources.subpages.size(); ++i)
        count += sources.subpages[i] && sources.template_subpages[i];
    }
    return count;
  }
  size_t empty_default_preference_count() const {
    size_t count = (panel_config_chrome_empty_defaults_ & 1u) != 0;
    count += (panel_config_chrome_empty_defaults_ & 2u) != 0;
    for (const auto &sources : panel_config_button_texts_)
      for (unsigned bit = 0; bit < 9; ++bit)
        count += (sources.empty_defaults & (1u << bit)) != 0;
    return count;
  }
  void set_panel_config_card_images_storage(bool enabled) {
    panel_config_card_images_storage_ = enabled;
  }
  void set_web_auth_credentials(const char *username, const char *password) {
    web_auth_username_ = username;
    web_auth_password_ = password;
  }

 private:
  class NativeConfigurationRuntime;

  configuration::ConfigurationRuntimeAdapter* selected_configuration_runtime();
  void register_panel_config_endpoints();
  void initialize_native_configuration();
  void defer_native_configuration_initialization();
  void apply_boot_configuration();
  void apply_configuration_text_refresh();
  bool native_configuration_requested() const;
  bool create_native_configuration_runtime();

  using PanelConfigTextSources =
      configuration::PanelConfigTextSources<esphome::text::Text>;

  const char *panel_config_device_profile_{nullptr};
  esphome::text::Text *panel_config_button_order_{nullptr};
  esphome::text::Text *panel_config_button_on_color_{nullptr};
  bool panel_config_order_template_{false};
  bool panel_config_color_template_{false};
  uint8_t panel_config_chrome_empty_defaults_{0};
  std::array<PanelConfigTextSources,
             configuration::PANEL_CONFIG_TEXT_SLOT_COUNT>
      panel_config_button_texts_{};
  bool panel_config_card_images_storage_{false};
  // This owns every non-trivial configuration object. Keeping it separate
  // leaves the ESPHome-created application object safe to construct before
  // the framework's allocators and component setup are ready.
  std::unique_ptr<NativeConfigurationRuntime> native_configuration_runtime_;
  EspControlAppCore core_{};
  HomeAssistantEndpointResolver home_assistant_endpoint_{};
  ApplicationSceneRuntime* application_scene_{nullptr};
  bool application_scene_required_{false};
  bool configuration_text_refresh_pending_{false};
  bool shutting_down_{false};
  bool native_configuration_initialized_{false};
  bool native_configuration_initialization_pending_{false};
  configuration::ConfigurationBufferRetry configuration_buffer_retry_{};
  bool panel_config_http_context_bound_{false};
  const char *web_auth_username_{nullptr};
  const char *web_auth_password_{nullptr};
};

}  // namespace espcontrol
