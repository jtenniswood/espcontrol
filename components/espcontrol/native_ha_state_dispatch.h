#pragma once

#include "esphome/core/defines.h"
#ifdef USE_API_HOMEASSISTANT_STATES

// Main-loop native API lookup. No SDK class layout or protected parent access.
#include "prepared_native_ha_subscription_index.h"
#include "device_ui_limits.h"
#include "esphome/components/api/api_server.h"
#include "esphome/core/helpers.h"

namespace espcontrol::ha::native_lookup {
using Row = NativeHaSubscriptionIndex<>::Row;
using Allocator = esphome::RAMAllocator<Row>;
using Owner = PreparedNativeHaSubscriptionIndex<Allocator>;
inline uint64_t fast_frames = 0, stock_frames = 0, interrupted_frames = 0;
inline Owner &owner() {
#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
  constexpr size_t maximum_rows = DEVICE_UI_LIMITS.ha_state_bytes / sizeof(Row);
#else
  constexpr size_t maximum_rows = 0;  // Unprofiled integrations keep stock dispatch.
#endif
  static Owner value(Allocator(Allocator::ALLOC_EXTERNAL | Allocator::ALLOC_INTERNAL |
                              Allocator::ALLOW_FAILURE),
                     64, maximum_rows);
  return value;
}
inline void maintain(uint32_t now_ms) {
  auto *server = esphome::api::global_api_server;
  if (server) owner().maintain(server, server->get_state_subs(), now_ms);
  else owner().reset();
}
template<typename Message>
bool dispatch(esphome::api::APIConnection *connection, const Message &message) {
  auto *server = esphome::api::global_api_server;
  if (!server) return false;
  bool member = false;
  for (const auto &client : server->active_clients())
    if (client && client.get() == connection) { member = true; break; }
  if (!member || !owner().current(server, server->get_state_subs())) return false;
  // Match the original handler's ordering: empty entity does not touch state.
  if (message.entity_id.size() == 0) { ++fast_frames; return true; }
  // Decode/authentication still run in the unchanged generated dispatcher.
  // The SDK reserves the trailing byte and null-terminates the same view here.
  if (message.state.size() != 0)
    const_cast<char *>(message.state.c_str())[message.state.size()] = '\0';
  const auto result = owner().dispatch(server, server->get_state_subs(),
      message.entity_id, message.attribute, message.state);
  if (result == Owner::Result::FALLBACK) return false;
  ++fast_frames;
  if (result == Owner::Result::INTERRUPTED) ++interrupted_frames;
  return true;
}
}  // namespace espcontrol::ha::native_lookup
#endif
