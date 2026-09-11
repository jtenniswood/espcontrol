#include "device_reset.h"
#include "esphome/core/defines.h"
#ifdef USE_ESP32
#include <algorithm>
#include <array>
#include <mutex>
#include <vector>
#include <esp_partition.h>
#include <nvs.h>
#include "esphome/core/application.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "esphome/components/ota/ota_backend.h"
#ifdef USE_UPDATE
#include "esphome/components/update/update_entity.h"
#endif

namespace espcontrol::reset {
namespace {
constexpr const char *TAG = "espcontrol.reset";
constexpr const char *JOURNAL_NAMESPACE = "espcontrol_rst";
std::atomic<uint32_t> current_epoch{0};
std::atomic<bool> reset_pending{false}, initialized{false}, installing{false}, entity_installing{false};
#ifdef USE_UPDATE
std::vector<esphome::update::UpdateEntity *> update_entities;
#endif
std::mutex reset_mutex;
Journal journal;
const char *auth_username = "";
const char *auth_password = "";
struct Entry { std::string ns; std::string key; };
class NvsStorage final : public Storage {
 public:
  uint32_t wifi_key{88491487UL};
  bool read(Journal &out) override {
    nvs_handle_t handle;
    const auto opened = nvs_open(JOURNAL_NAMESPACE, NVS_READONLY, &handle);
    if (opened == ESP_ERR_NVS_NOT_FOUND) { out = Journal{}; return true; }
    if (opened != ESP_OK) return false;
    size_t size = sizeof(out);
    auto err = nvs_get_blob(handle, "journal", &out, &size);
    nvs_close(handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) { out = Journal{}; return true; }
    return err == ESP_OK && size == sizeof(out) && out.valid();
  }
  bool write(const Journal &value) override {
    nvs_handle_t handle;
    if (nvs_open(JOURNAL_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    bool ok = nvs_set_blob(handle, "journal", &value, sizeof(value)) == ESP_OK && nvs_commit(handle) == ESP_OK;
    nvs_close(handle);
    Journal loaded;
    return ok && read(loaded) && std::memcmp(&value, &loaded, sizeof(value)) == 0;
  }
  bool panel(bool erase) {
    auto *part = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "card_images");
    if (part == nullptr) return true;  // Older OTA partition tables use NVS.
    std::array<uint8_t, 1024> bytes;
    for (size_t offset = 0; offset < part->size; offset += 4096) {
      esphome::App.feed_wdt();
      if (erase && esp_partition_erase_range(part, offset, 4096) != ESP_OK) return false;
      for (size_t chunk = 0; chunk < 4096; chunk += bytes.size()) {
        if (esp_partition_read(part, offset + chunk, bytes.data(), bytes.size()) != ESP_OK ||
            !std::all_of(bytes.begin(), bytes.end(), [](uint8_t b) { return b == 0xff; })) return false;
      }
    }
    return true;
  }
  bool clear_panel() override { return panel(true); }
  bool entries(Mode mode, std::vector<Entry> &remove) {
    nvs_iterator_t it = nullptr;
    auto err = nvs_entry_find("nvs", nullptr, NVS_TYPE_ANY, &it);
    while (err == ESP_OK) {
      nvs_entry_info_t info;
      if (nvs_entry_info(it, &info) != ESP_OK) { nvs_release_iterator(it); return false; }
      if (!preserve_key(mode, info.namespace_name, info.key, wifi_key)) remove.push_back({info.namespace_name, info.key});
      err = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    return err == ESP_ERR_NVS_NOT_FOUND;
  }
  bool clear_preferences(Mode mode) override {
    std::vector<Entry> remove;
    if (!entries(mode, remove)) return false;
    for (const auto &entry : remove) {
      esphome::App.feed_wdt();
      nvs_handle_t handle;
      if (nvs_open(entry.ns.c_str(), NVS_READWRITE, &handle) != ESP_OK) return false;
      auto err = nvs_erase_key(handle, entry.key.c_str());
      bool ok = (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) && nvs_commit(handle) == ESP_OK;
      nvs_close(handle);
      if (!ok) return false;
    }
    return true;
  }
  bool verify(Mode mode) override {
    std::vector<Entry> remaining;
    return panel(false) && entries(mode, remaining) && remaining.empty();
  }
} storage;
class OtaListener : public esphome::ota::OTAGlobalStateListener {
 public:
  void on_ota_global_state(esphome::ota::OTAState state, float, uint8_t, esphome::ota::OTAComponent *) override {
    set_update_busy(state != esphome::ota::OTA_ERROR && state != esphome::ota::OTA_ABORT);
  }
} ota_listener;
void respond(httpd_req_t *raw, const char *status, const char *body) {
  httpd_resp_set_status(raw, status);
  httpd_resp_set_type(raw, "application/json");
  httpd_resp_set_hdr(raw, "Cache-Control", "no-store");
  httpd_resp_send(raw, body, HTTPD_RESP_USE_STRLEN);
}
class ResetHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *r) const override {
    char url[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return r->url_to(url) == "/api/v1/reset";
  }
  size_t maximumBodySize() const override { return 128; }
  bool authenticate(esphome::web_server_idf::AsyncWebServerRequest *r) {
#ifdef USE_WEBSERVER_AUTH
    if (auth_username[0] && !r->authenticate(auth_username, auth_password)) { r->requestAuthentication(); return false; }
#else
    (void) r;
#endif
    return true;
  }
  bool canReceiveBody(esphome::web_server_idf::AsyncWebServerRequest *r) override {
    received_ = 0;
    return authenticate(r);
  }
  void handleBody(esphome::web_server_idf::AsyncWebServerRequest *, uint8_t *data, size_t len, size_t index, size_t) override {
    if (index != received_ || index + len >= sizeof(body_)) { received_ = 0; return; }
    std::memcpy(body_ + index, data, len);
    received_ += len;
    body_[received_] = 0;
  }
  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *r) override {
    httpd_req_t *raw = *r;
    if (!authenticate(r)) return;
    if (r->method() == HTTP_GET) {
      char response[200];
      std::snprintf(response, sizeof(response), "{\"modes\":[\"customization\",\"factory\"],\"epoch\":%lu,\"pending\":%s}",
                    static_cast<unsigned long>(epoch()), pending() ? "true" : "false");
      respond(raw, "200 OK", response);
      return;
    }
    if (r->method() != HTTP_POST) { respond(raw, "405 Method Not Allowed", "{\"error\":\"POST required\"}"); return; }
    if (!same_origin(r->get_header("Origin").value_or(""), r->get_header("Host").value_or(""),
                     r->get_header("Sec-Fetch-Site").value_or(""), r->get_header("X-EspControl-Request").value_or(""))) {
      respond(raw, "403 Forbidden", "{\"error\":\"Same-origin reset request required\"}"); return;
    }
    if (r->get_header("Content-Type").value_or("") != "application/json") {
      respond(raw, "415 Unsupported Media Type", "{\"error\":\"JSON required\"}"); return;
    }
    Mode mode = Mode::NONE;
    bool valid = received_ == raw->content_len && received_ > 0 && esphome::json::parse_json(body_, [&](JsonObject root) {
      if (root.size() != 1 || !root["mode"].is<const char *>()) return false;
      mode = parse_mode(root["mode"].as<std::string>());
      return mode != Mode::NONE;
    });
    received_ = 0;
    if (!valid) { respond(raw, "400 Bad Request", "{\"error\":\"Invalid reset mode\"}"); return; }
    std::lock_guard<std::mutex> lock(reset_mutex);
    const auto supplied = r->get_header("X-EspControl-Epoch").value_or("");
    if (supplied != std::to_string(epoch()) &&
        !(journal.pending() && journal.mode == mode && supplied == std::to_string(epoch() - 1))) {
      respond(raw, "409 Conflict", "{\"error\":\"Reload before resetting\"}"); return;
    }
    if (update_busy()) { respond(raw, "409 Conflict", "{\"error\":\"Firmware installation in progress\"}"); return; }
    const bool was_pending = journal.pending();
    auto result = request(storage, journal, mode);
    if (result != Result::ACCEPTED) {
      // A failed commit/readback may still have published the journal. Fail
      // closed until a reboot settles it rather than accepting more writes.
      if (result == Result::FAILED) reset_pending.store(true);
      respond(raw, result == Result::CONFLICT ? "409 Conflict" : "500 Internal Server Error", "{\"error\":\"Reset could not be recorded\"}"); return;
    }
    current_epoch.store(journal.epoch);
    reset_pending.store(true);
    respond(raw, "202 Accepted", "{\"status\":\"restarting\"}");
    if (!was_pending) {
      // Schedule only after the response has been sent, outside the HTTP task.
      esphome::App.scheduler.set_timeout(nullptr, "espcontrol_reset", 1000, []() { esphome::App.safe_reboot(); });
    }
  }
 private:
  char body_[129]{};
  size_t received_{0};
};
}  // namespace
uint32_t epoch() { return current_epoch.load(); }
bool pending() { return reset_pending.load(); }
bool ready() { return initialized.load(); }
void set_update_busy(bool busy) { installing.store(busy); }
bool update_busy() { return installing.load() || entity_installing.load(); }
void watch_update(esphome::update::UpdateEntity *entity) {
#ifdef USE_UPDATE
  update_entities.push_back(entity);
  entity->add_on_state_callback([]() {
    entity_installing.store(std::any_of(update_entities.begin(), update_entities.end(), [](auto *item) {
      return item->state == esphome::update::UPDATE_STATE_INSTALLING;
    }));
  });
#endif
}
void early_startup(bool compiled_networks, const char *username, const char *password) {
  auth_username = username;
  auth_password = password;
  storage.wifi_key = wifi_preference_key(compiled_networks, esphome::App.get_config_version_hash());
  for (;;) {
    if (storage.read(journal) && resume(storage, journal)) break;
    // No components or networking have restored stale settings yet. Keep that
    // invariant on failure; serial recovery remains available, with automatic
    // retries instead of booting a partially reset panel or reboot-looping.
    ESP_LOGE(TAG, "Reset recovery: storage unavailable; retrying in 10 seconds. Settings restoration blocked.");
    for (int i = 0; i < 100; ++i) { esphome::App.feed_wdt(); esphome::delay(100); }
  }
  current_epoch.store(journal.epoch);
  initialized.store(true);
  esphome::ota::get_global_ota_callback()->add_global_state_listener(&ota_listener);
}
void register_handlers(esphome::web_server_idf::AsyncWebServer &server) { server.addHandler(new ResetHandler()); }
}  // namespace espcontrol::reset

// Shared dispatcher hook covers native config and legacy entity POSTs,
// including calls from a stale browser that has no epoch header. ESPHome's
// provisioning/upload forms remain usable when no reset is pending.
extern "C" bool espcontrol_allow_web_write(httpd_req_t *raw) {
  using namespace espcontrol::reset;
  if (std::strncmp(raw->uri, "/api/v1/reset", 13) == 0 && (raw->uri[13] == 0 || raw->uri[13] == '?')) return true;
  const size_t size = httpd_req_get_hdr_value_len(raw, "X-EspControl-Epoch");
  char supplied[16]{};
  bool valid = size > 0 && size < sizeof(supplied) &&
               httpd_req_get_hdr_value_str(raw, "X-EspControl-Epoch", supplied, sizeof(supplied)) == ESP_OK &&
               std::to_string(epoch()) == supplied;
  if (ready() && !pending() && (!write_requires_epoch(raw->uri) || valid)) return true;
  httpd_resp_set_status(raw, pending() ? "409 Conflict" : "428 Precondition Required");
  httpd_resp_set_type(raw, "application/json");
  httpd_resp_send(raw, "{\"error\":\"Reload the page before changing settings\"}", HTTPD_RESP_USE_STRLEN);
  return false;
}
#endif
