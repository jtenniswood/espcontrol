#pragma once

// Runtime boundary for Companion cards. The grid knows only opaque action IDs;
// the companion service owns pairing, transport and the Mac-specific allowlist.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#ifdef USE_WEBSERVER
#include "esphome/components/web_server_idf/web_server_idf.h"
#endif

#ifdef USE_LVGL
#include "esphome/components/lvgl/lvgl_esphome.h"
#endif

struct CompanionAction {
  std::string id;
  std::string label;
};

using CompanionActionSender = std::function<bool(const std::string &, const std::string &)>;
using CompanionUrlSender = std::function<bool(const std::string &, const std::string &, const std::string &)>;

struct CompanionPairingSnapshot {
  bool available{false};
  bool active{false};
  bool paired{false};
  bool connected{false};
  uint32_t expires_in_seconds{0};
  std::string pairing_code;
  std::string verification_code;
};

using CompanionPairingProvider = std::function<CompanionPairingSnapshot()>;
using CompanionPairingStarter = std::function<CompanionPairingSnapshot()>;

inline void companion_request_card_refresh();

struct CompanionRuntimeState {
  std::mutex mutex;
  std::vector<CompanionAction> actions;
  bool connected{false};
};

struct CompanionRuntimeSnapshot {
  std::vector<CompanionAction> actions;
  bool connected{false};
};

inline CompanionRuntimeState &companion_runtime_state() {
  static CompanionRuntimeState state;
  return state;
}

inline CompanionRuntimeSnapshot companion_runtime_snapshot() {
  auto &state = companion_runtime_state();
  std::lock_guard<std::mutex> lock(state.mutex);
  return {state.actions, state.connected};
}

inline CompanionActionSender &companion_action_sender() {
  static CompanionActionSender sender;
  return sender;
}

inline CompanionUrlSender &companion_url_sender() {
  static CompanionUrlSender sender;
  return sender;
}

inline CompanionPairingProvider &companion_pairing_provider() {
  static CompanionPairingProvider provider;
  return provider;
}

inline CompanionPairingStarter &companion_pairing_starter() {
  static CompanionPairingStarter starter;
  return starter;
}

inline void companion_set_actions(std::vector<CompanionAction> actions) {
  actions.erase(std::remove_if(actions.begin(), actions.end(),
    [](const CompanionAction &action) {
      return action.id.empty() || action.id.size() > 96 || action.label.size() > 96;
    }), actions.end());
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.actions = std::move(actions);
  }
  companion_request_card_refresh();
}

inline uint32_t companion_next_request_number() {
  static std::atomic<uint32_t> request_number{0};
  return ++request_number;
}

inline void companion_set_connected(bool connected) {
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.connected = connected;
  }
  companion_request_card_refresh();
}

inline void register_companion_action_sender(CompanionActionSender sender) {
  companion_action_sender() = std::move(sender);
}

inline void register_companion_url_sender(CompanionUrlSender sender) {
  companion_url_sender() = std::move(sender);
}

inline void register_companion_pairing_callbacks(CompanionPairingProvider provider,
                                                  CompanionPairingStarter starter) {
  companion_pairing_provider() = std::move(provider);
  companion_pairing_starter() = std::move(starter);
}

inline std::vector<std::string> companion_shortcut_parts(const std::string &action_id) {
  static const std::string prefix = "shortcut.";
  if (action_id.rfind(prefix, 0) != 0 || action_id.size() <= prefix.size() || action_id.size() > 96) return {};
  std::vector<std::string> parts;
  size_t start = prefix.size();
  while (start <= action_id.size()) {
    const size_t end = action_id.find('+', start);
    parts.push_back(action_id.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return parts;
}

inline bool companion_shortcut_action_valid(const std::string &action_id) {
  const auto parts = companion_shortcut_parts(action_id);
  if (parts.size() < 2 || parts.size() > 5) return false;
  bool command = false, control = false, option = false, shift = false;
  for (size_t i = 0; i + 1 < parts.size(); i++) {
    const auto &part = parts[i];
    if (part == "command" && !command) command = true;
    else if (part == "control" && !control) control = true;
    else if (part == "option" && !option) option = true;
    else if (part == "shift" && !shift) shift = true;
    else return false;
  }
  if (!command && !control && !option) return false;
  const auto &key = parts.back();
  if (key.size() == 1 && ((key[0] >= 'a' && key[0] <= 'z') || (key[0] >= '0' && key[0] <= '9'))) return true;
  static const std::vector<std::string> named_keys{
    "space", "enter", "tab", "escape", "delete", "forwarddelete",
    "left", "right", "up", "down", "home", "end", "pageup", "pagedown",
    "keycomma", "keyperiod", "keyslash", "keysemicolon", "keyquote", "keybackslash", "keyminus",
    "keyequal", "keybracketleft", "keybracketright", "keybackquote",
  };
  if (std::find(named_keys.begin(), named_keys.end(), key) != named_keys.end()) return true;
  if (key.size() >= 2 && key[0] == 'f') {
    const int number = std::atoi(key.c_str() + 1);
    return number >= 1 && number <= 20 && key == "f" + std::to_string(number);
  }
  return false;
}

inline std::string companion_shortcut_label(const std::string &action_id) {
  if (!companion_shortcut_action_valid(action_id)) return "";
  const auto parts = companion_shortcut_parts(action_id);
  std::string label;
  for (size_t i = 0; i + 1 < parts.size(); i++) {
    if (parts[i] == "command") label += "Cmd+";
    else if (parts[i] == "control") label += "Ctrl+";
    else if (parts[i] == "option") label += "Opt+";
    else if (parts[i] == "shift") label += "Shift+";
  }
  std::string key = parts.back();
  if (key.size() == 1 && key[0] >= 'a' && key[0] <= 'z') key[0] = static_cast<char>(key[0] - 'a' + 'A');
  else if (key == "enter") key = "Return";
  else if (key == "escape") key = "Esc";
  else if (key == "forwarddelete") key = "Forward Delete";
  else if (key == "pageup") key = "Page Up";
  else if (key == "pagedown") key = "Page Down";
  else if (key == "keycomma") key = ",";
  else if (key == "keyperiod") key = ".";
  else if (key == "keyslash") key = "/";
  else if (key == "keysemicolon") key = ";";
  else if (key == "keyquote") key = "'";
  else if (key == "keybackslash") key = "\\";
  else if (key == "keyminus") key = "-";
  else if (key == "keyequal") key = "=";
  else if (key == "keybracketleft") key = "[";
  else if (key == "keybracketright") key = "]";
  else if (key == "keybackquote") key = "`";
  else if (!key.empty()) key[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[0])));
  return label + key;
}

inline bool companion_action_available(const std::string &action_id) {
  if (action_id.empty()) return false;
  const auto snapshot = companion_runtime_snapshot();
  if (!snapshot.connected) return false;
  if (companion_shortcut_action_valid(action_id)) return true;
  return std::any_of(snapshot.actions.begin(), snapshot.actions.end(), [&action_id](const CompanionAction &action) {
    return action.id == action_id;
  });
}

inline std::string companion_encoded_url(const std::string &url_config) {
  static const std::string prefix = "url.";
  if (url_config.rfind(prefix, 0) != 0 || url_config.size() <= prefix.size()) return "";
  const std::string encoded = url_config.substr(prefix.size());
  if (encoded.size() > 1024 ||
      (encoded.rfind("http%3A%2F%2F", 0) != 0 && encoded.rfind("https%3A%2F%2F", 0) != 0)) return "";
  if (!std::all_of(encoded.begin(), encoded.end(), [](unsigned char byte) {
        return byte >= 0x21 && byte <= 0x7e && byte != '|' && byte != ',';
      })) return "";
  return encoded;
}

inline bool companion_url_available(const std::string &app_id, const std::string &url_config) {
  return !companion_encoded_url(url_config).empty() && companion_action_available(app_id);
}

#ifdef USE_LVGL
struct CompanionCardRef {
  lv_obj_t *button = nullptr;
  std::string action_id;
  std::string url_config;
};

inline std::vector<CompanionCardRef> &companion_card_refs() {
  static std::vector<CompanionCardRef> refs;
  return refs;
}

inline std::atomic<bool> &companion_card_refresh_requested() {
  static std::atomic<bool> requested{false};
  return requested;
}

inline void companion_forget_card(lv_obj_t *button) {
  auto &refs = companion_card_refs();
  refs.erase(std::remove_if(refs.begin(), refs.end(), [button](const CompanionCardRef &ref) {
    return ref.button == button;
  }), refs.end());
}

inline void companion_card_deleted(lv_event_t *event) {
  companion_forget_card(static_cast<lv_obj_t *>(lv_event_get_target(event)));
}

inline void companion_track_card(lv_obj_t *button, const std::string &action_id,
                                 const std::string &url_config = "") {
  if (!button) return;
  auto &refs = companion_card_refs();
  auto existing = std::find_if(refs.begin(), refs.end(), [button](const CompanionCardRef &ref) {
    return ref.button == button;
  });
  if (existing != refs.end()) {
    existing->action_id = action_id;
    existing->url_config = url_config;
    return;
  }
  refs.push_back({button, action_id, url_config});
  lv_obj_add_event_cb(button, companion_card_deleted, LV_EVENT_DELETE, nullptr);
}

inline void companion_request_card_refresh() { companion_card_refresh_requested().store(true); }

inline void companion_refresh_cards_if_requested() {
  if (!companion_card_refresh_requested().exchange(false)) return;
  auto &refs = companion_card_refs();
  for (auto it = refs.begin(); it != refs.end();) {
    if (!it->button || !lv_obj_is_valid(it->button)) {
      it = refs.erase(it);
      continue;
    }
    const bool available = it->url_config.empty()
      ? companion_action_available(it->action_id)
      : companion_url_available(it->action_id, it->url_config);
    if (available) {
      lv_obj_clear_state(it->button, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(it->button, LV_STATE_DISABLED);
    }
    ++it;
  }
}
#else
inline void companion_track_card(void *, const std::string &, const std::string & = "") {}
inline void companion_request_card_refresh() {}
inline void companion_refresh_cards_if_requested() {}
#endif

inline bool invoke_companion_action(const std::string &action_id,
                                    const std::string &request_id) {
  if (!companion_action_available(action_id) || !companion_action_sender()) return false;
  return companion_action_sender()(action_id, request_id);
}

inline bool invoke_companion_url(const std::string &app_id,
                                 const std::string &url_config,
                                 const std::string &request_id) {
  const std::string encoded_url = companion_encoded_url(url_config);
  if (encoded_url.empty() || !companion_action_available(app_id) || !companion_url_sender()) return false;
  return companion_url_sender()(app_id, encoded_url, request_id);
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
    const auto snapshot = companion_runtime_snapshot();
    if (snapshot.connected) {
      for (const auto &action : snapshot.actions) {
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

inline std::string companion_pairing_json(const CompanionPairingSnapshot &snapshot) {
  return std::string("{\"available\":") + (snapshot.available ? "true" : "false") +
    ",\"active\":" + (snapshot.active ? "true" : "false") +
    ",\"paired\":" + (snapshot.paired ? "true" : "false") +
    ",\"connected\":" + (snapshot.connected ? "true" : "false") +
    ",\"expires_in_seconds\":" + std::to_string(snapshot.expires_in_seconds) +
    ",\"pairing_code\":\"" + companion_json_escape(snapshot.pairing_code) +
    "\",\"verification_code\":\"" + companion_json_escape(snapshot.verification_code) + "\"}";
}

class CompanionPairingHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET && request->method() != HTTP_POST) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buf) == "/companion/pairing";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    CompanionPairingSnapshot snapshot;
    if (request->method() == HTTP_POST) {
      if (companion_pairing_starter()) snapshot = companion_pairing_starter()();
    } else if (companion_pairing_provider()) {
      snapshot = companion_pairing_provider()();
    }
    const std::string json = companion_pairing_json(snapshot);
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, snapshot.available ? "200 OK" : "404 Not Found");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }
};

inline void register_companion_actions_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return;
  server.addHandler(new CompanionActionsHandler());
  server.addHandler(new CompanionPairingHandler());
  registered = true;
}

inline void register_companion_actions_endpoint() {
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (server) register_companion_actions_endpoint(*server);
}
#else
inline void register_companion_actions_endpoint() {}
#endif
