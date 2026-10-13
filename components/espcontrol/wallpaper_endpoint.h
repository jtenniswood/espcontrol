#pragma once
#ifdef USE_WEBSERVER
#include <cstring>
#include <cstdlib>
#include <esp_heap_caps.h>
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"
#include "wallpaper.h"

namespace espcontrol {
class WallpaperHandler final : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char path[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return (request->method() == HTTP_GET || request->method() == HTTP_PUT) &&
           request->url_to(path) == "/api/v1/wallpaper";
  }
  size_t maximumBodySize() const override { return WALLPAPER_MAX_BYTES; }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    reset();
    return authorize(request);
  }
  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data,
                  size_t len, size_t index, size_t total) override {
    if (index == 0 && total >= WALLPAPER_HEADER_BYTES && total <= WALLPAPER_MAX_BYTES) {
      expected_ = total;
      body_ = static_cast<uint8_t *>(heap_caps_malloc(total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    }
    if (!body_ || !data || index != received_ || len > expected_ - received_) { reset(); return; }
    std::memcpy(body_ + received_, data, len);
    received_ += len;
  }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!authorize(request)) { reset(); return; }
    httpd_req_t *raw = *request;
    httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
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
  uint8_t *body_{nullptr};
  size_t received_{0}, expected_{0};
};
inline void register_wallpaper_endpoint(esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) { server.addHandler(new WallpaperHandler()); registered = true; }
}
}  // namespace espcontrol
#endif
