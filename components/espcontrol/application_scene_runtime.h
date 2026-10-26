#pragma once

#include "configuration_service.h"

namespace espcontrol {
// One application-lifetime scene, installed after its widget bank and native
// ports are initialized, before native configuration initialization. It owns
// the prepared runtime adapter and all UI/resource lifetimes behind it.
class ApplicationSceneRuntime : public configuration::ConfigurationRuntimeAdapter {
 public:
  // Structural readiness, independent of connectivity and visible display mode.
  virtual bool ready() const = 0;
  // Read-only: rejects LVGL/native/HA dispatch and recursive scene maintenance.
  virtual bool maintenance_ready() const = 0;
  virtual void loop() = 0;
  // Revoke input, commands and future work. Safe during shutdown callbacks;
  // defer any widget/context collection until the dispatch boundary is clear.
  virtual void stop() = 0;
};
}  // namespace espcontrol
