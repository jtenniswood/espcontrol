#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ha_read_coordinator.h"

#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#endif

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant API boundary ──────────────────────────────────────

using HomeAssistantStateCallback = std::function<void(esphome::StringRef)>;
using HomeAssistantActionResponseCallback =
  std::function<void(const esphome::api::ActionResponse &)>;

#ifndef ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_ALL = 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_DEFAULT = 1u << 0;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_COVER_ART = 1u << 1;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_PHASE3 = 1u << 2;
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS = 1u << 3;
#define ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED 1
#endif

inline bool ha_api_available() {
  return esphome::api::global_api_server != nullptr;
}

inline bool ha_api_connected() {
  return ha_api_available() && esphome::api::global_api_server->is_connected();
}

inline bool ha_api_state_connected() {
  return ha_api_available() && esphome::api::global_api_server->is_connected_with_state_subscription();
}

constexpr size_t HA_READ_INTERNAL_FREE_MIN_BYTES = 8 * 1024;
constexpr size_t HA_READ_INTERNAL_LARGEST_MIN_BYTES = 4 * 1024;
constexpr size_t HA_ACTION_INTERNAL_FREE_MIN_BYTES = 8 * 1024;
constexpr size_t HA_ACTION_INTERNAL_LARGEST_MIN_BYTES = 4 * 1024;

inline bool ha_internal_heap_available(const char *stage,
                                       size_t min_free = HA_ACTION_INTERNAL_FREE_MIN_BYTES,
                                       size_t min_largest = HA_ACTION_INTERNAL_LARGEST_MIN_BYTES) {
#ifdef ESP_PLATFORM
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  size_t internal_largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
  if (internal_free < min_free || internal_largest < min_largest) {
    ESP_LOGW("ha", "Deferring %s: internal heap free=%u largest=%u",
             stage ? stage : "Home Assistant request",
             (unsigned) internal_free, (unsigned) internal_largest);
    return false;
  }
#else
  (void) stage;
  (void) min_free;
  (void) min_largest;
#endif
  return true;
}

// Last-known Home Assistant state per subscribed entity/attribute. Home
// Assistant only sends current values for subscriptions it learns about at
// connect time (APIConnection announces state_subs_ once per connection), so a
// subscription created later — e.g. the live "Apply Configuration" grid
// rebuild — would only ever see future changes, leaving rebuilt tiles on their
// default ("off") state. Every incoming push is cached here and replayed to
// new subscriptions so rebuilt tiles reflect the current state immediately.
struct HaStateCacheEntry {
  std::string entity_id;
  std::string attribute;
  std::string value;
};

inline std::vector<HaStateCacheEntry> &ha_state_cache() {
  static std::vector<HaStateCacheEntry> cache;
  return cache;
}

inline std::string *ha_state_cache_find(const std::string &entity_id,
                                        const std::string &attribute) {
  for (auto &entry : ha_state_cache()) {
    if (entry.entity_id == entity_id && entry.attribute == attribute) return &entry.value;
  }
  return nullptr;
}

inline void ha_state_cache_store(const std::string &entity_id,
                                 const std::string &attribute,
                                 esphome::StringRef state) {
  if (std::string *value = ha_state_cache_find(entity_id, attribute)) {
    value->assign(state.c_str(), state.size());
    return;
  }
  ha_state_cache().push_back({entity_id, attribute, std::string(state.c_str(), state.size())});
}

struct EspHomeHaReadTransport {
  using State = esphome::StringRef;
  using Callback = HomeAssistantStateCallback;

  bool available() const { return ha_api_available(); }
  bool state_connected() const { return ha_api_state_connected(); }

  void get(std::string entity_id, std::string attribute, Callback callback) {
    esphome::api::global_api_server->get_home_assistant_state(
        std::move(entity_id), std::move(attribute), std::move(callback));
  }

  void subscribe(const std::string &entity_id,
                 const std::string &attribute,
                 Callback callback) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
        entity_id, attribute,
        [entity_id, attribute, callback](esphome::StringRef state) {
          ha_state_cache_store(entity_id, attribute, state);
          callback(state);
        });
  }

  void replay(const std::string &entity_id,
              const std::string &attribute,
              Callback callback) {
    // Replay the last-known value so runtime re-subscribes start correct; the
    // cache is empty at boot, so boot subscriptions are unaffected.
    if (const std::string *cached = ha_state_cache_find(entity_id, attribute)) {
      callback(esphome::StringRef(*cached));
    }
  }
};

struct EspHomeHaHeapProbe {
  bool available(const char *stage, size_t min_free, size_t min_largest) const {
    return ha_internal_heap_available(stage, min_free, min_largest);
  }
};

using EspHomeHaReadCoordinator = HaReadCoordinator<EspHomeHaReadTransport, EspHomeHaHeapProbe>;

inline EspHomeHaReadCoordinator &ha_read_coordinator() {
  static EspHomeHaReadCoordinator coordinator;
  return coordinator;
}

inline uint32_t &ha_subscription_generation() {
  return ha_read_coordinator().generation_ref();
}

inline void ha_reset_subscription_callbacks(uint32_t scope = HA_SUBSCRIPTION_SCOPE_ALL) {
  ha_read_coordinator().reset_subscriptions(scope);
}
#define ESPCONTROL_HA_SUBSCRIPTION_HELPERS_DEFINED 1

inline void ha_reset_deferred_state_requests() {
  ha_read_coordinator().reset_deferred();
}
#define ESPCONTROL_HA_DEFERRED_HELPERS_DEFINED 1

inline void bump_ha_subscription_generation() {
  ha_read_coordinator().bump_generation(
      HA_SUBSCRIPTION_SCOPE_DEFAULT | HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS);
}
#define ESPCONTROL_HA_GENERATION_HELPERS_DEFINED 1

inline void ha_flush_deferred_state_requests(size_t max_requests = 8) {
  ha_read_coordinator().flush(
      max_requests, HA_READ_INTERNAL_FREE_MIN_BYTES, HA_READ_INTERNAL_LARGEST_MIN_BYTES);
}

inline bool ha_action_begin(esphome::api::HomeassistantActionRequest &req,
                            const char *service,
                            bool is_event,
                            size_t data_count,
                            uint32_t call_id = 0) {
  if (!ha_api_available() || service == nullptr || service[0] == '\0') return false;
  req.service = decltype(req.service)(service);
  req.is_event = is_event;
  if (call_id != 0) req.call_id = call_id;
  req.data.init(data_count);
  return true;
}

inline void ha_action_add_data(esphome::api::HomeassistantActionRequest &req,
                               const char *key,
                               const char *value) {
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)(key ? key : "");
  kv.value = decltype(kv.value)(value ? value : "");
}

inline void ha_action_add_data_template(esphome::api::HomeassistantActionRequest &req,
                                        const char *key,
                                        const char *value) {
  auto &kv = req.data_template.emplace_back();
  kv.key = decltype(kv.key)(key ? key : "");
  kv.value = decltype(kv.value)(value ? value : "");
}

inline void ha_action_add_variable(esphome::api::HomeassistantActionRequest &req,
                                   const char *key,
                                   const char *value) {
  auto &kv = req.variables.emplace_back();
  kv.key = decltype(kv.key)(key ? key : "");
  kv.value = decltype(kv.value)(value ? value : "");
}

inline void ha_action_add_entity(esphome::api::HomeassistantActionRequest &req,
                                 const std::string &entity_id) {
  ha_action_add_data(req, "entity_id", entity_id.c_str());
}

inline bool ha_action_send(esphome::api::HomeassistantActionRequest &req) {
  if (!ha_api_state_connected()) return false;
  if (!ha_internal_heap_available("Home Assistant action",
                                  HA_ACTION_INTERNAL_FREE_MIN_BYTES,
                                  HA_ACTION_INTERNAL_LARGEST_MIN_BYTES)) return false;
  esphome::api::global_api_server->send_homeassistant_action(req);
  return true;
}

inline bool ha_send_entity_action(const std::string &entity_id,
                                  const char *service) {
  if (entity_id.empty()) return false;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, 1)) return false;
  ha_action_add_entity(req, entity_id);
  return ha_action_send(req);
}

inline bool ha_send_entity_action(const std::string &entity_id,
                                  const char *service,
                                  const char *data_key,
                                  const char *data_value) {
  if (entity_id.empty()) return false;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, data_key && data_value ? 2 : 1)) return false;
  ha_action_add_entity(req, entity_id);
  if (data_key && data_value) ha_action_add_data(req, data_key, data_value);
  return ha_action_send(req);
}

inline bool ha_register_action_response_callback(uint32_t call_id,
                                                 HomeAssistantActionResponseCallback callback) {
  if (!ha_api_available() || call_id == 0) return false;
  esphome::api::global_api_server->register_action_response_callback(call_id, callback);
  return true;
}

inline bool ha_cancel_action_response_callback(uint32_t call_id, const char *error_message = "cancelled") {
  if (!ha_api_available() || call_id == 0) return false;
  esphome::api::global_api_server->handle_action_response(
    call_id, false, esphome::StringRef(error_message ? error_message : "cancelled"));
  return true;
}

inline bool ha_subscribe_state(const std::string &entity_id,
                               HomeAssistantStateCallback callback,
                               uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT) {
  return ha_read_coordinator().subscribe(entity_id, std::string(), std::move(callback), scope);
}

inline bool ha_get_state(const std::string &entity_id,
                         HomeAssistantStateCallback callback) {
  return ha_read_coordinator().get(
      entity_id, std::string(), std::move(callback), false,
      HA_READ_INTERNAL_FREE_MIN_BYTES, HA_READ_INTERNAL_LARGEST_MIN_BYTES);
}

inline bool ha_subscribe_attribute(const std::string &entity_id,
                                   const std::string &attribute,
                                   HomeAssistantStateCallback callback,
                                   uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT) {
  return ha_read_coordinator().subscribe(entity_id, attribute, std::move(callback), scope);
}

inline bool ha_get_attribute(const std::string &entity_id,
                             const std::string &attribute,
                             HomeAssistantStateCallback callback) {
  return ha_read_coordinator().get(
      entity_id, attribute, std::move(callback), true,
      HA_READ_INTERNAL_FREE_MIN_BYTES, HA_READ_INTERNAL_LARGEST_MIN_BYTES);
}
