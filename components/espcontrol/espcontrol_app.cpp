#include "espcontrol_app.h"
#include "device_reset.h"
#include "home_assistant_transport_runtime.h"
#include "esphome/components/artwork_image/transfer_observer.h"

#include <algorithm>
#include <array>
#include <cinttypes>
#include <new>

#ifdef USE_ESP32
#include <esp_heap_caps.h>
#endif

#include "esphome/core/log.h"

#include "button_grid.h"
#include "configuration_main_loop.h"
#include "configuration_release_policy.h"
#include "configuration_service.h"
#include "configuration_store.h"
#include "panel_config_capabilities_endpoint.h"
#include "panel_config_endpoint_policy.h"
#include "panel_config_esphome_text.h"
#include "panel_config_espidf_storage.h"
#include "panel_config_http_context.h"
#include "panel_config_legacy_adapter.h"
#include "panel_config_read_endpoint.h"
#include "panel_config_runtime_adapter.h"
#include "panel_config_service_validator.h"
#include "panel_config_storage_selection.h"
#include "panel_config_transient_storage.h"
#include "panel_config_write_endpoint.h"
#include "panel_identity_endpoint.h"
#include "screensaver_pin_endpoint.h"

extern "C" void espcontrol_register_web_server_handlers(
    esphome::web_server_idf::AsyncWebServer *server) {
#ifdef USE_WEBSERVER
  if (server == nullptr) return;
  espcontrol::reset::register_handlers(*server);
  espcontrol::register_panel_identity_endpoint(*server);
  espcontrol::register_screensaver_pin_endpoint(*server);
  register_local_sensor_endpoint(*server);
  register_local_action_endpoint(*server);
  espcontrol::configuration::register_panel_config_capabilities_endpoint(*server);
  espcontrol::configuration::register_panel_config_read_endpoint(*server);
  espcontrol::configuration::register_panel_config_write_endpoint(*server);
#else
  (void) server;
#endif
}

namespace espcontrol {

static const char *const TAG = "espcontrol.config";
// This component is shared by devices whose OTA rollback window can be as
// short as 10 seconds. Leave enough of that window for initialization itself
// to fail safely after the display and restored text entities have settled.
constexpr uint32_t NATIVE_CONFIGURATION_INITIALIZATION_DELAY_MS = 5000;

static uint8_t *allocate_configuration_slot(size_t size) {
#ifdef USE_ESP32
  // Native v1/v2 conversion scratch requires eight-byte alignment. Ordinary
  // ESP32 PSRAM allocations only guarantee four bytes, so heap reuse can make
  // an otherwise valid configuration fail representation conversion.
  return static_cast<uint8_t *>(
      heap_caps_aligned_alloc(8, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
  (void)size;
  return nullptr;
#endif
}

static void release_configuration_slot(uint8_t *buffer) {
#ifdef USE_ESP32
  heap_caps_free(buffer);
#else
  (void)buffer;
#endif
}

class EspControlApp::NativeConfigurationRuntime {
 public:
   using LegacyButtonTextSources = configuration::PanelConfigTextValues<
       configuration::EspHomePanelConfigTextValue>;

   // The profile-sized owner is below the ordinary malloc internal-preference
   // threshold on several devices. Keep its metadata explicitly in PSRAM and
   // preserve the existing fallible initialization contract.
   static void *operator new(size_t size, const std::nothrow_t &) noexcept {
#ifdef USE_ESP32
     return allocate_configuration_slot(size);
#else
     return ::operator new(size, std::nothrow);
#endif
   }
   static void operator delete(void *storage) noexcept {
#ifdef USE_ESP32
     release_configuration_slot(static_cast<uint8_t *>(storage));
#else
     ::operator delete(storage);
#endif
   }
   static void operator delete(void *storage, const std::nothrow_t &) noexcept {
     operator delete(storage);
   }

   NativeConfigurationRuntime()
       : legacy_config(text_bindings, allocate_configuration_slot, release_configuration_slot),
         runtime_config(text_bindings, allocate_configuration_slot, release_configuration_slot),
         validator(allocate_configuration_slot, release_configuration_slot),
         backend(blobs, allocate_configuration_slot,
                 release_configuration_slot),
         store(backend) {}

   configuration::PanelConfigTextBindings text_bindings{};
   configuration::PanelConfigLegacyAdapter legacy_config;
   configuration::PanelConfigRuntimeAdapter runtime_config;
   configuration::PanelConfigDocumentValidator validator{};
   configuration::EspIdfPanelConfigBlobStorage blobs{};
   configuration::TransientBlobStorageBackend<
       PANEL_CONFIG_STORAGE_SLOT_CAPACITY>
       backend;
   configuration::ConfigurationStore store;
   configuration::ConfigurationMainLoop transactions;
   configuration::ConfigurationCompatibilityRetry compatibility_retry;
   bool document_available{false};
   bool service_configured{false};
   bool storage_ready{false};
   size_t slot_capacity{0};
   bool boot_configuration_pending{false};
   bool boot_configuration_due{false};
   configuration::EspHomePanelConfigTextValue button_order{};
   configuration::EspHomePanelConfigTextValue button_on_color{};
   std::array<LegacyButtonTextSources,
              configuration::PANEL_CONFIG_TEXT_SLOT_COUNT>
       buttons{};
};

EspControlApp::EspControlApp() = default;

EspControlApp::~EspControlApp() = default;

bool EspControlApp::install_application_scene(ApplicationSceneRuntime& scene) {
  if (shutting_down_ || !scene.ready()) return false;
  if (application_scene_) return application_scene_ == &scene;
  if (native_configuration_initialized_ || core_.configuration_service()) return false;
  application_scene_ = &scene;
  return true;
}

bool EspControlApp::require_application_scene() {
  if (shutting_down_ || native_configuration_initialized_ ||
      core_.configuration_service())
    return false;
  application_scene_required_ = true;
  return true;
}

configuration::ConfigurationRuntimeAdapter* EspControlApp::selected_configuration_runtime() {
  if (application_scene_)
    return application_scene_;
  if (application_scene_required_) {
    class Unavailable final
        : public configuration::ConfigurationRuntimeAdapter {
    public:
      bool prepare(uint16_t, const uint8_t *, size_t) override { return false; }
      bool apply(uint16_t, const uint8_t *, size_t) override { return false; }
    };
    static Unavailable unavailable;
    return &unavailable;
  }
  return native_configuration_runtime_ ? &native_configuration_runtime_->runtime_config : nullptr;
}

void EspControlApp::set_panel_config_device_profile(const char *device_profile) {
  panel_config_device_profile_ = device_profile;
}

void EspControlApp::set_panel_config_button_order(
    esphome::text::Text *button_order) {
  panel_config_button_order_ = button_order;
  panel_config_order_template_ = false;
  panel_config_chrome_empty_defaults_ &= ~uint8_t{1};
}

void EspControlApp::set_panel_config_button_on_color(
    esphome::text::Text *button_on_color) {
  panel_config_button_on_color_ = button_on_color;
  panel_config_color_template_ = false;
  panel_config_chrome_empty_defaults_ &= ~uint8_t{2};
}

void EspControlApp::set_panel_config_button(
    uint8_t slot, esphome::text::Text *button,
    esphome::text::Text *subpage_0, esphome::text::Text *subpage_1,
      esphome::text::Text *subpage_2, esphome::text::Text *subpage_3,
      esphome::text::Text *subpage_4, esphome::text::Text *subpage_5,
      esphome::text::Text *subpage_6, esphome::text::Text *subpage_7) {
  if (slot == 0 || slot > panel_config_button_texts_.size()) return;
  auto &sources = panel_config_button_texts_[slot - 1];
  sources = {};
  sources.button = button;
  const std::array<esphome::text::Text *, 8> chunks = {
      subpage_0, subpage_1, subpage_2, subpage_3,
      subpage_4, subpage_5, subpage_6, subpage_7};
  std::copy_n(chunks.begin(), sources.subpages.size(),
              sources.subpages.begin());
}

void EspControlApp::set_panel_config_text_empty_default(
    esphome::text::Text *text) {
  if (!text)
    return;
  if (text == panel_config_button_order_ && panel_config_order_template_)
    panel_config_chrome_empty_defaults_ |= 1u;
  if (text == panel_config_button_on_color_ && panel_config_color_template_)
    panel_config_chrome_empty_defaults_ |= 2u;
  for (auto &sources : panel_config_button_texts_) {
    if (text == sources.button && sources.template_button)
      sources.empty_defaults |= 1u;
    for (size_t i = 0; i < sources.subpages.size(); ++i)
      if (text == sources.subpages[i] && sources.template_subpages[i])
        sources.empty_defaults |= uint16_t{1} << (i + 1);
  }
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
  runtime->text_bindings.set_device_profile(panel_config_device_profile_);
  runtime->button_order.bind(panel_config_button_order_,
                             panel_config_order_template_,
                             (panel_config_chrome_empty_defaults_ & 1u) != 0);
  runtime->text_bindings.set_button_order(&runtime->button_order);
  runtime->button_on_color.bind(
      panel_config_button_on_color_, panel_config_color_template_,
      (panel_config_chrome_empty_defaults_ & 2u) != 0);
  runtime->text_bindings.set_button_on_color(&runtime->button_on_color);
  for (size_t index = 0; index < panel_config_button_texts_.size(); ++index) {
    const PanelConfigTextSources &sources = panel_config_button_texts_[index];
    // Device profiles only provide text entities for their real panel slots.
    // Do not register placeholder wrappers for the remaining fixed-capacity
    // entries: a restored document would correctly try to clear them, but the
    // wrappers have no ESPHome text object to update.
    if (sources.button == nullptr) continue;
    NativeConfigurationRuntime::LegacyButtonTextSources &legacy_sources =
        runtime->buttons[index];
    legacy_sources.button.bind(sources.button, sources.template_button,
                               (sources.empty_defaults & 1u) != 0);
    std::array<configuration::PanelConfigTextValue *,
               configuration::PanelConfigTextBindings::MAX_SUBPAGE_CHUNKS>
        legacy_subpages{};
    for (size_t subpage = 0; subpage < sources.subpages.size(); ++subpage) {
      if (sources.subpages[subpage] == nullptr) continue;
      legacy_sources.subpages[subpage].bind(
          sources.subpages[subpage], sources.template_subpages[subpage],
          (sources.empty_defaults & (1u << (subpage + 1))) != 0);
      legacy_subpages[subpage] = &legacy_sources.subpages[subpage];
    }
    if (!runtime->text_bindings.set_button(static_cast<uint8_t>(index + 1),
                                           &legacy_sources.button,
                                           legacy_subpages)) {
      ESP_LOGE(TAG, "Configured text sources exceed their compiled capacity");
      native_configuration_runtime_.reset();
      return false;
    }
  }
  return true;
}

void EspControlApp::register_panel_config_endpoints() {
  // Do not let an early reconnect cache a legacy-only capability response
  // while the deferred native configuration setup is still in progress.
  if (!native_configuration_initialized_ || panel_config_http_context_bound_)
    return;
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  NativeConfigurationRuntime *const runtime = native_configuration_runtime_.get();
  const bool can_bind_document_endpoints =
      panel_config_service != nullptr && runtime != nullptr &&
      runtime->document_available && runtime->slot_capacity > 0;
  if (!can_bind_document_endpoints) {
    ESP_LOGE(TAG,
             "Native configuration endpoints unavailable: service=%s "
             "runtime=%s document=%s",
             panel_config_service != nullptr ? "ready" : "missing",
             runtime != nullptr ? "ready" : "missing",
             runtime != nullptr && runtime->document_available ? "ready"
                                                               : "missing");
    configuration::set_panel_config_read_supported(false);
    configuration::set_panel_config_write_supported(false);
    panel_config_http_context_bound_ = true;
    configuration::set_panel_config_http_context_initialization_complete(true);
    return;
  }
  configuration::bind_panel_config_http_context(
      *panel_config_service, runtime->transactions, runtime->slot_capacity,
      allocate_configuration_slot, release_configuration_slot,
      web_auth_username_ == nullptr ? "" : web_auth_username_,
      web_auth_password_ == nullptr ? "" : web_auth_password_);
  configuration::set_panel_config_read_supported(true);
  configuration::set_panel_config_write_supported(true);
  // The context transitions from not-ready to ready once. Rebinding it from
  // loop() would briefly make concurrent requests observe a false readiness
  // flag and rewrite the shared pointers while the web task is using them.
  panel_config_http_context_bound_ = true;
  configuration::set_panel_config_http_context_initialization_complete(true);
}

void EspControlApp::apply_boot_configuration() {
  if (shutting_down_) return;
  NativeConfigurationRuntime *const runtime = native_configuration_runtime_.get();
  if (runtime == nullptr || !runtime->boot_configuration_pending ||
      runtime->slot_capacity == 0)
    return;

  // The timeout establishes the startup delay. A busy scene keeps this work
  // pending for a later safe application loop, without scheduling more timers.
  runtime->boot_configuration_due = true;
  if (application_scene_ && !application_scene_->maintenance_ready()) return;
  if (!configuration_buffer_retry_.ready(esphome::millis()))
    return;
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  if (panel_config_service == nullptr) return;
  configuration::ConfigurationBuffer buffer(allocate_configuration_slot,
                                            release_configuration_slot);
  auto *document = buffer.allocate(runtime->slot_capacity);
  if (!document) {
    configuration_buffer_retry_.failed(esphome::millis());
    return;
  }
  // Reload the newest durable generation now, never a setup-time snapshot.
  // This scoped output stays alive through preparation, activation and
  // publication.
  const configuration::ServiceLoadResult loaded =
      panel_config_service->load_and_apply_runtime(document,
                                                   runtime->slot_capacity);
  if (loaded.status == configuration::ServiceStatus::ALLOCATION_FAILED) {
    configuration_buffer_retry_.failed(esphome::millis());
    return;
  }
  runtime->boot_configuration_pending = false;
  runtime->boot_configuration_due = false;
  configuration_buffer_retry_.clear();
  if (loaded.status == configuration::ServiceStatus::LEGACY_MIRROR_FAILED) {
    ESP_LOGW(TAG, "Native configuration restored; compatibility text "
                  "publication is incomplete");
    return;
  }
  if (!loaded.ok()) {
    ESP_LOGE(TAG, "Native configuration could not reload for the live grid (%u)",
             static_cast<unsigned>(loaded.status));
    return;
  }
}

void EspControlApp::apply_configuration_text_refresh() {
  if (!configuration_text_refresh_pending_ || !application_scene_ ||
      shutting_down_)
    return;
  auto *runtime = native_configuration_runtime_.get();
  auto *service = core_.configuration_service();
  if (!runtime || !runtime->slot_capacity || !service ||
      runtime->boot_configuration_pending ||
      !application_scene_->maintenance_ready())
    return;
  if (!configuration_buffer_retry_.ready(esphome::millis()))
    return;
  configuration::ConfigurationBuffer buffer(allocate_configuration_slot,
                                            release_configuration_slot);
  auto *document = buffer.allocate(runtime->slot_capacity);
  if (!document) {
    configuration_buffer_retry_.failed(esphome::millis());
    return;
  }
  const auto synchronized =
      service->synchronize_legacy_runtime(document, runtime->slot_capacity);
  if (synchronized.status == configuration::ServiceStatus::ALLOCATION_FAILED) {
    configuration_buffer_retry_.failed(esphome::millis());
    return;
  }
  configuration_text_refresh_pending_ = false;
  configuration_buffer_retry_.clear();
  if (synchronized.status ==
      configuration::ServiceStatus::LEGACY_MIRROR_FAILED) {
    ESP_LOGW(
        TAG,
        "Configuration updated; compatibility synchronization is incomplete");
    return;
  }
  if (!synchronized.ok())
    ESP_LOGE(TAG, "Configuration text edits could not be applied (%u)",
             static_cast<unsigned>(synchronized.status));
}

void EspControlApp::retry_configuration_compatibility() {
  if (shutting_down_ || configuration_text_refresh_pending_ ||
      !reset::ready() || reset::pending()) return;
  auto *runtime = native_configuration_runtime_.get();
  auto *service = core_.configuration_service();
  if (!runtime || !service || !runtime->slot_capacity || runtime->boot_configuration_pending ||
      (application_scene_ && !application_scene_->maintenance_ready())) return;
  const auto state = service->compatibility_state();
  if (!state.retry_pending()) {
    runtime->compatibility_retry.clear();
    return;
  }
  if (!runtime->compatibility_retry.ready(esphome::millis(), state.generation)) return;
  reset::ConfigurationWriteLease lease;
  if (!lease.ready()) {
    runtime->compatibility_retry.failed(esphome::millis());
    return;
  }
  configuration::ConfigurationBuffer buffer(allocate_configuration_slot, release_configuration_slot);
  auto *document = buffer.allocate(runtime->slot_capacity);
  if (!document) {
    runtime->compatibility_retry.failed(esphome::millis());
    return;
  }
  const auto repaired = service->retry_compatibility(document, runtime->slot_capacity);
  if (service->compatibility_state().retry_pending()) {
    runtime->compatibility_retry.failed(esphome::millis());
    ESP_LOGW(TAG, "Compatibility copy remains incomplete; retry deferred (%u)",
             static_cast<unsigned>(repaired.status));
  } else {
    runtime->compatibility_retry.clear();
    if (!repaired.ok())
      ESP_LOGW(TAG, "Compatibility copy cannot represent this native configuration (%u)",
               static_cast<unsigned>(repaired.status));
  }
}

void EspControlApp::setup() {
  screensaver_pin().setup();
  home_assistant_endpoint_.setup();
  esphome::artwork_image::TransferObserver::instance().set(
      [this](const std::string &origin) -> uint32_t {
        return home_assistant_endpoint::parse_origin(origin) == home_assistant_endpoint_.origin()
            ? home_assistant_endpoint_.generation() : 0;
      },
      [this](const esphome::artwork_image::TransferNotice &notice) {
        using Failure = esphome::artwork_image::TransferFailure;
        home_assistant_endpoint_.report_download(
            home_assistant_endpoint::parse_origin(notice.request.origin),
            notice.request.endpoint_generation, notice.status,
            notice.failure == Failure::NONE, notice.failure == Failure::TRANSPORT);
      });
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

void EspControlApp::defer_native_configuration_initialization() {
  native_configuration_initialization_pending_ = true;
  configuration_buffer_retry_.failed(esphome::millis());
  ESP_LOGW(TAG, "Native configuration is waiting for temporary memory");
}

void EspControlApp::initialize_native_configuration() {
  if (shutting_down_ || native_configuration_initialized_)
    return;
  native_configuration_initialization_pending_ = false;
  ESP_LOGI(TAG, "Starting native configuration initialization");
  if (!native_configuration_requested()) {
    ESP_LOGD(TAG, "Native configuration is not requested for this device");
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  if (!create_native_configuration_runtime()) {
    defer_native_configuration_initialization();
    return;
  }
  NativeConfigurationRuntime &runtime = *native_configuration_runtime_;
  if (!runtime.service_configured) {
    if (!core_.configure_configuration_service(
            runtime.store, runtime.legacy_config, &runtime.validator,
            configuration::PANEL_CONFIG_LEGACY_MODE)) {
      ESP_LOGE(TAG, "Native configuration service is already configured");
      native_configuration_initialized_ = true;
      register_panel_config_endpoints();
      return;
    }
    runtime.service_configured = true;
  }
  configuration::ConfigurationService *const panel_config_service =
      core_.configuration_service();
  if (panel_config_service == nullptr) {
    ESP_LOGE(TAG, "Native configuration service is unavailable");
    native_configuration_initialized_ = true;
    register_panel_config_endpoints();
    return;
  }
  // The selected scene uses the same durable prepare/commit/activate protocol
  // as HTTP writes and boot restore. Legacy text remains the downgrade mirror.
  panel_config_service->set_runtime_adapter(selected_configuration_runtime());
  if (!runtime.legacy_config.configured()) {
    ESP_LOGW(TAG, "Native configuration sources are not configured");
  } else {
    if (!runtime.storage_ready) {
      const configuration::PanelConfigStorageSelection storage =
          configuration::begin_panel_config_storage(
              runtime.blobs, panel_config_card_images_storage_,
              PANEL_CONFIG_STORAGE_SLOT_CAPACITY,
              PANEL_CONFIG_NVS_SLOT_CAPACITY);
      if (!storage.ready || !runtime.backend.begin(storage.slot_capacity)) {
        ESP_LOGE(TAG, "Native configuration storage is unavailable");
        native_configuration_initialized_ = true;
        register_panel_config_endpoints();
        return;
      }
      runtime.slot_capacity = storage.slot_capacity;
      runtime.storage_ready = true;
      if (storage.used_nvs_fallback) {
        ESP_LOGW(TAG,
                 "Card-images partition is unavailable; using NVS with %u-byte "
                 "slots",
                 static_cast<unsigned>(runtime.slot_capacity));
      }
    }
    panel_config_service->set_buffer_allocator(allocate_configuration_slot,
                                               release_configuration_slot);
    configuration::ConfigurationBuffer buffer(allocate_configuration_slot,
                                              release_configuration_slot);
    auto *document = buffer.allocate(runtime.slot_capacity);
    if (!document) {
      defer_native_configuration_initialization();
      return;
    }
    ESP_LOGI(TAG, "Loading native configuration document");
    const configuration::ServiceLoadResult loaded =
        panel_config_service->load(document, runtime.slot_capacity);
    if (loaded.status == configuration::ServiceStatus::ALLOCATION_FAILED) {
      defer_native_configuration_initialization();
      return;
    }
    runtime.document_available =
        configuration::panel_config_load_allows_native_endpoints(loaded.status);
    if (!runtime.document_available) {
      ESP_LOGW(TAG,
               "Native configuration initial load failed (%u); keeping legacy configuration endpoints active",
               static_cast<unsigned>(loaded.status));
    }
    if (loaded.status == configuration::ServiceStatus::IMPORTED_LEGACY) {
      ESP_LOGI(TAG, "Imported legacy panel configuration into generation %" PRIu32,
               loaded.generation);
    } else if (!loaded.ok() && loaded.status != configuration::ServiceStatus::EMPTY) {
      ESP_LOGE(TAG, "Native configuration load failed (%u)",
               static_cast<unsigned>(loaded.status));
    }
    // Once a durable native document exists it is authoritative. Its legacy
    // mirror is persisted asynchronously by ESPHome and can still contain the
    // previous layout after a quick reboot. Re-importing that stale mirror here
    // would silently undo a successful card move, resize, or backup restore.
    // ConfigurationService::load() already imports legacy values when native
    // storage is empty, so no second boot-time shadow refresh is required.
    const configuration::ServiceLoadResult live_document = loaded;
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
  configuration_buffer_retry_.clear();
  register_panel_config_endpoints();
}

void EspControlApp::loop() {
  if (shutting_down_) return;
  if (native_configuration_initialization_pending_ &&
      !native_configuration_initialized_ &&
      configuration_buffer_retry_.ready(esphome::millis()))
    initialize_native_configuration();
  const bool scene_ready = !application_scene_ || application_scene_->maintenance_ready();
  if (scene_ready && native_configuration_runtime_ != nullptr && core_.configuration_service() != nullptr) {
    native_configuration_runtime_->transactions.process(*core_.configuration_service());
    if (native_configuration_runtime_->boot_configuration_due) apply_boot_configuration();
    apply_configuration_text_refresh();
    retry_configuration_compatibility();
  }
  if (shutting_down_) return;
  home_assistant_endpoint_.loop();
  core_.run_once();
  ha::maintain_subscriptions();
  // The app core starts before WiFi so Home Assistant boot automations are
  // safe. The IDF web server starts later, so retry idempotent registrations.
  if (shutting_down_) return;
  register_panel_config_endpoints();
  if (application_scene_ && application_scene_->maintenance_ready()) application_scene_->loop();
}

void EspControlApp::on_safe_shutdown() {
  // ESPHome runs all safe-shutdown hooks before any on_shutdown hook. mDNS
  // shuts down before this AFTER_WIFI component, freeing outstanding query
  // handles, so release our discovery query while the service is still alive.
  home_assistant_endpoint_.shutdown();
}

void EspControlApp::on_shutdown() {
  if (shutting_down_) return;
  shutting_down_ = true;
  if (native_configuration_runtime_ != nullptr)
    native_configuration_runtime_->transactions.stop();
  if (application_scene_) application_scene_->stop();
  esphome::artwork_image::TransferObserver::instance().set({}, {});
  cards::set_card_runtime_registry_service(nullptr);
  core_.stop();
}

}  // namespace espcontrol
