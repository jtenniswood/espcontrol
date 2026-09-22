#pragma once

#ifdef USE_WEBSERVER

#include <array>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>

#include <esp_http_server.h>

#include "button_grid_ha.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/network/ip_address.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"

namespace espcontrol {

// The browser uses a short-polling endpoint because ESP-IDF's request object
// cannot safely be retained while an ESPHome action is travelling to HA.
constexpr size_t HA_ENTITY_CATALOG_MAX_PENDING = 2;
constexpr uint32_t HA_ENTITY_CATALOG_TIMEOUT_MS = 15000;
constexpr uint32_t HA_ENTITY_CATALOG_RESULT_RETENTION_MS = 60000;
constexpr size_t HA_ENTITY_CATALOG_MAX_QUERY = 120;
constexpr size_t HA_ENTITY_CATALOG_MAX_FILTER = 120;
constexpr size_t HA_ENTITY_CATALOG_MAX_BODY = 24000;
constexpr uint32_t HA_ENTITY_CATALOG_MAX_LIMIT = 50;
constexpr uint32_t HA_ENTITY_CATALOG_MAX_CURSOR = 10000;

struct HaEntityCatalogPending {
  enum class State : uint8_t { FREE, PENDING, COMPLETE, ERROR };
  State state{State::FREE};
  uint32_t request_id{0};
  uint32_t call_id{0};
  uint32_t created_ms{0};
  std::string body;
  std::string error;
};

inline std::array<HaEntityCatalogPending, HA_ENTITY_CATALOG_MAX_PENDING> &
ha_entity_catalog_pending() {
  static std::array<HaEntityCatalogPending, HA_ENTITY_CATALOG_MAX_PENDING> slots;
  return slots;
}

inline std::mutex &ha_entity_catalog_mutex() {
  static std::mutex mutex;
  return mutex;
}

inline uint32_t ha_entity_catalog_next_request_id() {
  static uint32_t value = 1;
  const uint32_t result = value++;
  return result == 0 ? value++ : result;
}

inline uint32_t ha_entity_catalog_next_call_id() {
  static uint32_t value = 310000;
  const uint32_t result = value++;
  return result == 0 ? value++ : result;
}

inline HaEntityCatalogPending *ha_entity_catalog_find(uint32_t request_id) {
  for (auto &slot : ha_entity_catalog_pending()) {
    if (slot.state != HaEntityCatalogPending::State::FREE &&
        slot.request_id == request_id) {
      return &slot;
    }
  }
  return nullptr;
}

inline std::string ha_entity_catalog_json_status(const char *status,
                                                 uint32_t request_id,
                                                 const char *error = nullptr) {
  return esphome::json::build_json([&](JsonObject root) {
    root["status"] = status;
    root["request_id"] = request_id;
    if (error != nullptr) root["error"] = error;
  });
}

inline void ha_entity_catalog_complete(uint32_t request_id,
                                       const esphome::api::ActionResponse &response) {
  std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
  HaEntityCatalogPending *slot = ha_entity_catalog_find(request_id);
  if (slot == nullptr || slot->state != HaEntityCatalogPending::State::PENDING) return;
  if (!response.is_success()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->created_ms = esphome::millis();
    slot->error = response.get_error_message().c_str();
    return;
  }
  auto root = response.get_json();
  auto payload = root["response"];
  if (payload.isNull()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->created_ms = esphome::millis();
    slot->error = "Home Assistant returned no catalog response";
    return;
  }
  slot->body.clear();
  serializeJson(payload, slot->body);
  if (slot->body.empty()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->created_ms = esphome::millis();
    slot->error = "Home Assistant returned an empty catalog response";
    return;
  }
  if (slot->body.size() > HA_ENTITY_CATALOG_MAX_BODY) {
    slot->body.clear();
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->created_ms = esphome::millis();
    slot->error = "Home Assistant entity catalog response is too large";
    return;
  }
  slot->state = HaEntityCatalogPending::State::COMPLETE;
  slot->created_ms = esphome::millis();
}

inline void ha_entity_catalog_schedule_cancel(uint32_t call_id, std::string reason) {
  if (call_id == 0) return;
  esphome::App.scheduler.set_timeout(
      nullptr, call_id, 0,
      [call_id, reason = std::move(reason)]() {
        ha_cancel_action_response_callback(call_id, reason.c_str());
      });
}

inline bool ha_entity_catalog_send(HaEntityCatalogPending &slot,
                                   const std::string &query,
                                   const std::string &field,
                                   const std::string &area,
                                   const std::string &device_id,
                                   const std::string &capabilities,
                                   bool include_hidden,
                                   bool include_disabled,
                                   const std::string &limit,
                                   const std::string &cursor) {
  if (!ha_api_state_connected() || !ha_internal_heap_available("entity catalog")) {
    return false;
  }
  esphome::api::HomeassistantActionRequest request;
  const uint32_t call_id = ha_entity_catalog_next_call_id();
  const size_t data_count = 8 + (capabilities.empty() ? 0 : 1);
  if (!ha_action_begin(request, "espcontrol.search_entities", false, data_count, call_id)) {
    return false;
  }
  request.wants_response = true;
  ha_action_add_data(request, "query", query.c_str());
  ha_action_add_data(request, "field", field.c_str());
  ha_action_add_data(request, "area", area.c_str());
  ha_action_add_data(request, "device_id", device_id.c_str());
  ha_action_add_data(request, "include_hidden", include_hidden ? "true" : "false");
  ha_action_add_data(request, "include_disabled", include_disabled ? "true" : "false");
  if (!capabilities.empty()) {
    ha_action_add_data(request, "capabilities", capabilities.c_str());
  }
  ha_action_add_data(request, "limit", limit.c_str());
  ha_action_add_data(request, "cursor", cursor.c_str());
  slot.call_id = call_id;
  if (!ha_register_action_response_callback(
          call_id, [request_id = slot.request_id](
                        const esphome::api::ActionResponse &response) {
            ha_entity_catalog_complete(request_id, response);
          })) {
    return false;
  }
  if (!ha_action_send(request)) {
    ha_entity_catalog_schedule_cancel(call_id, "Home Assistant action could not be sent");
    return false;
  }
  return true;
}

inline void ha_entity_catalog_schedule_send(
    uint32_t request_id,
    std::string query,
    std::string field,
    std::string area,
    std::string device_id,
    std::string capabilities,
    bool include_hidden,
    bool include_disabled,
    std::string limit,
    std::string cursor) {
  // ESPHome's API connection is owned by the main loop. The HTTP server runs
  // on another task, so dispatch the native action through the scheduler
  // instead of touching APIConnection directly from the request handler.
  esphome::App.scheduler.set_timeout(
      nullptr, request_id, 0,
      [request_id, query = std::move(query), field = std::move(field),
       area = std::move(area), device_id = std::move(device_id),
       capabilities = std::move(capabilities), include_hidden, include_disabled,
       limit = std::move(limit), cursor = std::move(cursor)]() mutable {
        HaEntityCatalogPending *slot = nullptr;
        {
          std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
          slot = ha_entity_catalog_find(request_id);
          if (slot == nullptr || slot->state != HaEntityCatalogPending::State::PENDING) return;
        }
        if (ha_entity_catalog_send(
                *slot, query, field, area, device_id, capabilities, include_hidden,
                include_disabled, limit, cursor)) {
          return;
        }
        std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
        if (slot->state == HaEntityCatalogPending::State::PENDING) {
          slot->state = HaEntityCatalogPending::State::ERROR;
          slot->created_ms = esphome::millis();
          slot->error = "Home Assistant is not ready for entity catalog requests";
        }
      });
}

class HaEntityCatalogHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buffer[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buffer) == "/api/v1/ha/entities/search";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (panel_identity == nullptr || !panel_identity->ready()) {
      request->send(503, "application/json", "{\"error\":\"panel identity unavailable\"}");
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(panel_identity->username(), panel_identity->password())) {
      request->requestAuthentication();
      return;
    }
#endif
    if (!origin_allowed(request)) {
      request->send(403, "application/json", "{\"error\":\"cross_origin_forbidden\"}");
      return;
    }
    const std::string request_id_text = request->arg("request_id");
    if (!request_id_text.empty()) {
      send_existing(request, static_cast<uint32_t>(std::strtoul(request_id_text.c_str(), nullptr, 10)));
      return;
    }
    start_request(request);
  }

 private:
  static bool origin_allowed(esphome::web_server_idf::AsyncWebServerRequest *request) {
    const auto origin = request->get_header("Origin");
    // Browser fetches include an independent Fetch Metadata signal. Reject a
    // cross-site request even when DNS rebinding makes the Host header match
    // the attacker's Origin; scripts cannot set this header themselves.
    const auto fetch_site = request->get_header("Sec-Fetch-Site");
    if (fetch_site.has_value() && *fetch_site != "same-origin") return false;
    if (!origin.has_value() || origin->empty()) return true;
    const auto scheme_end = origin->find("://");
    if (scheme_end == std::string::npos) return false;
    const size_t authority_start = scheme_end + 3;
    const size_t authority_end = origin->find('/', authority_start);
    const std::string authority = origin->substr(
        authority_start, authority_end == std::string::npos ? std::string::npos : authority_end - authority_start);
    if (authority.empty() || authority.find('@') != std::string::npos) return false;
    std::string host = authority;
    if (host.front() == '[') {
      const size_t closing = host.find(']');
      if (closing == std::string::npos) return false;
      host = host.substr(1, closing - 1);
    } else {
      const size_t port = host.find(':');
      if (port != std::string::npos) host.resize(port);
    }
    if (host.empty()) return false;
    for (char &character : host) {
      if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
    }
    std::string expected = panel_identity->target_hostname();
    for (char &character : expected) {
      if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
    }
    if (host == expected || host == expected + ".local") return true;
    char address[esphome::network::IP_ADDRESS_BUFFER_SIZE];
    for (const auto &ip : esphome::network::get_ip_addresses()) {
      std::string known = ip.str_to(address);
      for (char &character : known) {
        if (character >= 'A' && character <= 'Z') character += 'a' - 'A';
      }
      if (host == known) return true;
    }
    return false;
  }

  static void send_existing(
      esphome::web_server_idf::AsyncWebServerRequest *request,
      uint32_t request_id) {
    int status = 200;
    uint32_t cancel_call_id = 0;
    std::string body;
    {
      std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
      HaEntityCatalogPending *slot = ha_entity_catalog_find(request_id);
      if (slot == nullptr) {
        status = 404;
        body = "{\"error\":\"unknown catalog request\"}";
      } else if (slot->state == HaEntityCatalogPending::State::PENDING &&
                 esphome::millis() - slot->created_ms > HA_ENTITY_CATALOG_TIMEOUT_MS) {
        cancel_call_id = slot->call_id;
        status = 504;
        body = ha_entity_catalog_json_status(
            "error", request_id, "Home Assistant entity catalog request timed out");
        slot->state = HaEntityCatalogPending::State::FREE;
      } else if (slot->state == HaEntityCatalogPending::State::PENDING) {
        // Pending is represented in JSON because the web-server adapter does
        // not preserve 202 for browser clients.
        body = ha_entity_catalog_json_status("pending", request_id);
      } else if (slot->state == HaEntityCatalogPending::State::ERROR) {
        status = 502;
        body = ha_entity_catalog_json_status("error", request_id, slot->error.c_str());
        slot->state = HaEntityCatalogPending::State::FREE;
      } else {
        body = slot->body;
        slot->state = HaEntityCatalogPending::State::FREE;
      }
    }
    if (cancel_call_id != 0) {
      ha_entity_catalog_schedule_cancel(cancel_call_id, "entity catalog request timed out");
    }
    request->send(status, "application/json", body.c_str());
  }

  static void start_request(esphome::web_server_idf::AsyncWebServerRequest *request) {
    const std::string query = request->arg("query");
    if (query.size() > HA_ENTITY_CATALOG_MAX_QUERY) {
      request->send(400, "application/json", "{\"error\":\"query too long\"}");
      return;
    }
    const std::string field = request->arg("field").empty() ? "entity" : request->arg("field");
    const std::string area = request->arg("area");
    const std::string device_id = request->arg("device_id");
    const std::string capabilities = request->arg("capabilities");
    const bool include_hidden = request->arg("include_hidden") == "1" ||
                                request->arg("include_hidden") == "true";
    const bool include_disabled = request->arg("include_disabled") == "1" ||
                                  request->arg("include_disabled") == "true";
    const std::string limit = request->arg("limit").empty() ? "25" : request->arg("limit");
    const std::string cursor = request->arg("cursor").empty() ? "0" : request->arg("cursor");
    if (field.size() > HA_ENTITY_CATALOG_MAX_FILTER || area.size() > HA_ENTITY_CATALOG_MAX_FILTER ||
        device_id.size() > HA_ENTITY_CATALOG_MAX_FILTER ||
        capabilities.size() > HA_ENTITY_CATALOG_MAX_FILTER) {
      request->send(400, "application/json", "{\"error\":\"filter too long\"}");
      return;
    }
    char *limit_end = nullptr;
    char *cursor_end = nullptr;
    const unsigned long parsed_limit = std::strtoul(limit.c_str(), &limit_end, 10);
    const unsigned long parsed_cursor = std::strtoul(cursor.c_str(), &cursor_end, 10);
    if (limit_end == limit.c_str() || *limit_end != '\0' || parsed_limit < 1 ||
        parsed_limit > HA_ENTITY_CATALOG_MAX_LIMIT || cursor_end == cursor.c_str() ||
        *cursor_end != '\0' || parsed_cursor > HA_ENTITY_CATALOG_MAX_CURSOR) {
      request->send(400, "application/json", "{\"error\":\"invalid pagination\"}");
      return;
    }
    HaEntityCatalogPending *slot = nullptr;
    uint32_t stale_call_id = 0;
    {
      std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
      for (auto &candidate : ha_entity_catalog_pending()) {
        if ((candidate.state == HaEntityCatalogPending::State::COMPLETE ||
             candidate.state == HaEntityCatalogPending::State::ERROR) &&
            esphome::millis() - candidate.created_ms > HA_ENTITY_CATALOG_RESULT_RETENTION_MS) {
          candidate.state = HaEntityCatalogPending::State::FREE;
          candidate.body.clear();
          candidate.error.clear();
        }
        if (candidate.state == HaEntityCatalogPending::State::PENDING &&
            esphome::millis() - candidate.created_ms > HA_ENTITY_CATALOG_TIMEOUT_MS) {
          stale_call_id = candidate.call_id;
          candidate.state = HaEntityCatalogPending::State::ERROR;
          candidate.created_ms = esphome::millis();
          candidate.error = "Home Assistant entity catalog request timed out";
        }
        if (candidate.state == HaEntityCatalogPending::State::FREE) {
          slot = &candidate;
          break;
        }
      }
      if (slot == nullptr) {
        request->send(429, "application/json", "{\"error\":\"catalog busy\"}");
        return;
      }
      slot->state = HaEntityCatalogPending::State::PENDING;
      slot->request_id = ha_entity_catalog_next_request_id();
      slot->created_ms = esphome::millis();
      slot->body.clear();
      slot->error.clear();
    }
    if (stale_call_id != 0) {
      ha_entity_catalog_schedule_cancel(stale_call_id, "entity catalog request timed out");
    }
    ha_entity_catalog_schedule_send(
        slot->request_id, query, field, area, device_id, capabilities, include_hidden,
        include_disabled, limit, cursor);
    const std::string body = ha_entity_catalog_json_status("pending", slot->request_id);
    // See the polling response above: pending is represented in the JSON
    // payload because the web-server adapter does not preserve 202 here.
    request->send(200, "application/json", body.c_str());
  }
};

inline bool register_ha_entity_catalog_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) {
    server.addHandler(new HaEntityCatalogHandler());
    registered = true;
  }
  return true;
}

}  // namespace espcontrol

#endif  // USE_WEBSERVER
