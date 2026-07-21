#pragma once

#include "esphome/core/component.h"

#include "espcontrol_app_core.h"
#include "card_asset_service.h"

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
  CardAssetService &card_assets() { return card_assets_; }
  const CardAssetService &card_assets() const { return card_assets_; }
  AppLifecycleState lifecycle_state() const { return core_.lifecycle_state(); }

 private:
  EspControlAppCore core_{};
  CardAssetService card_assets_{};
};

}  // namespace espcontrol
