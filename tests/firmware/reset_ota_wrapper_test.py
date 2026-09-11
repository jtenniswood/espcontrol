"""Execute the production OTA wrappers against overlapping/failing transports."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/espcontrol/device_reset.cpp").read_text()
start = source.index('extern "C" esp_err_t __real_esp_ota_begin(')
end = source.index('#ifdef USE_ESP32_HOSTED', start)
wrappers = source[start:end]
harness = r'''
#include <cassert>
#include "reset_interlock.h"
using esp_err_t = int;
using esp_ota_handle_t = uint32_t;
struct esp_partition_t {};
constexpr int ESP_OK = 0, ESP_ERR_INVALID_STATE = 1, ESP_ERR_NOT_FOUND = 2, ESP_ERR_INVALID_ARG = 3;
namespace espcontrol::reset { OperationInterlock interlock; }
using namespace espcontrol::reset;
int begin_result = ESP_OK, begin_calls = 0, end_result = ESP_OK, abort_result = ESP_OK;
uint32_t next_handle = 42;
extern "C" int __real_esp_ota_begin(const esp_partition_t *, size_t, esp_ota_handle_t *handle) {
  ++begin_calls;
  assert(interlock.busy());
  if (begin_result == ESP_OK) *handle = next_handle++;
  return begin_result;
}
extern "C" int __real_esp_ota_end(esp_ota_handle_t) {
  assert(interlock.busy()); // Keep reservation throughout the actual end call.
  return end_result;
}
extern "C" int __real_esp_ota_abort(esp_ota_handle_t) {
  assert(interlock.busy());
  return abort_result;
}
struct TestStorage : Storage {
  bool read(Journal &) override { return true; }
  bool write(const Journal &) override { return true; }
  bool clear_panel() override { return true; }
  bool clear_preferences(Mode) override { return true; }
  bool verify(Mode) override { return true; }
};
''' + wrappers + r'''
int main() {
  TestStorage storage; Journal journal;
  int native = 0, web = 0;
  esp_ota_handle_t first = 0, rejected = 0;
  interlock.set_ota_source_busy(&native, true);
  assert(__wrap_esp_ota_begin(nullptr, 0, &first) == ESP_OK);
  interlock.set_ota_source_busy(&web, true);
  assert(__wrap_esp_ota_begin(nullptr, 0, &rejected) == ESP_ERR_INVALID_STATE);
  assert(begin_calls == 1);
  abort_result = ESP_ERR_NOT_FOUND;
  assert(__wrap_esp_ota_abort(rejected) == ESP_ERR_NOT_FOUND);
  interlock.set_ota_source_busy(&web, false);
  assert(interlock.record(storage, journal, Mode::FACTORY) == Result::CONFLICT);
  end_result = ESP_ERR_INVALID_ARG; // IDF consumes even a validation-failed handle.
  assert(__wrap_esp_ota_end(first) == ESP_ERR_INVALID_ARG);
  assert(interlock.busy()); // The transport is still finishing.
  interlock.set_ota_source_busy(&native, false);
  assert(!interlock.busy());

  assert(__wrap_esp_ota_begin(nullptr, 0, &first) == ESP_OK);
  assert(__wrap_esp_ota_abort(first - 1) == ESP_ERR_NOT_FOUND);
  assert(interlock.record(storage, journal, Mode::FACTORY) == Result::CONFLICT);
  abort_result = ESP_OK;
  assert(__wrap_esp_ota_abort(first) == ESP_OK);
  assert(!interlock.busy());
  begin_result = ESP_ERR_INVALID_ARG;
  assert(__wrap_esp_ota_begin(nullptr, 0, &first) == ESP_ERR_INVALID_ARG);
  assert(!interlock.busy());
  assert(interlock.record(storage, journal, Mode::FACTORY) == Result::ACCEPTED);
  const int calls_before_reset = begin_calls;
  assert(__wrap_esp_ota_begin(nullptr, 0, &first) == ESP_ERR_INVALID_STATE);
  assert(begin_calls == calls_before_reset);
}
'''
with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    (directory / "test.cpp").write_text(harness)
    executable = directory / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pthread",
        "-I", str(ROOT / "components/espcontrol"), str(directory / "test.cpp"),
        "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("OTA wrapper overlap and lifecycle checks passed.")
