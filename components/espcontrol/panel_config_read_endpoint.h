#pragma once

#include <cstddef>
#include <cstdint>

#ifdef USE_WEBSERVER
#include <array>
#include <cstdio>
#include <cstring>

#include <esp_http_server.h>

#include "configuration_service.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_http_context.h"
#include "panel_config_sensitive_data.h"

namespace espcontrol::configuration {

class PanelConfigReadHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);
    return std::strcmp(url.c_str(), "/api/v1/config") == 0;
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    PanelConfigHttpContext &context = panel_config_http_context();
    if (!panel_config_http_context_ready()) {
      httpd_req_t *raw_request = *request;
      httpd_resp_set_status(raw_request, "503 Service Unavailable");
      httpd_resp_set_type(raw_request, "text/plain");
      httpd_resp_send(raw_request, "Native configuration is starting",
                      HTTPD_RESP_USE_STRLEN);
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      return;
    }
#endif
    httpd_req_t *raw_request = *request;
    const ServiceLoadResult loaded =
        context.service->load(context.document, context.document_capacity);
    if (loaded.status == ServiceStatus::EMPTY) {
      httpd_resp_send_err(raw_request, HTTPD_404_NOT_FOUND,
                          "No native configuration is stored");
      return;
    }
    if (!loaded.ok()) {
      httpd_resp_send_err(raw_request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Native configuration is unavailable");
      return;
    }
#ifndef USE_WEBSERVER_AUTH
    if (panel_config_contains_wifi_password(context.document,
                                            loaded.document_size)) {
      httpd_resp_send_err(raw_request, HTTPD_403_FORBIDDEN,
                          "Wifi Sharing passwords require web authentication");
      return;
    }
#endif
    char generation[16]{};
    char etag[20]{};
    char version[8]{};
    std::snprintf(generation, sizeof(generation), "%lu",
                  static_cast<unsigned long>(loaded.generation));
    std::snprintf(etag, sizeof(etag), "\"%lu\"",
                  static_cast<unsigned long>(loaded.generation));
    std::snprintf(version, sizeof(version), "%u",
                  static_cast<unsigned>(loaded.document_version));
    httpd_resp_set_status(raw_request, "200 OK");
    httpd_resp_set_type(raw_request,
                        "application/vnd.espcontrol.panel-config");
    httpd_resp_set_hdr(raw_request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(raw_request, "ETag", etag);
    httpd_resp_set_hdr(raw_request, "X-Panel-Config-Generation", generation);
    httpd_resp_set_hdr(raw_request, "X-Panel-Config-Version", version);
    httpd_resp_send(raw_request, reinterpret_cast<const char *>(context.document),
                    loaded.document_size);
  }
};

inline bool register_panel_config_read_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return true;
  server.addHandler(new PanelConfigReadHandler());
  registered = true;
  return true;
}

}  // namespace espcontrol::configuration
#else
namespace espcontrol::configuration {
class ConfigurationService;
inline bool register_panel_config_read_endpoint(...) {
  return true;
}
}  // namespace espcontrol::configuration
#endif
