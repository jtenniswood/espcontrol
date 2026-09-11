#pragma once
#ifdef USE_ESP32
namespace esphome::web_server_idf { class AsyncWebServer; }
namespace espcontrol::card_asset_http {
void set_auth_credentials(const char *username, const char *password);
void register_endpoints(esphome::web_server_idf::AsyncWebServer &server);
}
#endif
