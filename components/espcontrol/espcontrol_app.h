#pragma once

#include "esphome/core/component.h"

#include "configuration_document_api.h"
#include "configuration_entity_document.h"
#include "configuration_service.h"
#include "configuration_store.h"
#include "espcontrol_app_core.h"
#include "esphome_configuration_registry.h"
#include "nvs_configuration_storage.h"

namespace espcontrol {

// The single ESPHome component boundary for EspControl-owned firmware state.
// YAML remains a compatibility/wiring layer and accesses services through this
// owner while behaviour moves into compiled modules.
class EspControlApp : public esphome::Component {
 public:
  void setup() override;
  void loop() override;
  void on_shutdown() override;

  DisplayModeController &display() { return core_.display(); }
  const DisplayModeController &display() const { return core_.display(); }
  AppLifecycleState lifecycle_state() const { return core_.lifecycle_state(); }
  configuration::ConfigurationDocumentApi &configuration_document() {
    return configuration_document_api_;
  }

 private:
  void bootstrap_configuration();

  EspControlAppCore core_{};
  configuration::EspHomeConfigurationRegistry configuration_registry_{};
  configuration::EntityConfigurationAdapter legacy_configuration_{
      configuration_registry_};
  configuration::NvsConfigurationStorage configuration_backend_{};
  configuration::ConfigurationStore configuration_store_{
      configuration_backend_};
  configuration::ConfigurationService configuration_service_{
      configuration_store_, legacy_configuration_};
  configuration::ConfigurationDocumentApi configuration_document_api_{
      configuration_service_};
  uint8_t *configuration_scratch_{nullptr};
  bool configuration_ready_{false};
  uint32_t configuration_retry_at_{0};
};

}  // namespace espcontrol
