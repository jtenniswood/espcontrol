#pragma once
#include "reset_policy.h"
#include "esphome/core/component.h"
#include <atomic>

namespace esphome::update { class UpdateEntity; }
namespace esphome::web_server_idf { class AsyncWebServer; }

namespace espcontrol::reset {
// Called directly by generated setup before safe mode or restoring components.
void early_startup(bool compiled_networks, const char *username, const char *password);
// Apply before WiFi starts, after the generated component has been created.
void apply_wifi_override();
// A registered component keeps reset recovery independent of package on_boot
// merging and runs after generated WiFi configuration, before WiFi starts.
class ResetBoot : public esphome::Component {
 public:
  void setup() override;
  float get_setup_priority() const override { return esphome::setup_priority::HARDWARE + 1.0f; }
};
uint32_t epoch();
bool pending();
bool ready();
void register_handlers(esphome::web_server_idf::AsyncWebServer &server);
void watch_update(esphome::update::UpdateEntity *entity);
bool update_busy();
}  // namespace espcontrol::reset
