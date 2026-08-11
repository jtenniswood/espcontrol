#include "espcontrol_app.h"

#include <array>
#include <cinttypes>
#include <new>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

#include "esphome/core/log.h"

#include "panel_config_capabilities_endpoint.h"
#include "configuration_release_policy.h"
#include "configuration_service.h"
#include "configuration_store.h"
#include "panel_config_read_endpoint.h"
#include "panel_config_espidf_storage.h"
#include "panel_config_esphome_text.h"
#include "panel_config_legacy_adapter.h"
#include "panel_config_service_validator.h"
#include "panel_config_storage_backend.h"
#include "panel_config_write_endpoint.h"

namespace espcontrol {

static const char *const TAG = "espcontrol.config";
// This component is shared by devices whose OTA rollback window can be as
// short as 10 seconds. Leave enough of that window for initialization itself
// to fail safely after the display and restored text entities have settled.
constexpr uint32_t NATIVE_CONFIGURATION_INITIALIZATION_DELAY_MS = 5000;

class EspControlApp::NativeConfigurationRuntime {
 public:
  struct LegacyButtonTextSources {
    configuration::EspHomeLegacyTextValue button;
    std::array<configuration::EspHomeLegacyTextValue,
               configuration::PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
        subpages{};
  };

  NativeConfigurationRuntime()
      : backend(blobs), store(backend) {}

  configuration::PanelConfigLegacyAdapter legacy_config{};
  configuration::PanelConfigDocumentValidator validator{};
  configuration::EspIdfPanelConfigBlobStorage blobs{};
  configuration::BufferedBlobStorageBackend<PANEL_CONFIG_STORAGE_SLOT_CAPACITY>
      backend;
  configuration::ConfigurationStore store;
  uint8_t *memory{nullptr};
  uint8_t *document_buffer{nullptr};
  uint8_t *boot_buffer{nullptr};
  bool boot_configuration_pending{false};
  configuration::EspHomeLegacyTextValue button_order{};
  configuration::EspHomeLegacyTextValue button_on_color{};
  std::array<LegacyButtonTextSources, configuration::PANEL_CONFIG_MAX_SLOT_COUNT>
      buttons{};
};

EspControlApp::EspControlApp() = default;

EspControlApp::~EspControlApp() = default;

void EspControlApp::set_panel_config_device_profile(const char *device_profile) {
  panel_config_device_profile_ = device_profile;
}

void EspControlApp::set_panel_config_button_order(
    esphome::text::Text *button_order) {
  panel_config_button_order_ = button_order;
}

void EspControlApp::set_panel_config_button_on_color(
    esphome::text::Text *button_on_color) {
  panel_config_button_on_color_ = button_on_color;
}

void EspControlApp::set_panel_config_button(
    uint8_t slot, esphome::text::Text *button,
    esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
      esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
      esphome::text::Text *subpage_4, esphome::text::Text *subpage_5,
      esphome::text::Text *subpage_6, esphome::text::Text *subpage_7) {
  if (slot == 0 || slot > panel_config_button_texts_.size()) return;
  panel_config_button_texts_[slot - 1] = {
      button, {subpage_0, subpage_1, subpage_2, subpage_3, subpage_4,
               subpage_5, subpage_6, subpage_7}};
}

bool EspControlApp::native_configuration_requested() const {
  return panel_config_device_profile_ != nullptr &&
         panel_config_button_order_ != nullptr;
}

bool EspControlApp::create_native_configuration_runtime() {
  if (native_configuration_runtime_ != nullptr) return true;
  NativeConfigurationRuntime *runtime =
      new (std::nothrow) NativeConfigurationRuntime();
  if (runtime == nullptr) {
    ESP_LOGE(TAG, "Native configuration runtime memory is unavailable");
    return false;
  }
  native_configuration_runtime_.reset(runtime);
  runtime->legacy_config.set_device_profile(panel_config_device_profile_);
  runtime->button_order.bind(panel_config_button_order_);
  runtime->legacy_config.set_button_order(&runtime->button_order);
  runtime->button_on_color.bind(panel_config_button_on_color_);
  runtime->legacy_config.set_button_on_color(&runtime->button_on_color);
  for (size_t index = 0; index < panel_config_button_texts_.size(); ++index) {
    const PanelConfigTextSources &sources = panel_config_button_texts_[index];
    // Device profiles only provide text entities for their real panel slots.
    // Do not register placeholder wrappers for the remaining fixed-capacity
    // entries: a restored document would correctly try to clear them, but the
    // wrappers have no ESPHome text object to update.
    if (sources.button == nullptr) continue;
    NativeConfigurationRuntime::LegacyButtonTextSources &legacy_sources =
        runtime->buttons[index];
    legacy_sources.button.bind(sources.button);
    std::array<configuration::LegacyTextValue *,
               configuration::PanelConfigLegacyAdapter::MAX_SUBPAGE_CHUNKS>
        legacy_subpages{};
    for (size_t subpage = 0; subpage < sources.subpages.size(); ++subpage) {
      legacy_sources.subpages[subpage].bind(sources.subpages[subpage]);
      legacy_subpages[subpage] = &legacy_sources.subpages[subpage];
    }
    runtime->legacy_config.set_button(static_cast<uint8_t>(index + 1),
                                      &legacy_sources.button, legacy_subpages);
  }
  return true;
}

void EspControlApp::register_panel_config_endpoints() {
  // Do not let an early reconnect cache a legacy-only capability response
  // while the deferred native configuration setup is still in progress.
  if (!native_configuration_initialized_) return;
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  NativeConfigurationRuntime *const runtime = native_configuration_runtime_.get();
  const bool can_register_document_endpoints = panel_config_service != nullptr &&
      runtime != nullptr && runtime->document_buffer != nullptr;
  const bool read_endpoint_registered = can_register_document_endpoints &&
      configuration::register_panel_config_read_endpoint(
          *panel_config_service, runtime->document_buffer,
          PANEL_CONFIG_STORAGE_SLOT_CAPACITY,
          web_auth_username_ == nullptr ? "" : web_auth_username_,
          web_auth_password_ == nullptr ? "" : web_auth_password_);
  const bool write_endpoint_registered = can_register_document_endpoints &&
      configuration::register_panel_config_write_endpoint(
          *panel_config_service, runtime->document_buffer,
          PANEL_CONFIG_STORAGE_SLOT_CAPACITY,
          web_auth_username_ == nullptr ? "" : web_auth_username_,
          web_auth_password_ == nullptr ? "" : web_auth_password_);
  configuration::set_panel_config_read_supported(read_endpoint_registered);
  configuration::set_panel_config_write_supported(write_endpoint_registered);
  configuration::register_panel_config_capabilities_endpoint();
}

void EspControlApp::apply_boot_configuration() {
  NativeConfigurationRuntime *const runtime = native_configuration_runtime_.get();
  if (runtime == nullptr || !runtime->boot_configuration_pending ||
      runtime->boot_buffer == nullptr)
    return;

  runtime->boot_configuration_pending = false;
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
          runtime->boot_buffer, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
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

  // NVS work and the legacy snapshot can be expensive on a populated panel.
  // Give the display and restored text entities time to come up before
  // collecting the first legacy snapshot, while preserving the OTA rollback
  // window for every supported device.
  ESP_LOGI(TAG, "Deferring native configuration initialization for %" PRIu32 " ms",
           NATIVE_CONFIGURATION_INITIALIZATION_DELAY_MS);
  this->set_timeout(NATIVE_CONFIGURATION_INITIALIZATION_DELAY_MS,
                    [this]() { this->initialize_native_configuration(); });
}

void EspControlApp::initialize_native_configuration() {
  ESP_LOGI(TAG, "Starting native configuration initialization");
  if (!native_configuration_requested()) {
    ESP_LOGD(TAG, "Native configuration is not requested for this device");
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  if (!create_native_configuration_runtime()) {
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  NativeConfigurationRuntime &runtime = *native_configuration_runtime_;
  if (!core_.configure_configuration_service(
          runtime.store, runtime.legacy_config, &runtime.validator,
          configuration::PANEL_CONFIG_LEGACY_MODE)) {
    ESP_LOGE(TAG, "Native configuration service is already configured");
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  if (panel_config_service == nullptr) {
    ESP_LOGE(TAG, "Native configuration service is unavailable");
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  panel_config_service->set_runtime_adapter(&runtime.legacy_config);
  if (!runtime.legacy_config.configured()) {
    ESP_LOGW(TAG, "Native configuration sources are not configured");
  } else if (!runtime.blobs.begin()) {
    ESP_LOGE(TAG, "Native configuration storage is unavailable");
  } else {
    ESP_LOGI(TAG, "Allocating native configuration buffers");
#ifdef USE_ESP32
    // Two fixed slots back the atomic store; the scratch, HTTP request, and
    // delayed boot-application buffers must not overlap each other.
    constexpr size_t panel_config_memory_size =
        PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 5;
    runtime.memory = static_cast<uint8_t *>(
        heap_caps_malloc(panel_config_memory_size,
                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#endif
    if (runtime.memory == nullptr ||
        !runtime.backend.begin(runtime.memory,
                                     PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 2)) {
      ESP_LOGE(TAG, "Native configuration memory is unavailable");
      native_configuration_initialized_ = true;
      register_panel_config_endpoints();
      return;
    }
    ESP_LOGI(TAG, "Loading native configuration document");
    panel_config_service->set_scratch_buffer(
        runtime.memory + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 2,
        PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
    runtime.document_buffer =
        runtime.memory + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 3;
    runtime.boot_buffer = runtime.memory + PANEL_CONFIG_STORAGE_SLOT_CAPACITY * 4;
    const configuration::ServiceLoadResult loaded = panel_config_service->load(
        runtime.document_buffer, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
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
              runtime.document_buffer, PANEL_CONFIG_STORAGE_SLOT_CAPACITY);
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
      runtime.boot_configuration_pending = true;
      this->set_timeout(1000, [this]() { this->apply_boot_configuration(); });
    }
  }
  native_configuration_initialized_ = true;
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
