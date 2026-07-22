#pragma once

#ifdef USE_ESP32

#include <esp_http_server.h>

namespace esphome::web_server_idf {
class AsyncWebServerRequest;
}

namespace espcontrol::card_asset_http {

bool is_shortcut_request(esphome::web_server_idf::AsyncWebServerRequest *request);
bool handle_request(esphome::web_server_idf::AsyncWebServerRequest *request);
esp_err_t handle_post(httpd_req_t *request);

}  // namespace espcontrol::card_asset_http

#endif
