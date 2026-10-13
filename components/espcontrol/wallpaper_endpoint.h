#pragma once
#ifdef USE_WEBSERVER
#include <cstdio>
#include <cstring>
#include "esphome/components/json/json_util.h"
#include <cstdlib>
#include <esp_heap_caps.h>
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"
#include "wallpaper.h"
#include "wallpaper_decode.h"

namespace espcontrol {
class WallpaperHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_PUT) &&
           (request->url_to(path) == "/api/v1/wallpaper" || request->url_to(path) == "/api/v1/wallpaper-opacity");
  }
  size_t maximumBodySize() const override { return opacity_request_ ? 64 : WALLPAPER_MAX_BYTES; }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    reset();
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    opacity_request_ = request->url_to(path) == "/api/v1/wallpaper-opacity";
    return authorize(request);
  }
  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data,
                  size_t len, size_t index, size_t total) override {
    if (index == 0 && total >= (opacity_request_ ? 1 : WALLPAPER_HEADER_BYTES) && total <= maximumBodySize()) {
      expected_ = total;
      body_ = static_cast<uint8_t *>(heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (!body_ || !data || index != received_ || len > expected_ - received_) { reset(); return; }
    std::memcpy(body_ + received_, data, len);
    received_ += len;
  }
  void handleBodyEnd(esphome::web_server_idf::AsyncWebServerRequest *) override { reset(); }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!authorize(request)) { reset(); return; }
    httpd_req_t *raw = *request;
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    if (request->url_to(path) == "/api/v1/wallpaper-opacity") {
      if (request->method() == HTTP_PUT) {
        int opacity = -1;
        const auto type = request->get_header("Content-Type");
        const bool valid = type.has_value() && *type == "application/json" && body_ && received_ == expected_ &&
            esphome::json::parse_json(body_, received_, [&](JsonObject root) {
              if (!root["opacity"].is<int>()) return false;
              opacity = root["opacity"].as<int>();
              return opacity >= 0 && opacity <= 100;
            });
        reset();
        if (!valid) { httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Invalid card opacity"); return; }
        if (!wallpaper_store().save_opacity_percent(opacity)) {
          httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "Card opacity could not be saved"); return;
        }
      }
      char response[32];
      const int size = std::snprintf(response, sizeof(response), "{\"opacity\":%u}",
                                    static_cast<unsigned>(wallpaper_store().opacity_percent()));
      httpd_resp_set_type(raw, "application/json");
      httpd_resp_send(raw, response, size);
      return;
    }
    if (request->method() == HTTP_GET) {
      size_t size; uint32_t revision;
      uint8_t *data = wallpaper_store().load(size, revision);
      if (size && !data) {
        httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "Wallpaper could not be read"); return;
      }
      httpd_resp_set_type(raw, "application/vnd.espcontrol.wallpaper");
      if (!data) httpd_resp_set_status(raw, "204 No Content");
      httpd_resp_send(raw, reinterpret_cast<const char *>(data), size);
      free(data);
      return;
    }
    if (request->method() == HTTP_PUT) {
      const auto type = request->get_header("Content-Type");
      if (!type.has_value() || *type != "application/vnd.espcontrol.wallpaper" ||
          received_ != expected_ || (!wallpaper_valid(body_, received_) && !wallpaper_is_removal(body_, received_))) {
        reset(); httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Invalid wallpaper"); return;
      }
    }
    if (body_ && wallpaper_u32(body_) == WALLPAPER_JPEG_MAGIC) {
      DecodedWallpaper decoded;
      if (!wallpaper_decode(body_, received_, decoded)) {
        reset(); httpd_resp_send_err(raw, HTTPD_400_BAD_REQUEST, "Wallpaper could not be decoded"); return;
      }
      free(decoded.pixels);
    }
    const bool saved = wallpaper_store().save(body_, wallpaper_is_removal(body_, received_) ? 0 : received_);
    reset();
    if (!saved) { httpd_resp_send_err(raw, HTTPD_500_INTERNAL_SERVER_ERROR, "Wallpaper could not be saved"); return; }
    httpd_resp_set_status(raw, "204 No Content");
    httpd_resp_send(raw, nullptr, 0);
  }
 private:
  bool authorize(esphome::web_server_idf::AsyncWebServerRequest *request) {
#ifdef USE_WEBSERVER_AUTH
    if (!panel_identity || !request->authenticate(panel_identity->username(), panel_identity->password())) {
      request->requestAuthentication(); return false;
    }
#endif
    if (!wallpaper_store().available()) {
      httpd_req_t *raw = *request;
      httpd_resp_set_status(raw, "501 Not Implemented");
      httpd_resp_send(raw, "Wallpaper storage is unavailable on this panel", HTTPD_RESP_USE_STRLEN);
      return false;
    }
    return true;
  }
  void reset() { free(body_); body_ = nullptr; received_ = expected_ = 0; }
  bool opacity_request_{false};
  uint8_t *body_{nullptr};
  size_t received_{0}, expected_{0};
};
inline void register_wallpaper_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new WallpaperHandler()); registered = true; }
}
}  // namespace espcontrol
#endif
