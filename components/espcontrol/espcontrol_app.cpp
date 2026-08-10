#include "espcontrol_app.h"

#include <cinttypes>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

#include "esphome/core/log.h"

#include "panel_config_capabilities_endpoint.h"
#include "configuration_release_policy.h"
#include "panel_config_read_endpoint.h"
#include "panel_config_write_endpoint.h"

namespace espcontrol {

static const char *const TAG = "espcontrol.config";

void EspControlApp::set_panel_config_device_profile(const char *device_profile) {
  legacy_config_.set_device_profile(device_profile);
}

void EspControlApp::set_panel_config_button_order(
    esphome::text::Text *button_order) {
  button_order_text_.bind(button_order);
  legacy_config_.set_button_order(&button_order_text_);
}

void EspControlApp::set_panel_config_button_on_color(
    esphome::text::Text *button_on_color) {
  button_on_color_text_.bind(button_on_color);
  legacy_config_.set_button_on_color(&button_on_color_text_);
}

void EspControlApp::set_panel_config_button(
    uint8_t slot, esphome::text::Text *button,
    esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
    esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
    esphome::text::Text *subpage_4, esphome::text::Text *subpage_5,
    esphome::text::Text *subpage_6, esphome::text::Text *subpage_7) {
  if (slot == 0 || slot > legacy_button_texts_.size()) return;
  LegacyButtonTextSources &sources = legacy_button_texts_[slot - 1];
  sources.button.bind(button);
  const std::array<esphome::text::Text *,
                   configuration::PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
      subpages{{subpage_0, subpage_1, subpage_2, subpage_3, subpage_4,
                subpage_5, subpage_6, subpage_7}};
  std::array<configuration::LegacyTextValue *,
             configuration::PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
      legacy_subpages{};
  for (size_t index = 0; index < subpages.size(); ++index) {
    sources.subpages[index].bind(subpages[index]);
    legacy_subpages[index] = &sources.subpages[index];
  }
  legacy_config_.set_button(slot, &sources.button, legacy_subpages);
}

void EspControlApp::register_panel_config_endpoints() {
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  const bool can_register_document_endpoints =
      panel_config_service != nullptr && panel_config_document_buffer_ != nullptr;
  const bool read_endpoint_registered = can_register_document_endpoints &&
      configuration::register_panel_config_read_endpoint(
          *panel_config_service, panel_config_document_buffer_,
          PANEL_CONFIG_STORAGE_SLOT_CAPACITY, web_auth_username_.c_str(),
          web_auth_password_.c_str());
  const bool write_endpoint_registered = can_register_document_endpoints &&
      configuration::register_panel_config_write_endpoint(
          *panel_config_service, panel_config_document_buffer_,
          PANEL_CONFIG_STORAGE_SLOT_CAPACITY, web_auth_username_.c_str(),
          web_auth_password_.c_str());
  configuration::set_panel_config_read_supported(read_endpoint_registered);
  configuration::set_panel_config_write_supported(write_endpoint_registered);
  configuration::register_panel_config_capabilities_endpoint();
}

void EspControlApp::apply_boot_configuration() {
  if (!boot_configuration_pending_ || boot_configuration_buffer_ == nullptr)
    return;

  boot_configuration_pending_ = false;
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  if (panel_config_service == nullptr) return;
  // Do not retain the document captured during setup: a browser save can
  // complete before this timeout runs, and the newest durable document must
  // always win over startup restoration. The boot buffer is intentionally
  // separate from the HTTP request buffer. ConfigurationService serializes
  // this reload and live apply with HTTP saves so neither its scratch buffer
  // nor the running grid can be reverted by an older startup document.
  const configuration::ServiceLoadResult loaded =
      panel_config_service->load_and_apply_runtime(
          boot_configuration_buffer_, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
  if (!loaded.ok()) {
    ESP_LOGE(TAG, "Native configuration could not reload for the live grid (%u)",
             static_cast<unsigned>(loaded.status));
    return;
  }
}

void EspControlApp::setup() {
  if (core_.start()) {
    cards::set_card_runtime_registry_service(&core_.card_runtime_registry());
  } else {
    ESP_LOGE(TAG, "Application core failed to start");
  }
  if (!core_.configure_configuration_service(
          panel_config_store_, legacy_config_, &panel_config_validator_,
          configuration::PANEL_CONFIG_LEGACY_MODE)) {
    ESP_LOGE(TAG, "Native configuration service is already configured");
    register_panel_config_endpoints();
    return;
  }
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  if (panel_config_service == nullptr) {
    ESP_LOGE(TAG, "Native configuration service is unavailable");
    register_panel_config_endpoints();
    return;
  }
  panel_config_service->set_runtime_adapter(&legacy_config_);
  if (!legacy_config_.configured()) {
    ESP_LOGW(TAG, "Native configuration sources are not configured");
  } else if (!panel_config_blobs_.begin()) {
    ESP_LOGE(TAG, "Native configuration storage is unavailable");
  } else {
#ifdef USE_ESP32
    // Two fixed slots back the atomic store; the scratch, HTTP request, and
    // delayed boot-application buffers must not overlap each other.
    constexpr size_t panel_config_memory_size =
        PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 5;
    panel_config_memory_ = static_cast<uint8_t *>(
        heap_caps_malloc(panel_config_memory_size,
                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#endif
    if (panel_config_memory_ == nullptr ||
        !panel_config_backend_.begin(panel_config_memory_,
                                     PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 2)) {
      ESP_LOGE(TAG, "Native configuration memory is unavailable");
      register_panel_config_endpoints();
      return;
    }
    panel_config_service->set_scratch_buffer(
        panel_config_memory_ + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 2,
        PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
    panel_config_document_buffer_ =
        panel_config_memory_ + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 3;
    boot_configuration_buffer_ =
        panel_config_memory_ + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 4;
    const configuration::ServiceLoadResult loaded = panel_config_service->load(
        panel_config_document_buffer_, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
    if (loaded.status == configuration::ServiceStatus::IMPORTED_LEGACY) {
      ESP_LOGI(TAG, "Imported legacy panel configuration into generation %" PRIu32,
               loaded.generation);
    } else if (!loaded.ok() && loaded.status != configuration::ServiceStatus::EMPTY) {
      ESP_LOGE(TAG, "Native configuration load failed (%u)",
               static_cast<unsigned>(loaded.status));
    }
    configuration::ServiceLoadResult live_document = loaded;
    if (panel_config_service->legacy_writes_enabled()) {
      const configuration::ServiceLoadResult refreshed =
          panel_config_service->refresh_legacy_shadow(
              panel_config_document_buffer_, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
      if (refreshed.status == configuration::ServiceStatus::SYNCED_LEGACY) {
        ESP_LOGI(TAG, "Refreshed native configuration shadow to generation %" PRIu32,
                 refreshed.generation);
      } else if (!refreshed.ok() &&
                 refreshed.status != configuration::ServiceStatus::EMPTY) {
        ESP_LOGE(TAG, "Native configuration refresh failed (%u)",
                 static_cast<unsigned>(refreshed.status));
      }
      if (refreshed.ok()) live_document = refreshed;
    }
    if (live_document.ok()) {
      // Publishing the restored values triggers the existing grid-refresh
      // automations. Run that only after every ESPHome component has completed
      // setup: on P4 panels the grid and LVGL objects are not safe to refresh
      // while this component's WiFi-priority setup callback is still running.
      // Browser PUT requests still apply immediately through the runtime
      // adapter; this deferral is strictly for startup restoration.
      boot_configuration_pending_ = true;
      this->set_timeout(1000, [this]() { this->apply_boot_configuration(); });
    }
  }
  register_panel_config_endpoints();
}

void EspControlApp::loop() {
  core_.run_once();
  // The app core starts before WiFi so Home Assistant boot automations are
  // safe. The IDF web server starts later, so retry idempotent registrations.
  register_panel_config_endpoints();
}

void EspControlApp::on_shutdown() {
  cards::set_card_runtime_registry_service(nullptr);
  core_.stop();
}

}  // namespace espcontrol
