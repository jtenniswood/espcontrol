#pragma once

// Runtime boundary for Companion cards. The grid knows only opaque action IDs;
// the companion service owns pairing, transport and the Mac-specific allowlist.

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#ifdef USE_WEBSERVER
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

struct CompanionAction {
  std::string id;
  std::string label;
};

using CompanionActionSender = std::function<bool(const std::string &, const std::string &)>;

inline std::vector<CompanionAction> &companion_actions() {
  static std::vector<CompanionAction> actions;
  return actions;
}

inline CompanionActionSender &companion_action_sender() {
  static CompanionActionSender sender;
  return sender;
}

inline bool &companion_connected() {
  static bool connected = false;
  return connected;
}

inline void companion_set_actions(std::vector<CompanionAction> actions) {
  actions.erase(std::remove_if(actions.begin(), actions.end(),
    [](const CompanionAction &action) {
      return action.id.empty() || action.id.size() > 96 || action.label.size() > 96;
    }), actions.end());
  companion_actions() = std::move(actions);
}

inline uint32_t companion_next_request_number() {
  static uint32_t request_number = 0;
  return ++request_number;
}

inline void companion_set_connected(bool connected) { companion_connected() = connected; }

inline void register_companion_action_sender(CompanionActionSender sender) {
  companion_action_sender() = std::move(sender);
}

inline bool companion_action_available(const std::string &action_id) {
  if (!companion_connected() || action_id.empty()) return false;
  const auto &actions = companion_actions();
  return std::any_of(actions.begin(), actions.end(), [&action_id](const CompanionAction &action) {
    return action.id == action_id;
  });
}

inline bool invoke_companion_action(const std::string &action_id,
                                    const std::string &request_id) {
  if (!companion_action_available(action_id) || !companion_action_sender()) return false;
  return companion_action_sender()(action_id, request_id);
}

#ifdef USE_WEBSERVER
inline std::string companion_json_escape(const std::string &value) {
  std::string result;
  result.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\': result += "\\\\"; break;
      case '\"': result += "\\\""; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (static_cast<unsigned char>(ch) >= 0x20) result.push_back(ch);
    }
  }
  return result;
}

class CompanionActionsHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buf) == "/companion/actions";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    std::string json = "[";
    bool first = true;
    if (companion_connected()) {
      for (const auto &action : companion_actions()) {
        if (!first) json += ",";
        first = false;
        json += "{\"id\":\"" + companion_json_escape(action.id) +
          "\",\"label\":\"" + companion_json_escape(action.label) + "\"}";
      }
    }
    json += "]";
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }
};

inline void register_companion_actions_endpoint() {
  static bool registered = false;
  if (registered) return;
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (!server) return;
  server->addHandler(new CompanionActionsHandler());
  registered = true;
}
#else
inline void register_companion_actions_endpoint() {}
#endif
