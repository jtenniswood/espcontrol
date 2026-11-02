#pragma once
#ifdef USE_WEBSERVER
#include <array>
#include <cstring>
#include "esphome/components/json/json_util.h"
#include "esphome/components/network/util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"
#include "gpio_relays.h"

namespace espcontrol {
class GpioRelaysHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_POST) &&
           request->url_to(path) == "/api/v1/gpio-relays";
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
      GpioRelaySettings settings;
      const bool parsed = valid_ && received_ == expected_ &&
          esphome::json::parse_json(body_.data(), received_, [&](JsonObject root) {
            if (!root["enabled"].is<bool>() || !root["controls"].is<JsonArray>()) return false;
            settings.enabled = root["enabled"].as<bool>();
            JsonArray controls = root["controls"].as<JsonArray>();
            if (controls.size() != settings.controls.size()) return false;
            size_t i = 0;
            for (JsonVariant value : controls) {
              if (!value.is<JsonObject>()) return false;
              JsonObject control = value.as<JsonObject>();
              if (!control["pin"].is<int>() || !control["inverted"].is<bool>() ||
                  !control["name"].is<const char *>()) return false;
              const int pin = control["pin"].as<int>();
              if (pin < -1 || pin > 3) return false;
              auto &target = settings.controls[i++];
              target.pin = pin;
              target.inverted = control["inverted"].as<bool>();
              const JsonString name = control["name"].as<JsonString>();
              if (name.size() > 32 || std::memchr(name.c_str(), 0, name.size()) != nullptr) return false;
              std::memcpy(target.name, name.c_str(), name.size());
            }
            return GpioRelays::valid(settings);
          });
      valid_ = false;
      if (!parsed) {
        httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Choose unique supported pins for GPIO controls");
        return;
      }
      if (!gpio_relays().save(settings)) {
        httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "GPIO settings could not be saved");
        return;
      }
    }
    const auto settings = gpio_relays().settings();
    const std::string response = esphome::json::build_json([&](JsonObject root) {
      root["enabled"] = static_cast<bool>(settings.enabled);
      JsonArray pins = root["allowed_pins"].to<JsonArray>();
      for (int pin : {1, 2, 3}) pins.add(pin);
      JsonArray controls = root["controls"].to<JsonArray>();
      for (const auto &control : settings.controls) {
        JsonObject item = controls.add<JsonObject>();
        item["pin"] = control.pin;
        item["name"] = control.name;
        item["inverted"] = static_cast<bool>(control.inverted);
      }
    });
    httpd_resp_set_type(raw, "application/json");
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    httpd_resp_send(raw, response.data(), response.size());
  }
 private:
  bool authorize(esphome::web_server_idf::AsyncWebServerRequest *request) {
    if (!gpio_relays().supported()) {
      httpd_resp_send_err(*request, HTTPD_404_NOT_FOUND, "GPIO relays are unsupported on this panel");
      return false;
    }
    if (panel_identity == nullptr || !gpio_relays().ready()) {
      httpd_req_t *raw = *request;
      httpd_resp_set_status(raw, "503 Service Unavailable");
      httpd_resp_send(raw, "GPIO settings are unavailable", HTTPD_RESP_USE_STRLEN);
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
  std::array<uint8_t, 2048> body_{};
  size_t received_{0}, expected_{0};
  bool valid_{false};
};
inline void register_gpio_relays_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new GpioRelaysHandler()); registered = true; }
}
}  // namespace espcontrol
#endif
