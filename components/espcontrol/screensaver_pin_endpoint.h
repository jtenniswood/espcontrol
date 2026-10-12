#pragma once
#ifdef USE_WEBSERVER
#include <array>
#include <cstring>
#include "esphome/components/json/json_util.h"
#include "esphome/components/network/util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"
#include "screensaver_pin.h"

namespace espcontrol {
class ScreensaverPinHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_POST) &&
           request->url_to(path) == "/api/v1/screensaver-pin";
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
      std::string pin;
      bool enabled = false, has_pin_value = false;
      bool randomize_keypad = false, has_randomize_value = false;
      const bool parsed = valid_ && received_ == expected_ &&
          esphome::json::parse_json(body_.data(), received_, [&](JsonObject root) {
            if (!root["enabled"].is<bool>()) return false;
            enabled = root["enabled"].as<bool>();
            has_randomize_value = !root["randomize_keypad"].isUnbound();
            if (has_randomize_value) {
              if (!root["randomize_keypad"].is<bool>()) return false;
              randomize_keypad = root["randomize_keypad"].as<bool>();
            }
            has_pin_value = !root["pin"].isUnbound();
            if (!has_pin_value) return true;  // Toggle without changing the saved PIN.
            if (!root["pin"].is<const char *>()) return false;
            const JsonString value = root["pin"].as<JsonString>();
            pin.assign(value.c_str(), value.size());
            return ScreensaverPin::valid_pin(pin);
          });
      valid_ = false;
      if (!parsed) {
        httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Invalid PIN settings");
        return;
      }
      if (!screensaver_pin().save(enabled, has_pin_value ? &pin : nullptr,
                                  has_randomize_value ? &randomize_keypad : nullptr)) {
        httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "PIN settings could not be saved");
        return;
      }
    }
    const std::string response = esphome::json::build_json([](JsonObject root) {
      root["enabled"] = screensaver_pin().enabled();
      root["has_pin"] = screensaver_pin().has_pin();
      root["pin_length"] = screensaver_pin().pin_length();
      root["randomize_keypad"] = screensaver_pin().randomize_keypad();
    });
    httpd_resp_set_type(raw, "application/json");
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    httpd_resp_send(raw, response.data(), response.size());
  }
 private:
  bool authorize(esphome::web_server_idf::AsyncWebServerRequest *request) {
    if (panel_identity == nullptr || !screensaver_pin().ready()) {
      httpd_req_t *raw = *request;
      httpd_resp_set_status(raw, "503 Service Unavailable");
      httpd_resp_send(raw, "PIN settings are unavailable", HTTPD_RESP_USE_STRLEN);
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
inline void register_screensaver_pin_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new ScreensaverPinHandler()); registered = true; }
}
}  // namespace espcontrol
#endif
