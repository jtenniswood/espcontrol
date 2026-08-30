#pragma once

#include <string_view>

namespace espcontrol::configuration {

inline bool panel_config_numbered_name_matches(std::string_view name,
                                               std::string_view prefix,
                                               std::string_view suffix) {
  if (name.size() <= prefix.size() + suffix.size() ||
      name.substr(0, prefix.size()) != prefix ||
      name.substr(name.size() - suffix.size()) != suffix) {
    return false;
  }
  const std::string_view number = name.substr(
      prefix.size(), name.size() - prefix.size() - suffix.size());
  for (const char character : number) {
    if (character < '0' || character > '9') return false;
  }
  return true;
}

inline bool panel_config_legacy_entity_path(std::string_view path) {
  constexpr std::string_view TEXT_PREFIX = "/text/";
  if (path.substr(0, TEXT_PREFIX.size()) != TEXT_PREFIX) return false;
  const std::string_view name = path.substr(TEXT_PREFIX.size());

  if (panel_config_numbered_name_matches(name, "Button ", " Config") ||
      panel_config_numbered_name_matches(name, "button_", "_config") ||
      panel_config_numbered_name_matches(name, "Subpage ", " Config") ||
      panel_config_numbered_name_matches(name, "subpage_", "_config")) {
    return true;
  }

  constexpr std::string_view FRIENDLY_EXT = " Config Ext";
  constexpr std::string_view OBJECT_EXT = "_config_ext";
  if (panel_config_numbered_name_matches(name, "Subpage ", FRIENDLY_EXT) ||
      panel_config_numbered_name_matches(name, "subpage_", OBJECT_EXT)) {
    return true;
  }
  for (char chunk = '2'; chunk <= '7'; ++chunk) {
    char friendly_suffix[] = " Config Ext 2";
    char object_suffix[] = "_config_ext_2";
    friendly_suffix[sizeof(friendly_suffix) - 2] = chunk;
    object_suffix[sizeof(object_suffix) - 2] = chunk;
    if (panel_config_numbered_name_matches(name, "Subpage ", friendly_suffix) ||
        panel_config_numbered_name_matches(name, "subpage_", object_suffix)) {
      return true;
    }
  }
  return false;
}

}  // namespace espcontrol::configuration

#ifdef USE_WEBSERVER

#include <esp_http_server.h>

#include "esphome/components/web_server_idf/web_server_idf.h"

namespace espcontrol::configuration {

class PanelConfigLegacyEntityGuard final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
#ifdef USE_WEBSERVER_AUTH
    (void) request;
    return false;
#else
    if (request->method() != HTTP_GET) return false;
    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);
    return panel_config_legacy_entity_path(
        std::string_view(url.c_str(), url.size()));
#endif
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    httpd_req_t *raw_request = *request;
    httpd_resp_send_err(raw_request, HTTPD_403_FORBIDDEN,
                        "Legacy panel configuration requires web authentication");
  }
};

inline bool register_panel_config_legacy_entity_guard(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return true;
  server.addHandler(new PanelConfigLegacyEntityGuard());
  registered = true;
  return true;
}

}  // namespace espcontrol::configuration

#endif  // USE_WEBSERVER
