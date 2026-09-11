#pragma once
#include "reset_policy.h"
#include <atomic>

namespace esphome::update { class UpdateEntity; }
namespace esphome::web_server_idf { class AsyncWebServer; }

namespace espcontrol::reset {
// Called directly by generated setup before safe mode or restoring components.
void early_startup(bool compiled_networks, const char *username, const char *password);
uint32_t epoch();
bool pending();
bool ready();
void register_handlers(esphome::web_server_idf::AsyncWebServer &server);
void watch_update(esphome::update::UpdateEntity *entity);
bool update_busy();
}  // namespace espcontrol::reset
