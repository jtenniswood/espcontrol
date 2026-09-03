#pragma once

#include <cstdint>
#include <string>

#include "esphome/core/helpers.h"
#include "esphome/core/preferences.h"

#include "button_grid_ha.h"
#include "companion_controls.h"

namespace espcontrol::connectors {

struct ConnectorPreference {
  uint32_t version{1};
  uint8_t home_assistant_configured{0};
  uint8_t home_assistant_actions_confirmed{0};
  uint8_t reserved[2]{};
};

struct ConnectorStatus {
  bool onboarding_complete{false};
  bool home_assistant_configured{false};
  bool home_assistant_connected{false};
  bool home_assistant_actions_confirmed{false};
  bool companion_available{false};
  bool companion_paired{false};
  bool companion_connected{false};
};

class ConnectorStateService {
 public:
  void setup(bool existing_layout, const char *web_auth_username,
             const char *web_auth_password) {
    web_auth_username_ = web_auth_username == nullptr ? "" : web_auth_username;
    web_auth_password_ = web_auth_password == nullptr ? "" : web_auth_password;
    if (initialized_) return;
    preference_ = esphome::global_preferences->make_preference<ConnectorPreference>(
        esphome::fnv1a_hash("espcontrol_connectors"));
    const bool loaded = preference_.load(&state_);
    if (!loaded || state_.version != 1) state_ = ConnectorPreference{};
    // Before connector-aware onboarding, every configured layout necessarily
    // came through the Home Assistant setup path. Preserve that experience on
    // upgrade instead of sending an established panel back to first-run setup.
    if (existing_layout && !configured()) {
      state_.home_assistant_configured = 1;
      state_.home_assistant_actions_confirmed = 1;
      save_();
    }
    initialized_ = true;
  }

  bool confirm_home_assistant() {
    if (!ha_api_state_connected()) return false;
    state_.home_assistant_configured = 1;
    state_.home_assistant_actions_confirmed = 1;
    save_();
    return true;
  }

  bool configured() const {
    return state_.home_assistant_configured != 0 || companion_paired_();
  }

  const char *web_auth_username() const { return web_auth_username_; }
  const char *web_auth_password() const { return web_auth_password_; }

  ConnectorStatus status() const {
    const CompanionPairingSnapshot companion = companion_pairing_provider()
        ? companion_pairing_provider()()
        : CompanionPairingSnapshot{};
    ConnectorStatus result;
    result.home_assistant_configured = state_.home_assistant_configured != 0;
    result.home_assistant_connected = ha_api_state_connected();
    result.home_assistant_actions_confirmed =
        state_.home_assistant_actions_confirmed != 0;
    result.companion_available = companion.available;
    result.companion_paired = companion.paired;
    result.companion_connected = companion.connected;
    result.onboarding_complete = result.home_assistant_configured ||
                                 result.companion_paired;
    return result;
  }

 private:
  bool companion_paired_() const {
    if (!companion_pairing_provider()) return false;
    return companion_pairing_provider()().paired;
  }

  void save_() { preference_.save(&state_); }

  esphome::ESPPreferenceObject preference_{};
  ConnectorPreference state_{};
  bool initialized_{false};
  const char *web_auth_username_{""};
  const char *web_auth_password_{""};
};

inline ConnectorStateService &connector_state_service() {
  static ConnectorStateService service;
  return service;
}

inline bool onboarding_complete() {
  return connector_state_service().status().onboarding_complete;
}

#ifdef USE_WEBSERVER
inline std::string connector_status_json(const ConnectorStatus &status) {
  auto boolean = [](bool value) { return value ? "true" : "false"; };
  return std::string("{\"onboarding_complete\":") +
      boolean(status.onboarding_complete) +
      ",\"home_assistant\":{\"available\":true,\"configured\":" +
      boolean(status.home_assistant_configured) +
      ",\"connected\":" + boolean(status.home_assistant_connected) +
      ",\"actions_confirmed\":" +
      boolean(status.home_assistant_actions_confirmed) +
      "},\"mac_companion\":{\"available\":" +
      boolean(status.companion_available) +
      ",\"configured\":" + boolean(status.companion_paired) +
      ",\"paired\":" + boolean(status.companion_paired) +
      ",\"connected\":" + boolean(status.companion_connected) + "}}";
}

class ConnectorStatusHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET && request->method() != HTTP_POST) {
      return false;
    }
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const auto url = request->url_to(url_buf);
    return url == "/connectors/status" ||
           url == "/connectors/home-assistant/complete";
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
#ifdef USE_WEBSERVER_AUTH
    const ConnectorStateService &service = connector_state_service();
    if (!request->authenticate(service.web_auth_username(),
                               service.web_auth_password())) {
      request->requestAuthentication();
      return;
    }
#endif
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const auto url = request->url_to(url_buf);
    bool accepted = true;
    if (url == "/connectors/home-assistant/complete") {
      accepted = request->method() == HTTP_POST &&
                 connector_state_service().confirm_home_assistant();
    } else if (request->method() != HTTP_GET) {
      accepted = false;
    }
    const std::string json = connector_status_json(
        connector_state_service().status());
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, accepted ? "200 OK" : "409 Conflict");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }
};

inline void register_connector_status_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return;
  server.addHandler(new ConnectorStatusHandler());
  registered = true;
}
#else
inline void register_connector_status_endpoint(...) {}
#endif

}  // namespace espcontrol::connectors
