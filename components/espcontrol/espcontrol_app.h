#pragma once

#include "esphome/core/component.h"

#include "espcontrol_app_core.h"

namespace espcontrol::configuration {
class ConfigurationDocumentApi;
}

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
  configuration::ConfigurationDocumentApi &configuration_document();

 private:
  struct ConfigurationRuntime;

  void bootstrap_configuration();
  void register_configuration_transport();

  EspControlAppCore core_{};
  ConfigurationRuntime *configuration_{nullptr};
};

}  // namespace espcontrol
