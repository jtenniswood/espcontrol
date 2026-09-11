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
hosted_wrappers = source[end + len("#ifdef USE_ESP32_HOSTED"):source.index("#endif", end)]
hook_start = hosted_wrappers.index('extern "C" void espcontrol_hosted_ota_failed()')
hook_end = hosted_wrappers.index("\n}", hook_start) + 2
hosted_hook = hosted_wrappers[hook_start:hook_end]
hosted_wrappers = hosted_wrappers[:hook_start] + hosted_wrappers[hook_end:]
recovery = (ROOT / "components/c6_recovery/c6_recovery.cpp").read_text()
recovery = recovery[recovery.index("bool C6RecoveryComponent::install_firmware_()"):
                    recovery.index("}  // namespace esphome::c6_recovery")]
hosted_harness = r'''
#include <cassert>
#include <algorithm>
#include <cstring>
#include "reset_interlock.h"
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_ERR_INVALID_STATE = 1, ESP_ERR_NOT_FOUND = 2, ESP_ERR_INVALID_ARG = 3;
namespace espcontrol::reset { OperationInterlock interlock; }
using namespace espcontrol::reset;
int failure = 0;
extern "C" void espcontrol_hosted_ota_failed() __attribute__((weak));
extern "C" int __real_esp_hosted_slave_ota_begin() { return failure == 1 ? ESP_ERR_INVALID_ARG : ESP_OK; }
int esp_hosted_slave_ota_write(uint8_t *, uint32_t) {
  assert(interlock.busy());
  return failure == 2 ? ESP_ERR_INVALID_ARG : ESP_OK;
}
int esp_hosted_slave_ota_end() {
  assert(interlock.busy()); // Release only after cleanup, even on a write failure.
  return failure == 3 ? ESP_ERR_INVALID_ARG : ESP_OK;
}
int esp_hosted_slave_ota_activate() {
  assert(interlock.busy());
  return failure == 4 ? ESP_ERR_INVALID_ARG : ESP_OK;
}
#define ESP_LOGE(...) ((void) 0)
#define ESP_LOGI(...) ((void) 0)
namespace esphome {
namespace watchdog { struct WatchdogManager { explicit WatchdogManager(int) {} }; }
struct Application { void feed_wdt() {} } App;
void yield() {}
namespace c6_recovery {
constexpr size_t CHUNK_SIZE = 1500;
struct C6RecoveryComponent {
  uint8_t firmware[3100]{};
  const uint8_t *firmware_data_ = firmware;
  size_t firmware_size_ = sizeof(firmware);
  bool install_firmware_();
};
}}
''' + hosted_wrappers + r'''
#define esp_hosted_slave_ota_begin __wrap_esp_hosted_slave_ota_begin
namespace esphome::c6_recovery {
''' + recovery + r'''
}
int main() {
  esphome::c6_recovery::C6RecoveryComponent recovery;
  // A rejected begin does not own (and cannot release) the active reservation.
  assert(interlock.begin_installation(true));
  assert(!recovery.install_firmware_());
  assert(interlock.busy());
  interlock.set_coprocessor_busy(false);
  // No UpdateEntity notifications: direct recovery must handle every failure.
  for (int stage = 1; stage <= 4; ++stage) {
    failure = stage;
    assert(!recovery.install_firmware_());
    assert(!interlock.busy());
    failure = 0;
    assert(recovery.install_firmware_());
    assert(interlock.busy()); // Successful recovery still has a scheduled reboot.
    interlock.set_coprocessor_busy(false);
  }
}
'''
with tempfile.TemporaryDirectory() as temporary:
    directory = Path(temporary)
    for name, code in (("native", harness), ("hosted", hosted_harness)):
        test_source = directory / (name + ".cpp")
        test_source.write_text(code)
        executable = directory / name
        extra_sources = []
        if name == "hosted":
            # Match firmware translation units: the weak consumer must not see
            # the optional callback's definition at compile time.
            hook_source = directory / "hook.cpp"
            hook_source.write_text('#include "reset_interlock.h"\n'
                                   'namespace espcontrol::reset { extern OperationInterlock interlock; }\n'
                                   + hosted_hook)
            extra_sources.append(str(hook_source))
        subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
            "-std=c++17", "-Wall", "-Wextra", "-Werror", "-pthread",
            "-I", str(ROOT / "components/espcontrol"), str(test_source),
            "-o", str(executable), *extra_sources], check=True)
        subprocess.run([str(executable)], check=True)
print("Native OTA and direct C6 recovery lifecycle checks passed.")
