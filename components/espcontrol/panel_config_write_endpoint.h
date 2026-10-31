#pragma once

#include <cstddef>
#include <cstdint>

#ifdef USE_WEBSERVER
#include <cstdio>
#include <cstring>
#include <memory>
#include <new>

#include <esp_http_server.h>

#include "configuration_main_loop.h"
#include "configuration_service.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_document.h"
#include "panel_config_http_context.h"
#include "panel_config_http_etag.h"
#include "panel_config_upload.h"
#include "panel_config_representation.h"
#include "panel_config_write_status.h"

namespace espcontrol::configuration {

class PanelConfigWriteHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
  struct Upload final : esphome::web_server_idf::RequestBodyContext {
    Upload(size_t total, ConfigurationBuffer::Allocate allocate,
           ConfigurationBuffer::Release release)
        : body(total, allocate, release) {}
    PanelConfigUpload body;
  };

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
               ? PANEL_CONFIG_V2_HTTP_MAX_BYTES
               : 0;
  }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    httpd_req_t *raw_request = *request;
    if (!panel_config_http_context_ready()) {
      send_status(raw_request, "503 Service Unavailable",
                  "Native configuration is starting");
      return false;
    }
    PanelConfigHttpContext &context = panel_config_http_context();
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      return false;
    }
#endif
    const auto content_type = request->get_header("Content-Type");
    if (!content_type.has_value() ||
        *content_type != "application/vnd.espcontrol.panel-config") {
      send_status(raw_request, "415 Unsupported Media Type",
                  "Panel configuration must use its binary media type");
      return false;
    }
    const auto requested = request->get_header(PANEL_CONFIG_VERSION_HEADER);
    uint16_t representation = 1;
    if (!panel_config_requested_version(requested ? requested->c_str() : nullptr, &representation)) {
      send_status(raw_request, "415 Unsupported Media Type", "Unsupported panel configuration representation");
      return false;
    }
    const auto if_match = request->get_header("If-Match");
    uint32_t expected_generation = 0;
    if (!if_match.has_value() ||
        !parse_panel_config_etag(if_match->c_str(), &expected_generation, representation)) {
      send_status(raw_request, "428 Precondition Required",
                  "A quoted If-Match generation is required");
      return false;
    }
    const size_t total = request->contentLength();
    if (!total || total > (representation == 2 ? PANEL_CONFIG_V2_HTTP_MAX_BYTES : context.document_capacity)) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "Invalid panel configuration body");
      return false;
    }
    std::unique_ptr<Upload> upload(
        new (std::nothrow) Upload(total, context.allocate, context.release));
    if (!upload || !upload->body.ready()) {
      send_status(
          raw_request, "503 Service Unavailable",
          "Panel configuration memory is unavailable; retry the request");
      return false;
    }
    request->set_body_context(std::move(upload));
    return true;
  }

  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *request,
                  uint8_t *data, size_t len, size_t index,
                  size_t total) override {
    auto *upload = static_cast<Upload *>(request->body_context());
    if (upload)
      upload->body.append(data, len, index, total);
  }

  void handleRequest(
      esphome::web_server_idf::AsyncWebServerRequest *request) override {
    PanelConfigHttpContext &context = panel_config_http_context();
    if (!panel_config_http_context_ready()) {
      httpd_req_t *raw_request = *request;
      send_status(raw_request, "503 Service Unavailable",
                  "Native configuration is starting");
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(context.username, context.password)) {
      request->requestAuthentication();
      return;
    }
#endif
    httpd_req_t *raw_request = *request;
    const auto content_type = request->get_header("Content-Type");
    if (!content_type.has_value() ||
        *content_type != "application/vnd.espcontrol.panel-config") {
      send_status(raw_request, "415 Unsupported Media Type",
                  "Panel configuration must use its binary media type");
      return;
    }
    auto *upload = static_cast<Upload *>(request->body_context());
    if (upload == nullptr || !upload->body.complete()) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "Invalid panel configuration body");
      return;
    }
    const auto requested = request->get_header(PANEL_CONFIG_VERSION_HEADER);
    uint16_t representation = 1;
    if (!panel_config_requested_version(requested ? requested->c_str() : nullptr, &representation)) {
      send_status(raw_request, "415 Unsupported Media Type", "Unsupported panel configuration representation");
      return;
    }
    const auto if_match = request->get_header("If-Match");
    uint32_t expected_generation = 0;
    if (!if_match.has_value() ||
        !parse_panel_config_etag(if_match->c_str(), &expected_generation, representation)) {
      send_status(raw_request, "428 Precondition Required",
                  "A quoted If-Match generation is required");
      return;
    }

    PanelConfigRepresentation converted(context.allocate, context.release);
    const uint8_t *document = upload->body.data();
    size_t document_size = upload->body.size();
    if (representation == 2) {
      const auto status = converted.convert(2, 1, document, document_size, context.document_capacity);
      if (status != PanelConfigRepresentationStatus::OK) {
        send_status(raw_request, status == PanelConfigRepresentationStatus::UNAVAILABLE
            ? "503 Service Unavailable" : status == PanelConfigRepresentationStatus::TOO_LARGE
            ? "413 Content Too Large" : "400 Bad Request",
            "Configuration cannot be converted losslessly to the stored v1 format");
        return;
      }
      document = converted.data(); document_size = converted.size();
    }
    const auto transaction = context.transactions->save_if_generation(
        expected_generation, PANEL_CONFIG_DOCUMENT_VERSION, document, document_size);
    if (transaction.dispatch != ConfigurationDispatchStatus::COMPLETED) {
      send_status(raw_request, "503 Service Unavailable",
                  "Panel configuration is busy; retry the request");
      return;
    }
    const ServiceSaveResult saved = transaction.result;
    // ESP-IDF retains header value pointers until httpd_resp_send(). Keep these
    // buffers in handleRequest() so they remain valid through every send below.
    char generation_text[16]{};
    char etag[20]{};
    const PanelConfigWriteResponse response =
        panel_config_write_response(saved.status);
    if (response == PanelConfigWriteResponse::GENERATION_CONFLICT) {
      set_generation_headers(raw_request, saved.generation, generation_text,
                             etag, representation);
      send_status(raw_request, "409 Conflict",
                  "Panel configuration changed on the device");
      return;
    }
    if (response == PanelConfigWriteResponse::CAPACITY_EXCEEDED) {
      send_status(raw_request, "413 Content Too Large",
                  "Configuration exceeds this display's page, image or text limits; previous configuration remains active");
      return;
    }
    if (response == PanelConfigWriteResponse::BAD_REQUEST) {
      httpd_resp_send_err(raw_request, HTTPD_400_BAD_REQUEST,
                          "Invalid panel configuration document");
      return;
    }
    if (response == PanelConfigWriteResponse::RUNTIME_UNAVAILABLE) {
      send_status(raw_request, "503 Service Unavailable",
                  "Not enough resources to prepare the panel configuration; previous configuration remains active");
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
                           etag, representation);
    httpd_resp_set_hdr(raw_request, "X-Panel-Config-Durable-Version", saved.document_version == 2 ? "2" : "1");
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

  static void set_generation_headers(httpd_req_t *request, uint32_t generation,
                                     char (&generation_text)[16],
                                     char (&etag)[20], uint16_t representation) {
    std::snprintf(generation_text, sizeof(generation_text), "%lu",
                  static_cast<unsigned long>(generation));
    std::snprintf(etag, sizeof(etag), representation == 2 ? "\"v2-%lu\"" : "\"%lu\"",
                  static_cast<unsigned long>(generation));
    httpd_resp_set_hdr(request, "ETag", etag);
    httpd_resp_set_hdr(request, "X-Panel-Config-Generation", generation_text);
    httpd_resp_set_hdr(request, "X-Panel-Config-Version", representation == 2 ? "2" : "1");
  }

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
