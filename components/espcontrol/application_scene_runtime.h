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
// Keep the live compatibility entities aligned even when downgrade persistence
// is unavailable (for example, a native value exceeds an old text chunk).
// Publication happens only after the prepared scene has become authoritative.
class ApplicationConfigurationRuntime final
    : public configuration::ConfigurationRuntimeAdapter {
public:
  explicit ApplicationConfigurationRuntime(
      configuration::ConfigurationRuntimeAdapter &publication)
      : publication_(publication) {}
  void bind(ApplicationSceneRuntime &scene) { scene_ = &scene; }
  bool prepare(uint16_t version, const uint8_t *document,
               size_t size) override {
    return scene_ && scene_->prepare(version, document, size);
  }
  void discard_prepared() override {
    if (scene_)
      scene_->discard_prepared();
  }
  bool activate_prepared(uint32_t generation, uint16_t version,
                         const uint8_t *document, size_t size) override {
    return scene_ &&
           scene_->activate_prepared(generation, version, document, size) &&
           publication_.apply(version, document, size);
  }
  bool apply(uint16_t version, const uint8_t *document, size_t size) override {
    return scene_ && scene_->apply(version, document, size) &&
           publication_.apply(version, document, size);
  }

private:
  ApplicationSceneRuntime *scene_{nullptr};
  configuration::ConfigurationRuntimeAdapter &publication_;
};
}  // namespace espcontrol
