#pragma once
#include <cstdint>
#include <map>
#include <string>
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_TIMEOUT = 1, ESP_ERR_NO_MEM = 2;
constexpr int HTTP_METHOD_GET = 0, HTTP_AUTH_TYPE_NONE = 0, HTTP_EVENT_ON_HEADER = 1;
struct esp_http_client_event_t { void *user_data; int event_id; const char *header_key; const char *header_value; };
struct esp_http_client_config_t {
 const char *url{};
 int method{}, timeout_ms{}, buffer_size{};
 bool disable_auto_redirect{}, skip_cert_common_name_check{};
 int auth_type{};
 void *user_data{};
 int (*crt_bundle_attach)(void*){};
 esp_err_t (*event_handler)(esp_http_client_event_t*){};
};
using esp_http_client_handle_t = esp_http_client_config_t*;
inline esp_http_client_config_t probe_test_config;
inline std::string probe_test_url, probe_test_content_type = "application/manifest+json";
inline std::map<std::string, std::string> probe_test_headers;
inline int probe_test_status = 200, probe_test_error = 0, probe_test_cleanups = 0;
inline int probe_test_client_allocations = 0;
inline bool probe_test_client_allocation_fails = false;
inline esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config) {
 ++probe_test_client_allocations; probe_test_config = *config; probe_test_url = config->url; probe_test_headers.clear();
 return probe_test_client_allocation_fails ? nullptr : &probe_test_config;
}
inline void esp_http_client_set_header(esp_http_client_handle_t, const char *key, const char *value) {
 probe_test_headers[key] = value;
}
inline int esp_http_client_open(esp_http_client_handle_t, int) { return probe_test_error; }
inline void esp_http_client_set_timeout_ms(esp_http_client_handle_t, unsigned) {}
inline int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client) {
 esp_http_client_event_t event{client->user_data, HTTP_EVENT_ON_HEADER, "Content-Type", probe_test_content_type.c_str()};
 return client->event_handler(&event) == ESP_OK ? 100 : -1;
}
inline int esp_http_client_get_status_code(esp_http_client_handle_t) { return probe_test_status; }
inline void esp_http_client_close(esp_http_client_handle_t) {}
inline void esp_http_client_cleanup(esp_http_client_handle_t) { ++probe_test_cleanups; }
