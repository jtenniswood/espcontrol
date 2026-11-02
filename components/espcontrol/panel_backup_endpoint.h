#pragma once
#ifdef USE_WEBSERVER
#include <algorithm>
#include <cmath>
#include <ctime>
#include <functional>

#include "esphome/components/json/json_util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "configuration_service.h"
#include "panel_backup.h"
#include "panel_config_http_context.h"
#include "panel_identity.h"

namespace espcontrol::backup {
using SettingsWriter = std::function<void(JsonObject, JsonObject)>;
struct BackupContext {
  const char *profile{nullptr};
  size_t slots{0};
  SettingsWriter settings_writer;
};
inline BackupContext &context() {
  static BackupContext value;
  return value;
}
inline void configure(const char *profile, size_t slots, SettingsWriter writer) {
  context() = {profile, slots, std::move(writer)};
}

class PanelBackupHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->method() == HTTP_GET && request->url_to(path) == "/api/v1/backup";
  }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    using namespace configuration;
    // Authenticate before loading settings or returning startup/storage status.
    if (panel_identity == nullptr || !panel_identity->ready()) {
      request->send(503, "text/plain", "Backup is starting");
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(panel_identity->username(), panel_identity->password())) {
      request->requestAuthentication();
      return;
    }
#endif
    auto &backup = context();
    if (!panel_config_http_context_ready() || backup.profile == nullptr || !backup.settings_writer) {
      request->send(503, "text/plain", "Backup is unavailable");
      return;
    }
    auto &config = panel_config_http_context();
    const auto loaded = config.service->load(config.document, config.document_capacity);
    if (!loaded.ok()) {
      // Never present a settings-only or empty-layout file as a complete backup.
      request->send(503, "text/plain", "Saved configuration is unavailable");
      return;
    }
    if (!valid_backup_document(config.document, loaded.document_size, backup.slots)) {
      request->send(500, "text/plain", "Saved configuration is invalid");
      return;
    }
    // Settings are small; the potentially large cards, subpages and native
    // payload below are streamed directly from the stored document.
    JsonDocument metadata(esphome::json::heap_json_allocator());
    JsonObject settings_object = metadata["settings"].to<JsonObject>();
    JsonObject screen_object = metadata["screen"].to<JsonObject>();
    backup.settings_writer(settings_object, screen_object);
    JsonObject identity_object = metadata["identity"].to<JsonObject>();
    identity_object["version"] = 1;
    identity_object["name"] = panel_identity->saved_name();
    identity_object["hostname"] = panel_identity->target_hostname();
    identity_object["mac_suffix"] = panel_identity->suffix();
    if (metadata.overflowed()) {
      request->send(500, "text/plain", "Backup settings could not be serialized");
      return;
    }
    std::string settings, screen, identity;
    serializeJson(settings_object, settings);
    serializeJson(screen_object, screen);
    serializeJson(identity_object, identity);
    char exported_at[25]{};
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (now >= 1577836800 && gmtime_r(&now, &utc) != nullptr)
      std::strftime(exported_at, sizeof(exported_at), "%Y-%m-%dT%H:%M:%SZ", &utc);
    httpd_req_t *raw = *request;
    const char *filename = "attachment; filename=\"espcontrol-backup.json\"";
    httpd_resp_set_type(raw, "application/json");
    httpd_resp_set_hdr(raw, "Content-Disposition", filename);
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    std::string buffer;
    buffer.reserve(1024);
    auto sink = [&](const std::string &part) {
      for (size_t offset = 0; offset < part.size();) {
        const size_t count = std::min(size_t(1024) - buffer.size(), part.size() - offset);
        buffer.append(part, offset, count);
        offset += count;
        if (buffer.size() == 1024) {
          if (httpd_resp_send_chunk(raw, buffer.data(), buffer.size()) != ESP_OK) return false;
          buffer.clear();
        }
      }
      return true;
    };
    if (!write_backup(config.document, loaded.document_size, backup.slots,
                      settings, screen, identity, exported_at, sink)) return;
    if (!buffer.empty() && httpd_resp_send_chunk(raw, buffer.data(), buffer.size()) != ESP_OK) return;
    httpd_resp_send_chunk(raw, nullptr, 0);
  }
};
inline void register_panel_backup_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new PanelBackupHandler()); registered = true; }
}
}  // namespace espcontrol::backup
#endif
