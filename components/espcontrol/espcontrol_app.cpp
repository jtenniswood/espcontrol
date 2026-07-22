#include "espcontrol_app.h"

#include <cstdio>
#include <new>

#include "configuration_document_api.h"
#include "configuration_entity_document.h"
#include "configuration_http_handler.h"
#include "configuration_service.h"
#include "configuration_store.h"
#include "esp_heap_caps.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_WEBSERVER
#include "esphome/components/web_server_base/web_server_base.h"
#endif
#include "esphome_configuration_registry.h"
#include "nvs_configuration_storage.h"

namespace espcontrol {
namespace {

constexpr char TAG[] = "espcontrol.app";
constexpr uint32_t CONFIGURATION_RETRY_DELAY_MS = 5000;

}  // namespace

struct EspControlApp::ConfigurationRuntime {
  configuration::EspHomeConfigurationRegistry registry{};
  configuration::EntityConfigurationAdapter legacy{registry};
  configuration::NvsConfigurationStorage backend{};
  configuration::ConfigurationStore store{backend};
  configuration::ConfigurationService service{store, legacy};
  configuration::ConfigurationDocumentApi document_api{service};
  uint8_t *scratch{nullptr};
  uint8_t *upload{nullptr};
#ifdef USE_WEBSERVER
  configuration::ConfigurationHttpHandler *handler{nullptr};
#endif
  bool ready{false};
  bool transport_registered{false};
  uint32_t retry_at{0};
};

configuration::ConfigurationDocumentApi &
EspControlApp::configuration_document() {
  return configuration_->document_api;
}

void EspControlApp::setup() {
  core_.start();

  void *runtime_storage = heap_caps_malloc(
      sizeof(ConfigurationRuntime), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (runtime_storage == nullptr) {
    ESP_LOGE(TAG, "Unable to reserve configuration runtime memory");
    return;
  }
  configuration_ = new (runtime_storage) ConfigurationRuntime();
  if (!configuration_->backend.setup()) return;
  configuration_->scratch = static_cast<uint8_t *>(heap_caps_malloc(
      configuration::NvsConfigurationStorage::SLOT_CAPACITY,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  configuration_->upload = static_cast<uint8_t *>(heap_caps_malloc(
      configuration::NvsConfigurationStorage::SLOT_CAPACITY,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (configuration_->scratch == nullptr || configuration_->upload == nullptr) {
    ESP_LOGE(TAG, "Unable to reserve fixed configuration transaction memory");
    if (configuration_->scratch != nullptr) {
      heap_caps_free(configuration_->scratch);
      configuration_->scratch = nullptr;
    }
    if (configuration_->upload != nullptr) {
      heap_caps_free(configuration_->upload);
      configuration_->upload = nullptr;
    }
    return;
  }
  configuration_->service.use_scratch(
      configuration_->scratch,
      configuration::NvsConfigurationStorage::SLOT_CAPACITY);
}

void EspControlApp::bootstrap_configuration() {
  if (configuration_ == nullptr || configuration_->ready ||
      configuration_->scratch == nullptr ||
      !esphome::App.is_setup_complete()) {
    return;
  }
  const uint32_t now = esphome::millis();
  if (static_cast<int32_t>(now - configuration_->retry_at) < 0) return;

  const configuration::DocumentSnapshot snapshot =
      configuration_->document_api.snapshot(
          configuration_->scratch,
          configuration_->document_api.maximum_document_size());
  if (snapshot.status == configuration::DocumentApiStatus::EMPTY) {
    configuration_->ready = true;
    ESP_LOGI(TAG, "No configuration entities require a durable snapshot");
    return;
  }
  if (!snapshot.ok()) {
    ESP_LOGW(TAG, "Configuration snapshot unavailable; retrying later");
    configuration_->retry_at = now + CONFIGURATION_RETRY_DELAY_MS;
    return;
  }
  if (!configuration_->legacy.mirror(snapshot.document_version,
                                     configuration_->scratch,
                                     snapshot.document_size)) {
    ESP_LOGW(TAG, "Configuration activation deferred; retrying later");
    configuration_->retry_at = now + CONFIGURATION_RETRY_DELAY_MS;
    return;
  }

  configuration_->ready = true;
  ESP_LOGI(TAG, "Configuration revision %u is active (%u bytes)",
           static_cast<unsigned>(snapshot.revision),
           static_cast<unsigned>(snapshot.document_size));
}

void EspControlApp::register_configuration_transport() {
#ifdef USE_WEBSERVER
  if (configuration_ == nullptr || !configuration_->ready ||
      configuration_->transport_registered ||
      configuration_->upload == nullptr) {
    return;
  }
  auto *server_base = esphome::web_server_base::global_web_server_base;
  if (server_base == nullptr) return;
  configuration_->handler = new configuration::ConfigurationHttpHandler(
      configuration_->document_api, configuration_->legacy,
      configuration_->scratch,
      configuration::NvsConfigurationStorage::SLOT_CAPACITY,
      configuration_->upload,
      configuration::NvsConfigurationStorage::SLOT_CAPACITY);
  // Register through WebServerBase so optional Basic/Digest authentication is
  // applied to the configuration routes exactly like the built-in web API.
  server_base->add_handler(configuration_->handler);
  configuration_->transport_registered = true;
  ESP_LOGI(TAG, "Revisioned configuration transport is ready");
#endif
}

void EspControlApp::loop() {
  core_.run_once();
  bootstrap_configuration();
  register_configuration_transport();
#ifdef USE_WEBSERVER
  if (configuration_ != nullptr && configuration_->handler != nullptr) {
    const uint32_t revision =
        configuration_->handler->take_committed_revision();
    auto *events = esphome::web_server_idf::global_async_event_source();
    if (revision != 0 && events != nullptr) {
      char message[32];
      const int length = std::snprintf(
          message, sizeof(message), "{\"revision\":%u}",
          static_cast<unsigned>(revision));
      if (length > 0 && static_cast<size_t>(length) < sizeof(message)) {
        events->try_send_nodefer(message, static_cast<size_t>(length),
                                 "espcontrol_configuration");
      }
    }
  }
#endif
}

void EspControlApp::on_shutdown() {
  core_.stop();
  if (configuration_ != nullptr) configuration_->backend.close();
  // The web server retains the registered handler until reboot. Keep its
  // fixed PSRAM storage alive during shutdown so late socket callbacks cannot
  // observe freed memory.
}

}  // namespace espcontrol
