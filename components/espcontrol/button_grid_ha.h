#pragma once

#include <utility>

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant API boundary ──────────────────────────────────────

using HomeAssistantStateCallback = std::function<void(esphome::StringRef)>;
using HomeAssistantActionResponseCallback =
  std::function<void(const esphome::api::ActionResponse &)>;

inline bool ha_api_available() {
  return esphome::api::global_api_server != nullptr;
}

inline bool ha_api_connected() {
  return ha_api_available() && esphome::api::global_api_server->is_connected();
}

inline bool ha_api_state_connected() {
  return ha_api_available() && esphome::api::global_api_server->is_connected_with_state_subscription();
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

inline void ha_action_add_entity(esphome::api::HomeassistantActionRequest &req,
                                 const std::string &entity_id) {
  ha_action_add_data(req, "entity_id", entity_id.c_str());
}

inline bool ha_action_send(esphome::api::HomeassistantActionRequest &req) {
  if (!ha_api_state_connected()) return false;
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
                               HomeAssistantStateCallback callback) {
  if (!ha_api_available() || entity_id.empty()) return false;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {}, std::move(callback));
  return true;
}

inline bool ha_subscribe_attribute(const std::string &entity_id,
                                   const std::string &attribute,
                                   HomeAssistantStateCallback callback) {
  if (!ha_api_available() || entity_id.empty()) return false;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, attribute, std::move(callback));
  return true;
}
