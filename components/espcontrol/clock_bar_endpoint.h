#pragma once
#ifdef USE_WEBSERVER
#include <array>
#include <cstring>
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "editable_clock_bar.h"
#include "panel_identity.h"

namespace espcontrol::clockbar {
class Endpoint final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_POST) && request->url_to(path) == "/api/v1/clock-bar";
  }
  size_t maximumBodySize() const override { return body_.size(); }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    received_ = 0; expected_ = 0; valid_ = false;
    return authorize(request);
  }
  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data, size_t len, size_t index, size_t total) override {
    if (index == 0) { received_ = 0; expected_ = total; valid_ = total > 0 && total <= body_.size(); }
    if (!valid_ || !data || index != received_ || len > body_.size() - received_) { valid_ = false; return; }
    std::memcpy(body_.data() + received_, data, len); received_ += len;
  }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!authorize(request)) return;
    httpd_req_t *raw = *request;
    auto &config = configuration();
    if (request->method() == HTTP_POST) {
      const auto content_type = request->get_header("Content-Type");
      if (!content_type.has_value() || *content_type != "application/json" || !valid_ || received_ != expected_) {
        valid_ = false; httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Expected clock bar JSON"); return;
      }
      valid_ = false;
      std::string value(reinterpret_cast<const char *>(body_.data()), received_);
      bool legacy = false;
      esphome::json::parse_json(value, [&](JsonObject root) { legacy = root.size() == 1 && root["legacy"].is<bool>() && root["legacy"].as<bool>(); return true; });
      std::vector<Element> items;
      if (legacy) value.clear();
      else if (!parse(value, items)) { httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Invalid clock bar layout"); return; }
      if (!config.save(value)) { httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "Clock bar could not be saved"); return; }
    }
    const std::string saved = config.value();
    const std::string value = saved.empty() ? "{\"legacy\":true}" : saved;
    httpd_resp_set_type(raw, "application/json"); httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    httpd_resp_send(raw, value.data(), value.size());
  }
 private:
  bool authorize(esphome::web_server_idf::AsyncWebServerRequest *request) {
    if (!panel_identity || !configuration().ready()) {
      httpd_req_t *raw = *request; httpd_resp_set_status(raw, "503 Service Unavailable");
      httpd_resp_send(raw, "Clock bar is starting", HTTPD_RESP_USE_STRLEN); return false;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(panel_identity->username(), panel_identity->password())) { request->requestAuthentication(); return false; }
#endif
    return true;
  }
  std::array<uint8_t, MAX_BYTES> body_{};
  size_t received_ = 0, expected_ = 0;
  bool valid_ = false;
};
inline void register_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new Endpoint()); registered = true; }
}
}  // namespace espcontrol::clockbar
#endif
