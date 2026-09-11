#pragma once
#ifdef USE_WEBSERVER
#include <array>
#include <cstring>
#include "esphome/components/json/json_util.h"
#include "esphome/components/network/util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"

namespace espcontrol {
class PanelIdentityHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_POST) &&
           request->url_to(path) == "/api/v1/identity";
  }
  size_t maximumBodySize() const override { return body_.size(); }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    received_ = 0;
    valid_ = false;
    return authorize(request);
  }
  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data,
                  size_t len, size_t index, size_t total) override {
    if (index == 0) { received_ = 0; valid_ = total > 0 && total <= body_.size(); }
    if (!valid_ || data == nullptr || index != received_ || len > body_.size() - received_) {
      valid_ = false;
      return;
    }
    std::memcpy(body_.data() + received_, data, len);
    received_ += len;
    expected_ = total;
  }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!authorize(request)) return;
    httpd_req_t *raw = *request;
    if (request->method() == HTTP_POST) {
      const auto content_type = request->get_header("Content-Type");
      if (!content_type.has_value() || *content_type != "application/json") {
        httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Expected application/json");
        return;
      }
      std::string name;
      const bool parsed = valid_ && received_ == expected_ &&
          esphome::json::parse_json(body_.data(), received_, [&](JsonObject root) {
            if (!root["name"].is<const char *>()) return false;
            const JsonString value = root["name"].as<JsonString>();
            return normalize_panel_name(std::string(value.c_str(), value.size()), name);
          });
      valid_ = false;
      if (!parsed) {
        httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Invalid panel name (maximum 120 UTF-8 bytes, no control characters)");
        return;
      }
      if (!panel_identity->save(name)) {
        httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "Panel name could not be saved");
        return;
      }
    }
    const std::string response = esphome::json::build_json([](JsonObject root) {
      root["name"] = panel_identity->saved_name();
      root["friendly_name"] = panel_identity->target_name();
      root["hostname"] = panel_identity->target_hostname();
      root["mac_suffix"] = panel_identity->suffix();
      root["restart_required"] = panel_identity->restart_required();
      root["ip_address"] = esphome::network::get_ip_addresses()[0].str();
    });
    httpd_resp_set_type(raw, "application/json");
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    httpd_resp_send(raw, response.data(), response.size());
  }
 private:
  bool authorize(esphome::web_server_idf::AsyncWebServerRequest *request) {
    if (panel_identity == nullptr || !panel_identity->ready()) {
      httpd_req_t *raw = *request;
      httpd_resp_send_err(raw, HTTPD_503_SERVICE_UNAVAILABLE, "Panel identity is unavailable");
      return false;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(panel_identity->username(), panel_identity->password())) {
      request->requestAuthentication();
      return false;
    }
#endif
    return true;
  }
  std::array<uint8_t, 1024> body_{};
  size_t received_{0}, expected_{0};
  bool valid_{false};
};
inline void register_panel_identity_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new PanelIdentityHandler()); registered = true; }
}
}  // namespace espcontrol
#endif
