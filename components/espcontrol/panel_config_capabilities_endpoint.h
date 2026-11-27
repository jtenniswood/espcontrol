#pragma once

#include "panel_config_capabilities.h"

#ifdef USE_WEBSERVER
#include <cstring>
#include <memory>
#include <new>

#include <esp_http_server.h>

#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_http_context.h"

namespace espcontrol::configuration {

class PanelConfigCapabilitiesHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);
    return std::strcmp(url.c_str(), "/api/v1/capabilities") == 0;
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    httpd_req_t *raw_request = *request;
    if (!panel_config_http_context_initialization_complete()) {
      httpd_resp_set_status(raw_request, "503 Service Unavailable");
      httpd_resp_set_type(raw_request, "text/plain");
      httpd_resp_send(raw_request, "Native configuration is starting",
                      HTTPD_RESP_USE_STRLEN);
      return;
    }
    // Sized for the compiled-in custom icon list, which has no fixed length.
    const size_t capacity = panel_config_capabilities_json_capacity();
    std::unique_ptr<char[]> response(new (std::nothrow) char[capacity]);
    size_t response_size = 0;
    if (!response ||
        !write_panel_config_capabilities_json(response.get(), capacity,
                                              &response_size)) {
      httpd_resp_send_err(raw_request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Could not build capabilities response");
      return;
    }
    httpd_resp_set_status(raw_request, "200 OK");
    httpd_resp_set_type(raw_request, "application/json");
    httpd_resp_set_hdr(raw_request, "Cache-Control", "no-store");
    httpd_resp_send(raw_request, response.get(), response_size);
  }
};

inline bool register_panel_config_capabilities_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return true;
  server.addHandler(new PanelConfigCapabilitiesHandler());
  registered = true;
  return true;
}

inline bool register_panel_config_capabilities_endpoint() {
  auto *server = esphome::web_server_idf::global_async_web_server();
  return server != nullptr && register_panel_config_capabilities_endpoint(*server);
}

}  // namespace espcontrol::configuration
#else
namespace espcontrol::configuration {

inline bool register_panel_config_capabilities_endpoint(...) { return true; }

}  // namespace espcontrol::configuration
#endif
