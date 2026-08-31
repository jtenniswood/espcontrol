#pragma once

#include <cstddef>
#include <cstdint>

#ifdef USE_WEBSERVER
#include <cstdio>
#include <cstring>

#include <esp_http_server.h>

#include "configuration_service.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_document.h"
#include "panel_config_http_etag.h"
#include "panel_config_http_context.h"
#include "panel_config_sensitive_data.h"
#include "panel_config_write_status.h"

namespace espcontrol::configuration {

class PanelConfigWriteHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(
      esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_PUT) return false;
    char url_buffer[
        esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buffer);
    return std::strcmp(url.c_str(), "/api/v1/config") == 0;
  }
  size_t maximumBodySize() const override {
    return panel_config_http_context_ready()
               ? panel_config_http_context().document_capacity
               : 0;
  }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!panel_config_http_context_ready()) {
      // AsyncWebServer stops processing when this returns false, so reply here
      // rather than relying on handleRequest() to report the startup state.
      httpd_req_t *raw_request = *request;
      send_status(raw_request, "503 Service Unavailable",
                  "Native configuration is starting");
      reset_upload();
      return false;
    }
    PanelConfigHttpContext &context = panel_config_http_context();
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      return false;
    }
#endif
    return true;
  }

  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data,
                  size_t len, size_t index, size_t total) override {
    PanelConfigHttpContext &context = panel_config_http_context();
    if (index == 0) {
      received_size_ = 0;
      expected_size_ = total;
      body_valid_ = panel_config_http_context_ready() && total > 0 &&
                    total <= context.document_capacity;
    }
    if (!body_valid_ || data == nullptr || index != received_size_ ||
        len > context.document_capacity - received_size_) {
      body_valid_ = false;
      return;
    }
    std::memcpy(context.document + received_size_, data, len);
    received_size_ += len;
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    PanelConfigHttpContext &context = panel_config_http_context();
    if (!panel_config_http_context_ready()) {
      httpd_req_t *raw_request = *request;
      send_status(raw_request, "503 Service Unavailable",
                  "Native configuration is starting");
      reset_upload();
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      reset_upload();
      return;
    }
#endif
    httpd_req_t *raw_request = *request;
    const auto content_type = request->get_header("Content-Type");
    if (!content_type.has_value() ||
        *content_type != "application/vnd.espcontrol.panel-config") {
      send_status(raw_request, "415 Unsupported Media Type",
                  "Panel configuration must use its binary media type");
      reset_upload();
      return;
    }
    if (!body_valid_ || expected_size_ == 0 ||
        received_size_ != expected_size_) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "Invalid panel configuration body");
      reset_upload();
      return;
    }
#ifndef USE_WEBSERVER_AUTH
    if (panel_config_contains_wifi_password(context.document, received_size_)) {
      send_status(raw_request, "403 Forbidden",
                  "Wifi Sharing passwords require web authentication");
      reset_upload();
      return;
    }
#endif
    const auto if_match = request->get_header("If-Match");
    uint32_t expected_generation = 0;
    if (!if_match.has_value() ||
        !parse_panel_config_etag(if_match->c_str(), &expected_generation)) {
      send_status(raw_request, "428 Precondition Required",
                  "A quoted If-Match generation is required");
      reset_upload();
      return;
    }

    const ServiceSaveResult saved = context.service->save_if_generation(
        expected_generation, PANEL_CONFIG_DOCUMENT_VERSION, context.document,
        received_size_);
    reset_upload();
    // ESP-IDF retains header value pointers until httpd_resp_send(). Keep these
    // buffers in handleRequest() so they remain valid through every send below.
    char generation_text[16]{};
    char etag[20]{};
    const PanelConfigWriteResponse response =
        panel_config_write_response(saved.status);
    if (response == PanelConfigWriteResponse::GENERATION_CONFLICT) {
      set_generation_headers(raw_request, saved.generation, generation_text,
                             etag);
      send_status(raw_request, "409 Conflict",
                  "Panel configuration changed on the device");
      return;
    }
    if (response == PanelConfigWriteResponse::BAD_REQUEST) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "Invalid panel configuration document");
      return;
    }
    if (response == PanelConfigWriteResponse::INTERNAL_ERROR) {
      const char *const message =
          saved.status == ServiceStatus::RUNTIME_APPLY_FAILED
              ? "Panel configuration was saved but could not be applied"
              : "Panel configuration could not be saved";
      httpd_resp_send_err(raw_request, HTTPD_500_INTERNAL_SERVER_ERROR,
                          message);
      return;
    }
    set_generation_headers(raw_request, saved.generation, generation_text,
                           etag);
    if (response ==
        PanelConfigWriteResponse::ACCEPTED_WITH_LEGACY_WARNING) {
      httpd_resp_set_hdr(raw_request, "X-Panel-Config-Legacy-Mirror", "failed");
      httpd_resp_set_status(raw_request, "202 Accepted");
    } else {
      httpd_resp_set_status(raw_request, "204 No Content");
    }
    httpd_resp_send(raw_request, nullptr, 0);
  }

 private:
  static void send_status(httpd_req_t *request, const char *status,
                          const char *message) {
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_send(request, message, HTTPD_RESP_USE_STRLEN);
  }

  void reset_upload() {
    received_size_ = 0;
    expected_size_ = 0;
    body_valid_ = false;
  }

  static void set_generation_headers(httpd_req_t *request, uint32_t generation,
                                     char (&generation_text)[16],
                                     char (&etag)[20]) {
    std::snprintf(generation_text, sizeof(generation_text), "%lu",
                  static_cast<unsigned long>(generation));
    std::snprintf(etag, sizeof(etag), "\"%lu\"",
                  static_cast<unsigned long>(generation));
    httpd_resp_set_hdr(request, "ETag", etag);
    httpd_resp_set_hdr(request, "X-Panel-Config-Generation", generation_text);
    httpd_resp_set_hdr(request, "X-Panel-Config-Version", "1");
  }

  size_t received_size_{0};
  size_t expected_size_{0};
  bool body_valid_{false};
};

inline bool register_panel_config_write_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return true;
  server.addHandler(new PanelConfigWriteHandler());
  registered = true;
  return true;
}

}  // namespace espcontrol::configuration
#else
namespace espcontrol::configuration {
class ConfigurationService;
inline bool register_panel_config_write_endpoint(...) {
  return true;
}
}  // namespace espcontrol::configuration
#endif
