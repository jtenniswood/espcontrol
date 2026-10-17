"""Exercise the production restart wrapper with deterministic task stubs."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
compiler = sys.argv[1] if len(sys.argv) > 1 else "c++"

with tempfile.TemporaryDirectory() as directory:
    root = Path(directory)
    headers = {
        "esphome/core/defines.h": "#define ESPHOME_LOOP_TASK_STACK_SIZE 8192\n",
        "freertos/FreeRTOS.h": "#pragma once\n#define pdPASS 1\n#define tskIDLE_PRIORITY 0\n",
        "freertos/task.h": """#pragma once
#include <cstdint>
int xPortGetCoreID();
int xTaskCreatePinnedToCore(void (*)(void *), const char *, uint32_t, void *, unsigned, void *, int);
void vTaskSuspend(void *);
""",
    }
    for name, text in headers.items():
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    source = root / "test.cpp"
    source.write_text(r'''
#include <cassert>
#include <cstdint>
#include <cstring>
int core = 0, created = 0, restarted = 0, suspended = 0, create_result = 1;
void (*callback)(void *) = nullptr;
struct Restarted {};
struct Suspended {};
int xPortGetCoreID() { return core; }
int xTaskCreatePinnedToCore(void (*fn)(void *), const char *name, uint32_t stack,
                            void *arg, unsigned priority, void *handle, int target) {
  assert(std::strcmp(name, "reboot") == 0 && stack == 8192 && !arg && !handle);
  assert(priority == 1 && target == 0);
  ++created; callback = fn; return create_result;
}
void vTaskSuspend(void *task) { assert(!task); ++suspended; throw Suspended{}; }
extern "C" [[noreturn]] void __real_esp_restart() { ++restarted; throw Restarted{}; }
extern "C" [[noreturn]] void __wrap_esp_restart();
int main() {
  try { __wrap_esp_restart(); } catch (const Restarted &) {}
  assert(restarted == 1 && created == 0 && suspended == 0);
  core = 1;
  try { __wrap_esp_restart(); } catch (const Suspended &) {}
  assert(restarted == 1 && created == 1 && suspended == 1 && callback);
  core = 0;
  try { callback(nullptr); } catch (const Restarted &) {}
  assert(restarted == 2 && created == 1);
  core = 1; create_result = 0;
  try { __wrap_esp_restart(); } catch (const Restarted &) {}
  assert(restarted == 3 && created == 2 && suspended == 1);
}
''')
    output = root / "test"
    production = ROOT / "components/espcontrol/restart_primary_core.cpp"
    subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-DESPCONTROL_RESTART_ON_PRIMARY_CORE", "-I", str(root),
                    str(source), str(production), "-o", str(output)], check=True)
    subprocess.run([str(output)], check=True)
    subprocess.run([compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root), "-c", str(production), "-o", str(root / "inactive.o")], check=True)
print("Primary-core restart, handoff, allocation failure and inactive profile checks passed")
