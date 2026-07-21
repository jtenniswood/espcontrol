#include "espcontrol_app.h"

#include "esp_heap_caps.h"
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace espcontrol {
namespace {

constexpr char TAG[] = "espcontrol.app";
constexpr uint32_t CONFIGURATION_RETRY_DELAY_MS = 5000;

}  // namespace

void EspControlApp::setup() {
  core_.start();

  if (!configuration_backend_.setup()) return;
  configuration_scratch_ = static_cast<uint8_t *>(heap_caps_malloc(
      configuration::NvsConfigurationStorage::SLOT_CAPACITY,
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (configuration_scratch_ == nullptr) {
    ESP_LOGE(TAG, "Unable to reserve fixed configuration transaction memory");
    return;
  }
  configuration_service_.use_scratch(
      configuration_scratch_,
      configuration::NvsConfigurationStorage::SLOT_CAPACITY);
}

void EspControlApp::bootstrap_configuration() {
  if (configuration_ready_ || configuration_scratch_ == nullptr ||
      !esphome::App.is_setup_complete()) {
    return;
  }
  const uint32_t now = esphome::millis();
  if (static_cast<int32_t>(now - configuration_retry_at_) < 0) return;

  const configuration::DocumentSnapshot snapshot =
      configuration_document_api_.snapshot(
          configuration_scratch_,
          configuration_document_api_.maximum_document_size());
  if (snapshot.status == configuration::DocumentApiStatus::EMPTY) {
    configuration_ready_ = true;
    ESP_LOGI(TAG, "No configuration entities require a durable snapshot");
    return;
  }
  if (!snapshot.ok()) {
    ESP_LOGW(TAG, "Configuration snapshot unavailable; retrying later");
    configuration_retry_at_ = now + CONFIGURATION_RETRY_DELAY_MS;
    return;
  }
  if (!legacy_configuration_.mirror(snapshot.document_version,
                                    configuration_scratch_,
                                    snapshot.document_size)) {
    ESP_LOGW(TAG, "Configuration activation deferred; retrying later");
    configuration_retry_at_ = now + CONFIGURATION_RETRY_DELAY_MS;
    return;
  }

  configuration_ready_ = true;
  ESP_LOGI(TAG, "Configuration revision %u is active (%u bytes)",
           static_cast<unsigned>(snapshot.revision),
           static_cast<unsigned>(snapshot.document_size));
}

void EspControlApp::loop() {
  core_.run_once();
  bootstrap_configuration();
}

void EspControlApp::on_shutdown() {
  core_.stop();
  configuration_backend_.close();
  if (configuration_scratch_ != nullptr) {
    heap_caps_free(configuration_scratch_);
    configuration_scratch_ = nullptr;
  }
}

}  // namespace espcontrol
