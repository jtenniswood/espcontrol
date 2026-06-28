#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

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
#define ESPCONTROL_HA_SUBSCRIPTION_SCOPE_CONSTANTS_DEFINED 1
#endif

inline uint32_t &ha_subscription_generation();

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
constexpr size_t HA_ACTION_INTERNAL_FREE_MIN_BYTES = 12 * 1024;
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

struct HaDeferredStateRequest {
  std::string entity_id;
  std::string attribute;
  std::vector<std::shared_ptr<HomeAssistantStateCallback>> callbacks;
  uint32_t generation = 0;
  bool has_attribute = false;
};

inline std::vector<HaDeferredStateRequest> &ha_deferred_state_requests() {
  static std::vector<HaDeferredStateRequest> requests;
  return requests;
}

inline void ha_release_deferred_state_request_storage() {
  std::vector<HaDeferredStateRequest> &requests = ha_deferred_state_requests();
  if (!requests.empty() || requests.capacity() == 0) return;
  std::vector<HaDeferredStateRequest>().swap(requests);
}

inline uint8_t &ha_state_callback_depth() {
  static uint8_t depth = 0;
  return depth;
}

struct HaSubscriptionCallbackRef {
  std::shared_ptr<HomeAssistantStateCallback> callback;
  uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT;
};

inline std::vector<HaSubscriptionCallbackRef> &ha_subscription_callback_refs() {
  static std::vector<HaSubscriptionCallbackRef> refs;
  return refs;
}

inline uint32_t &ha_subscription_callback_reset_pending_mask() {
  static uint32_t pending_mask = 0;
  return pending_mask;
}

inline void ha_release_subscription_callbacks_now(uint32_t scope = HA_SUBSCRIPTION_SCOPE_ALL) {
  std::vector<HaSubscriptionCallbackRef> &refs = ha_subscription_callback_refs();
  if (scope == HA_SUBSCRIPTION_SCOPE_ALL) {
    for (auto &ref : refs) {
      if (ref.callback && *ref.callback) {
        *ref.callback = nullptr;
      }
    }
    std::vector<HaSubscriptionCallbackRef>().swap(refs);
    return;
  }

  size_t write_index = 0;
  for (size_t read_index = 0; read_index < refs.size(); read_index++) {
    HaSubscriptionCallbackRef &ref = refs[read_index];
    if ((ref.scope & scope) != 0) {
      if (ref.callback && *ref.callback) {
        *ref.callback = nullptr;
      }
      continue;
    }
    if (write_index != read_index) refs[write_index] = std::move(ref);
    write_index++;
  }
  refs.resize(write_index);
  if (refs.empty()) {
    std::vector<HaSubscriptionCallbackRef>().swap(refs);
  }
}

inline void ha_reset_subscription_callbacks(uint32_t scope = HA_SUBSCRIPTION_SCOPE_ALL) {
  if (ha_state_callback_depth() != 0) {
    uint32_t &pending_mask = ha_subscription_callback_reset_pending_mask();
    pending_mask = scope == HA_SUBSCRIPTION_SCOPE_ALL
        ? UINT32_MAX
        : (pending_mask | scope);
    return;
  }
  ha_release_subscription_callbacks_now(scope);
}
#define ESPCONTROL_HA_SUBSCRIPTION_HELPERS_DEFINED 1

inline void ha_track_subscription_callback(
    const std::shared_ptr<HomeAssistantStateCallback> &callback,
    uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT) {
  if (!callback || !*callback) return;
  ha_subscription_callback_refs().push_back({
    callback,
    scope,
  });
}

inline void ha_reset_deferred_state_requests() {
  std::vector<HaDeferredStateRequest>().swap(ha_deferred_state_requests());
}
#define ESPCONTROL_HA_DEFERRED_HELPERS_DEFINED 1

inline void ha_invoke_state_callback(const std::shared_ptr<HomeAssistantStateCallback> &callback,
                                     esphome::StringRef state) {
  if (!callback || !*callback) return;
  uint8_t &depth = ha_state_callback_depth();
  depth++;
  (*callback)(state);
  depth--;
  uint32_t &pending_mask = ha_subscription_callback_reset_pending_mask();
  if (depth == 0 && pending_mask != 0) {
    uint32_t mask = pending_mask;
    pending_mask = 0;
    if (mask == UINT32_MAX) {
      ha_release_subscription_callbacks_now();
    } else {
      ha_release_subscription_callbacks_now(mask);
    }
  }
}

inline bool ha_queue_deferred_state_request(const std::string &entity_id,
                                            const std::string &attribute,
                                            std::shared_ptr<HomeAssistantStateCallback> callback,
                                            bool has_attribute) {
  constexpr size_t HA_DEFERRED_STATE_REQUEST_MAX = 64;
  if (!callback || !*callback) return false;
  std::vector<HaDeferredStateRequest> &requests = ha_deferred_state_requests();
  const uint32_t generation = ha_subscription_generation();
  for (auto &request : requests) {
    if (request.generation == generation &&
        request.has_attribute == has_attribute &&
        request.entity_id == entity_id &&
        request.attribute == attribute) {
      request.callbacks.push_back(std::move(callback));
      return true;
    }
  }
  if (requests.size() >= HA_DEFERRED_STATE_REQUEST_MAX) {
    ESP_LOGW("ha", "Dropping deferred Home Assistant state request for %s: queue full",
             entity_id.c_str());
    return false;
  }
  requests.push_back({
    entity_id,
    attribute,
    {std::move(callback)},
    generation,
    has_attribute,
  });
  return true;
}

inline void ha_flush_deferred_state_requests(size_t max_requests = 8) {
  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) return;
  std::vector<HaDeferredStateRequest> &requests = ha_deferred_state_requests();
  size_t processed = 0;
  while (!requests.empty() && processed < max_requests) {
    HaDeferredStateRequest request = std::move(requests.front());
    requests.erase(requests.begin());
    if (request.callbacks.empty()) continue;
    if (request.generation != ha_subscription_generation()) continue;
    if (!ha_internal_heap_available("deferred Home Assistant state request",
                                    HA_READ_INTERNAL_FREE_MIN_BYTES,
                                    HA_READ_INTERNAL_LARGEST_MIN_BYTES)) {
      requests.insert(requests.begin(), std::move(request));
      return;
    }

    auto callbacks = std::make_shared<std::vector<std::shared_ptr<HomeAssistantStateCallback>>>(
      std::move(request.callbacks));
    if (request.has_attribute) {
      esphome::api::global_api_server->get_home_assistant_state(
        std::move(request.entity_id),
        std::move(request.attribute),
        [callbacks](esphome::StringRef state) {
          for (const auto &callback : *callbacks) {
            ha_invoke_state_callback(callback, state);
          }
        });
    } else {
      esphome::api::global_api_server->get_home_assistant_state(
        std::move(request.entity_id),
        {},
        [callbacks](esphome::StringRef state) {
          for (const auto &callback : *callbacks) {
            ha_invoke_state_callback(callback, state);
          }
        });
    }
    processed++;
  }
  ha_release_deferred_state_request_storage();
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
  if (!ha_api_available() || entity_id.empty() || !callback) return false;
  auto callback_ref = std::make_shared<HomeAssistantStateCallback>(std::move(callback));
  ha_track_subscription_callback(callback_ref, scope);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    [callback_ref](esphome::StringRef state) {
      ha_invoke_state_callback(callback_ref, state);
    });
  return true;
}

inline bool ha_get_state(const std::string &entity_id,
                         HomeAssistantStateCallback callback) {
  if (!ha_api_available() || entity_id.empty() || !callback) return false;
  if (!ha_internal_heap_available("Home Assistant state request",
                                  HA_READ_INTERNAL_FREE_MIN_BYTES,
                                  HA_READ_INTERNAL_LARGEST_MIN_BYTES)) return false;
  auto callback_ref = std::make_shared<HomeAssistantStateCallback>(std::move(callback));
  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) {
    return ha_queue_deferred_state_request(entity_id, std::string(), callback_ref, false);
  }
  esphome::api::global_api_server->get_home_assistant_state(
    entity_id, {},
    [callback_ref](esphome::StringRef state) {
      ha_invoke_state_callback(callback_ref, state);
    });
  return true;
}

inline bool ha_subscribe_attribute(const std::string &entity_id,
                                   const std::string &attribute,
                                   HomeAssistantStateCallback callback,
                                   uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT) {
  if (!ha_api_available() || entity_id.empty() || !callback) return false;
  auto callback_ref = std::make_shared<HomeAssistantStateCallback>(std::move(callback));
  ha_track_subscription_callback(callback_ref, scope);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, attribute,
    [callback_ref](esphome::StringRef state) {
      ha_invoke_state_callback(callback_ref, state);
    });
  return true;
}

inline bool ha_get_attribute(const std::string &entity_id,
                             const std::string &attribute,
                             HomeAssistantStateCallback callback) {
  if (!ha_api_available() || entity_id.empty() || !callback) return false;
  if (!ha_internal_heap_available("Home Assistant attribute request",
                                  HA_READ_INTERNAL_FREE_MIN_BYTES,
                                  HA_READ_INTERNAL_LARGEST_MIN_BYTES)) return false;
  auto callback_ref = std::make_shared<HomeAssistantStateCallback>(std::move(callback));
  if (ha_state_callback_depth() != 0 || !ha_api_state_connected()) {
    return ha_queue_deferred_state_request(entity_id, attribute, callback_ref, true);
  }
  esphome::api::global_api_server->get_home_assistant_state(
    entity_id, attribute,
    [callback_ref](esphome::StringRef state) {
      ha_invoke_state_callback(callback_ref, state);
    });
  return true;
}
