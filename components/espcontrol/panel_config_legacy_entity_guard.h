#pragma once

#include <cstddef>
#include <string_view>

namespace espcontrol::configuration {

inline bool panel_config_numbered_name_matches(std::string_view name,
                                               std::string_view prefix,
                                               std::string_view suffix,
                                               size_t *number_out = nullptr) {
  if (name.size() <= prefix.size() + suffix.size() ||
      name.substr(0, prefix.size()) != prefix ||
      name.substr(name.size() - suffix.size()) != suffix) {
    return false;
  }
  const std::string_view number = name.substr(
      prefix.size(), name.size() - prefix.size() - suffix.size());
  size_t parsed = 0;
  for (const char character : number) {
    if (character < '0' || character > '9') return false;
    parsed = parsed * 10 + static_cast<size_t>(character - '0');
  }
  if (parsed == 0) return false;
  if (number_out != nullptr) *number_out = parsed;
  return true;
}

struct PanelConfigLegacyEntityTarget {
  size_t slot{0};
  int subpage_chunk{-1};
};

inline bool panel_config_legacy_entity_target(
    std::string_view path, PanelConfigLegacyEntityTarget *target) {
  if (target == nullptr) return false;
  constexpr std::string_view TEXT_PREFIX = "/text/";
  if (path.substr(0, TEXT_PREFIX.size()) != TEXT_PREFIX) return false;
  const std::string_view name = path.substr(TEXT_PREFIX.size());
  size_t slot = 0;
  if (panel_config_numbered_name_matches(name, "Button ", " Config", &slot) ||
      panel_config_numbered_name_matches(name, "button_", "_config", &slot)) {
    *target = {slot, -1};
    return true;
  }
  if (panel_config_numbered_name_matches(name, "Subpage ", " Config", &slot) ||
      panel_config_numbered_name_matches(name, "subpage_", "_config", &slot)) {
    *target = {slot, 0};
    return true;
  }
  if (panel_config_numbered_name_matches(name, "Subpage ", " Config Ext", &slot) ||
      panel_config_numbered_name_matches(name, "subpage_", "_config_ext", &slot)) {
    *target = {slot, 1};
    return true;
  }
  for (int chunk = 2; chunk <= 7; ++chunk) {
    char friendly_suffix[] = " Config Ext 2";
    char object_suffix[] = "_config_ext_2";
    friendly_suffix[sizeof(friendly_suffix) - 2] = static_cast<char>('0' + chunk);
    object_suffix[sizeof(object_suffix) - 2] = static_cast<char>('0' + chunk);
    if (panel_config_numbered_name_matches(name, "Subpage ", friendly_suffix, &slot) ||
        panel_config_numbered_name_matches(name, "subpage_", object_suffix, &slot)) {
      *target = {slot, chunk};
      return true;
    }
  }
  return false;
}

inline bool panel_config_legacy_entity_path(std::string_view path) {
  PanelConfigLegacyEntityTarget target;
  return panel_config_legacy_entity_target(path, &target);
}

}  // namespace espcontrol::configuration

#ifdef USE_WEBSERVER

#include <array>

#include <esp_http_server.h>

#include "esphome/components/text/text.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_document.h"

namespace espcontrol::configuration {

struct PanelConfigLegacyEntitySources {
  esphome::text::Text *button{nullptr};
  std::array<esphome::text::Text *, 8> subpages{};
};

inline std::array<PanelConfigLegacyEntitySources,
                  PANEL_CONFIG_MAX_SLOT_COUNT> &
panel_config_legacy_entity_sources() {
  static std::array<PanelConfigLegacyEntitySources,
                    PANEL_CONFIG_MAX_SLOT_COUNT> sources{};
  return sources;
}

inline void bind_panel_config_legacy_entity_sources(
    size_t slot, esphome::text::Text *button,
    const std::array<esphome::text::Text *, 8> &subpages) {
  if (slot == 0 || slot > panel_config_legacy_entity_sources().size()) return;
  panel_config_legacy_entity_sources()[slot - 1] = {button, subpages};
}

inline bool panel_config_legacy_source_contains_wifi_password(
    const PanelConfigLegacyEntitySources &sources, int subpage_chunk) {
  bool has_wifi_type = false;
  bool has_password = false;
  const auto inspect = [&](esphome::text::Text *text) {
    if (text == nullptr) return;
    const std::string_view value(text->state);
    has_wifi_type = has_wifi_type ||
                    value.find("wifi_qr") != std::string_view::npos;
    has_password = has_password ||
                   value.find("pass64=") != std::string_view::npos;
  };
  if (subpage_chunk < 0) {
    inspect(sources.button);
  } else {
    for (esphome::text::Text *text : sources.subpages) inspect(text);
  }
  return has_wifi_type && has_password;
}

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
    PanelConfigLegacyEntityTarget target;
    if (!panel_config_legacy_entity_target(
            std::string_view(url.c_str(), url.size()), &target) ||
        target.slot > panel_config_legacy_entity_sources().size()) {
      return false;
    }
    return panel_config_legacy_source_contains_wifi_password(
        panel_config_legacy_entity_sources()[target.slot - 1],
        target.subpage_chunk);
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
