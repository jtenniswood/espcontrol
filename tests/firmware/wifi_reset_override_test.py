"""Run production WiFi override/reset boot code with modeled NVS and WiFi."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/espcontrol/device_reset.cpp").read_text()
methods = source[source.index("  bool wifi_override"):source.index("  bool read(Journal")]
verify = source[source.index("  bool verify(Mode"):source.index("\n} storage;")]
startup = source[source.index("void early_startup("):source.index("void register_handlers(")]
harness = r'''
#include "reset_policy.h"
#include <cassert>
#include <map>
#include <vector>
using namespace espcontrol::reset;
using nvs_handle_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NVS_NOT_FOUND = 1, ESP_FAIL = 2;
constexpr int NVS_READONLY = 0, NVS_READWRITE = 1;
const char *JOURNAL_NAMESPACE = "espcontrol_rst";
std::map<std::string, uint8_t> nvs;
bool nvs_unavailable = false, fail_commit = false;
int nvs_open(const char *, int mode, int *handle) {
  if (nvs_unavailable) return ESP_FAIL;
  if (nvs.empty() && mode == NVS_READONLY) return ESP_ERR_NVS_NOT_FOUND;
  *handle = 1; return ESP_OK;
}
int nvs_get_u8(int, const char *key, uint8_t *out) {
  if (!nvs.count(key)) return ESP_ERR_NVS_NOT_FOUND;
  *out = nvs[key]; return ESP_OK;
}
int nvs_set_u8(int, const char *key, uint8_t value) { nvs[key] = value; return ESP_OK; }
int nvs_commit(int) { return fail_commit ? ESP_FAIL : ESP_OK; }
void nvs_close(int) {}
Journal durable_journal;
std::map<uint32_t, std::string> credentials;
struct Entry {};
struct NvsStorage : Storage {
  uint32_t wifi_key = 88491487;
''' + methods + r'''
  bool read(Journal &out) override { out = durable_journal; return true; }
  bool write(const Journal &value) override { durable_journal = value; return true; }
  bool clear_panel() override { return true; }
  bool panel(bool) { return true; }
  bool entries(Mode, std::vector<Entry> &) { return true; }
  bool clear_preferences(Mode mode) override {
    for (auto it = credentials.begin(); it != credentials.end();) {
      if (mode == Mode::FACTORY || it->first != wifi_key) it = credentials.erase(it);
      else ++it;
    }
    return true;
  }
''' + verify + r'''
} storage;
namespace esphome {
struct Application {
  uint32_t get_config_version_hash() { return 123; }
  void feed_wdt() { assert(false && "unexpected retry"); }
} App;
void delay(int) {}
namespace ota {
struct Callback { template <typename T> void add_global_state_listener(T *) {} } callback;
Callback *get_global_ota_callback() { return &callback; }
}
namespace wifi {
struct WiFi { bool has_compiled_sta = true; void clear_sta() { has_compiled_sta = false; } } wifi;
WiFi *global_wifi_component = &wifi;
}
}
struct Value { template <typename T> void store(T) {} } current_epoch, initialized;
Journal journal;
const char *auth_username, *auth_password;
int ota_listener;
#define ESP_LOGE(...) ((void)0)
''' + startup + r'''
void boot() {
  storage = NvsStorage{};
  esphome::wifi::wifi.has_compiled_sta = true;
  early_startup(true, "", "");
  apply_wifi_override();
}
std::string selected_network() {
  auto key = wifi_preference_key(esphome::wifi::wifi.has_compiled_sta, 123);
  assert(key == storage.wifi_key);
  return credentials[key];
}
int main() {
  credentials[123] = "compiled-network";
  boot(); assert(selected_network() == "compiled-network");
  assert(request(storage, journal, Mode::FACTORY) == Result::ACCEPTED);
  boot(); assert(nvs["wifi_reset"] == 1);
  assert(!esphome::wifi::wifi.has_compiled_sta && !journal.pending());
  assert(storage.wifi_key == 88491487);
  assert(credentials.empty());
  // ESPHome saves against its fallback key after compiled stations are cleared.
  credentials[88491487] = "newly-provisioned-network";
  for (int reboot = 0; reboot < 3; ++reboot) { boot(); assert(selected_network() == "newly-provisioned-network"); }
  assert(request(storage, journal, Mode::CUSTOMIZATION) == Result::ACCEPTED);
  boot(); assert(selected_network() == "newly-provisioned-network");
  // A failed override commit must leave factory reset pending for retry.
  assert(request(storage, journal, Mode::FACTORY) == Result::ACCEPTED);
  fail_commit = true;
  assert(!resume(storage, journal)); assert(journal.pending());
  fail_commit = false;
  assert(resume(storage, journal)); assert(!journal.pending());
  boot(); assert(!esphome::wifi::wifi.has_compiled_sta);
  nvs_unavailable = true; assert(!storage.read_wifi_override());
  assert(!storage.save_wifi_override());
}
'''
with tempfile.TemporaryDirectory(prefix="espcontrol-wifi-reset-") as directory:
    path = Path(directory) / "test.cpp"
    path.write_text(harness)
    executable = Path(directory) / "test"
    subprocess.run(["g++", "-std=c++17", "-I", str(ROOT / "components/espcontrol"), str(path), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("WiFi reset: production boot/override code passed factory reset, reprovisioning, three reboots, partial reset and storage failure checks (modeled NVS/WiFi).")
