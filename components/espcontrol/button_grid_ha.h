#pragma once

#include <array>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>
#include <utility>
#include <vector>

#include "ha_state_broker.h"

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

constexpr size_t HA_STATE_CHANNEL_CAPACITY = MAX_GRID_SLOTS * 8 + 32;
constexpr size_t HA_SCOPED_LEASE_CAPACITY = MAX_GRID_SLOTS * 12 + 64;
constexpr size_t HA_STATE_SUBSCRIBER_CAPACITY =
    HA_SCOPED_LEASE_CAPACITY * 2 + 16;

struct EspHomeHaStateTransport {
  using State = std::string;
  using Callback = std::function<void(State)>;
  struct Handle {
    uint16_t slot = UINT16_MAX;
    uint32_t generation = 0;
  };

  bool available() const { return ha_api_available(); }

  bool open(const std::string &entity_id, const std::string &attribute,
            Callback callback, Handle *handle) {
    if (!available() || !callback || handle == nullptr) return false;
    for (size_t index = 0; index < entries.size(); ++index) {
      Entry &entry = entries[index];
      if (!entry.registered || entry.entity_id != entity_id ||
          entry.attribute != attribute) continue;
      entry.callback = std::move(callback);
      entry.referenced = true;
      *handle = {static_cast<uint16_t>(index), entry.generation};
      return true;
    }

    size_t free_index = entries.size();
    for (size_t index = 0; index < entries.size(); ++index) {
      if (!entries[index].registered) {
        free_index = index;
        break;
      }
    }
    if (free_index == entries.size()) return false;

    auto *server = esphome::api::global_api_server;
    if (server == nullptr) return false;
    Entry &entry = entries[free_index];
    advance_generation(entry.generation);
    entry.entity_id = entity_id;
    entry.attribute = attribute;
    entry.callback = std::move(callback);
    entry.referenced = true;
    entry.registered = true;
    const uint32_t generation = entry.generation;
    const size_t before = server->get_state_subs().size();
    server->subscribe_home_assistant_state(
        entity_id, attribute,
        [this, free_index, generation](esphome::StringRef state) {
          if (free_index >= entries.size()) return;
          Entry &current = entries[free_index];
          if (!current.registered || !current.referenced ||
              current.generation != generation || !current.callback) return;
          current.callback(std::string(state.c_str(), state.size()));
        });
    if (server->get_state_subs().size() <= before) {
      clear_entry(entry);
      return false;
    }
    entry.owned_index = server->get_state_subs().size() - 1;
    *handle = {static_cast<uint16_t>(free_index), generation};
    if (ha_api_state_connected()) request_reset();
    return true;
  }

  void close(Handle handle) {
    if (!valid(handle)) return;
    Entry &entry = entries[handle.slot];
    entry.referenced = false;
    entry.callback = Callback{};
    request_reset();
  }

  void poll() {
    if (!reset_requested) return;
    // ESPHome runs component loops serially. Mark the current clients to close
    // before changing the subscription table; their next loop exits before it
    // can inspect that table, and newly accepted clients see the final table.
    // Waiting for a moment with no clients is not reliable because Home
    // Assistant reconnects immediately and can keep the reset pending forever.
    disconnect_clients();
    auto *server = esphome::api::global_api_server;
    if (server == nullptr) return;
    auto &subscriptions = const_cast<
        std::vector<esphome::api::APIServer::HomeAssistantStateSubscription> &>(
            server->get_state_subs());
    for (;;) {
      size_t selected_slot = entries.size();
      size_t selected_index = 0;
      for (size_t slot = 0; slot < entries.size(); ++slot) {
        const Entry &entry = entries[slot];
        if (entry.registered && !entry.referenced &&
            (selected_slot == entries.size() || entry.owned_index > selected_index)) {
          selected_slot = slot;
          selected_index = entry.owned_index;
        }
      }
      if (selected_slot == entries.size()) break;
      if (selected_index < subscriptions.size()) {
        subscriptions.erase(subscriptions.begin() + selected_index);
        for (auto &entry : entries) {
          if (entry.registered && entry.owned_index > selected_index) --entry.owned_index;
        }
      }
      clear_entry(entries[selected_slot]);
    }
    reset_requested = false;
  }

 private:
  struct Entry {
    std::string entity_id;
    std::string attribute;
    Callback callback{};
    size_t owned_index = 0;
    uint32_t generation = 0;
    bool registered = false;
    bool referenced = false;
  };

  bool valid(Handle handle) const {
    return handle.slot < entries.size() && entries[handle.slot].registered &&
           entries[handle.slot].generation == handle.generation;
  }

  static void advance_generation(uint32_t &generation) {
    ++generation;
    if (generation == 0) ++generation;
  }

  static void clear_entry(Entry &entry) {
    const uint32_t generation = entry.generation;
    entry = {};
    entry.generation = generation;
  }

  void request_reset() {
    reset_requested = true;
  }

  static void disconnect_clients() {
    auto *server = esphome::api::global_api_server;
    if (server == nullptr || !server->is_connected()) return;
    esphome::api::DisconnectRequest request;
    for (const auto &client : server->active_clients()) {
      // Home Assistant is the client that owns the remote state subscription
      // list. Leave diagnostics, OTA and other native API clients connected;
      // closing a logger between authentication and its subscribe-logs request
      // can otherwise prevent it from ever completing a handshake.
      const char *name = client ? client->get_name() : nullptr;
      if (client && client->is_authenticated() && name != nullptr &&
          std::strstr(name, "Home Assistant") != nullptr &&
          !client->is_marked_for_removal()) {
        client->on_disconnect_request(request);
      }
    }
  }

  std::array<Entry, HA_STATE_CHANNEL_CAPACITY> entries{};
  bool reset_requested = false;
};

using EspHomeHaStateBroker = espcontrol::ha::StateBroker<
    EspHomeHaStateTransport, HA_STATE_CHANNEL_CAPACITY,
    HA_STATE_SUBSCRIBER_CAPACITY>;
using EspHomeHaSubscriptions = espcontrol::ha::ScopedStateSubscriptions<
    EspHomeHaStateBroker, HA_SCOPED_LEASE_CAPACITY>;

inline EspHomeHaStateBroker &ha_state_broker() {
#ifdef ESP_PLATFORM
  static EspHomeHaStateBroker *broker = []() {
    void *storage = heap_caps_malloc(
        sizeof(EspHomeHaStateBroker), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (storage == nullptr) {
      ESP_LOGE("ha", "Unable to reserve Home Assistant broker memory");
      std::abort();
    }
    return new (storage) EspHomeHaStateBroker();
  }();
  return *broker;
#else
  static EspHomeHaStateBroker broker;
  return broker;
#endif
}

inline EspHomeHaSubscriptions &ha_state_subscriptions() {
#ifdef ESP_PLATFORM
  static EspHomeHaSubscriptions *subscriptions = []() {
    void *storage = heap_caps_malloc(
        sizeof(EspHomeHaSubscriptions), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (storage == nullptr) {
      ESP_LOGE("ha", "Unable to reserve Home Assistant lease memory");
      std::abort();
    }
    return new (storage) EspHomeHaSubscriptions(ha_state_broker());
  }();
  return *subscriptions;
#else
  static EspHomeHaSubscriptions subscriptions(ha_state_broker());
  return subscriptions;
#endif
}

inline uint32_t &ha_subscription_generation() {
  return ha_state_subscriptions().generation_ref();
}

inline void ha_reset_subscription_callbacks(uint32_t scope = HA_SUBSCRIPTION_SCOPE_ALL) {
  ha_state_subscriptions().reset(scope);
}
#define ESPCONTROL_HA_SUBSCRIPTION_HELPERS_DEFINED 1

inline void ha_reset_deferred_state_requests() {
  // Reads are one-shot broker subscribers and need no unbounded deferred queue.
}
#define ESPCONTROL_HA_DEFERRED_HELPERS_DEFINED 1

inline void bump_ha_subscription_generation() {
  ha_state_subscriptions().begin_generation(
      HA_SUBSCRIPTION_SCOPE_DEFAULT | HA_SUBSCRIPTION_SCOPE_COVER_ART_PROGRESS);
}
#define ESPCONTROL_HA_GENERATION_HELPERS_DEFINED 1

inline void ha_commit_subscription_generation() {
  if (!ha_state_subscriptions().commit_generation()) {
    ESP_LOGW("ha", "Keeping the previous Home Assistant bindings; replacement exceeded fixed capacity");
  }
}

inline void ha_poll_subscription_refresh() {
  ha_state_broker().poll();
}

inline void ha_flush_deferred_state_requests(size_t max_requests = 8) {
  (void) max_requests;
  ha_state_broker().poll();
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
  return ha_state_subscriptions().subscribe(
      entity_id, std::string(),
      [callback = std::move(callback)](std::string state) {
        callback(esphome::StringRef(state));
      },
      scope);
}

inline bool ha_get_state(const std::string &entity_id,
                         HomeAssistantStateCallback callback) {
  if (!ha_internal_heap_available("Home Assistant state request",
                                  HA_READ_INTERNAL_FREE_MIN_BYTES,
                                  HA_READ_INTERNAL_LARGEST_MIN_BYTES)) return false;
  return ha_state_broker().get(
      entity_id, std::string(),
      [callback = std::move(callback)](std::string state) {
        callback(esphome::StringRef(state));
      });
}

inline bool ha_subscribe_attribute(const std::string &entity_id,
                                   const std::string &attribute,
                                   HomeAssistantStateCallback callback,
                                   uint32_t scope = HA_SUBSCRIPTION_SCOPE_DEFAULT) {
  return ha_state_subscriptions().subscribe(
      entity_id, attribute,
      [callback = std::move(callback)](std::string state) {
        callback(esphome::StringRef(state));
      },
      scope);
}

inline bool ha_get_attribute(const std::string &entity_id,
                             const std::string &attribute,
                             HomeAssistantStateCallback callback) {
  if (!ha_internal_heap_available("Home Assistant state request",
                                  HA_READ_INTERNAL_FREE_MIN_BYTES,
                                  HA_READ_INTERNAL_LARGEST_MIN_BYTES)) return false;
  return ha_state_broker().get(
      entity_id, attribute,
      [callback = std::move(callback)](std::string state) {
        callback(esphome::StringRef(state));
      });
}
