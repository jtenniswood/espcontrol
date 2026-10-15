#!/usr/bin/env python3
"""Exercise the production preference store with a bounded in-memory preference adapter."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "components/espcontrol/clock_bar_configuration.h").read_text()
store = source[source.index("class Configuration {"):source.index("inline Configuration &configuration()")]
program = r'''
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
constexpr size_t MAX_BYTES = 4096;
struct Element { std::string type; };
bool parse(const std::string &value, std::vector<Element> &items) {
  if (value == "bad") return false;
  if (value == "temperature") items.push_back({"temperature"});
  return true;
}
using nvs_handle_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NVS_NOT_FOUND = 1, ESP_ERR_NVS_INVALID_LENGTH = 2, ESP_ERR_NVS_TYPE_MISMATCH = 3, NVS_READWRITE = 1;
std::vector<unsigned char> saved, pending;
bool fail_save = false, fail_sync = false;
int nvs_open(const char *, int, nvs_handle_t *handle) { *handle = 1; return ESP_OK; }
int nvs_get_blob(nvs_handle_t, const char *, void *record, size_t *size) {
  if (saved.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if (saved.size() > *size) return ESP_ERR_NVS_INVALID_LENGTH;
  std::memcpy(record, saved.data(), saved.size()); *size = saved.size(); return ESP_OK;
}
int nvs_set_blob(nvs_handle_t, const char *, const void *record, size_t size) {
  if (fail_save) return 4;
  pending.assign(static_cast<const unsigned char *>(record), static_cast<const unsigned char *>(record) + size); return ESP_OK;
}
int nvs_commit(nvs_handle_t) { if (fail_sync) return 4; saved = pending; return ESP_OK; }
void nvs_close(nvs_handle_t) { pending.clear(); }
'''
program += store + r'''
int main() {
  Configuration original;
  assert(!original.ready());
  original.load();
  assert(original.ready() && original.value().empty());
  assert(original.save("temperature"));
  assert(original.contains("temperature"));
  assert(original.take_changed() && !original.take_changed());
  Configuration rebooted; rebooted.load();
  assert(rebooted.value() == "temperature" && rebooted.contains("temperature"));
  assert(!rebooted.save(std::string(MAX_BYTES + 1, 'a')));
  assert(!rebooted.save("bad"));
  assert(rebooted.value() == "temperature");
  fail_save = true;
  assert(!rebooted.save("other"));
  assert(rebooted.value() == "temperature");
  fail_save = false;
  fail_sync = true;
  assert(!rebooted.save("other"));
  assert(rebooted.value() == "temperature");
  Configuration failed_commit; failed_commit.load(); assert(failed_commit.value() == "temperature");
  fail_sync = false;
  assert(rebooted.save("{\"version\":1,\"items\":[]}"));
  Configuration empty; empty.load();
  assert(empty.value() == "{\"version\":1,\"items\":[]}");
  assert(!empty.contains("temperature"));
  assert(empty.save(""));
  Configuration legacy; legacy.load(); assert(legacy.value().empty());
  saved.assign(4, 0);
  Configuration corrupt; corrupt.load(); assert(corrupt.ready() && corrupt.value().empty());
}
'''
with tempfile.TemporaryDirectory(prefix="espcontrol-clock-bar-") as directory:
    src = Path(directory) / "test.cpp"
    exe = Path(directory) / "test"
    src.write_text(program)
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print("Clock bar persistence, reboot, empty layout and failed-save checks passed.")
