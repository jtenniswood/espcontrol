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
        "soc/rtc.h": "#pragma once\n#include <cstdint>\nuint32_t rtc_clk_slow_freq_get_hz();\n",
        "hal/wdt_hal.h": """#pragma once
#include <cstdint>
enum { WDT_RWDT = 3, WDT_STAGE0 = 0, WDT_STAGE1 = 1,
       WDT_STAGE_ACTION_RESET_SYSTEM = 4, WDT_STAGE_ACTION_RESET_RTC = 5 };
struct wdt_hal_context_t { int inst; };
void wdt_hal_init(wdt_hal_context_t *, int, uint32_t, bool);
void wdt_hal_write_protect_disable(wdt_hal_context_t *);
void wdt_hal_config_stage(wdt_hal_context_t *, int, uint32_t, int);
void wdt_hal_set_flashboot_en(wdt_hal_context_t *, bool);
void wdt_hal_enable(wdt_hal_context_t *);
void wdt_hal_write_protect_enable(wdt_hal_context_t *);
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
#include "hal/wdt_hal.h"
int core = 0, created = 0, restarted = 0, suspended = 0, create_result = 1;
int armed = 0, configured = 0;
bool protected_registers = true, enabled = false, flashboot = false, resume_once = false;
void (*callback)(void *) = nullptr;
struct Restarted {};
struct Suspended {};
uint32_t rtc_clk_slow_freq_get_hz() { return 136000; }
void wdt_hal_init(wdt_hal_context_t *wdt, int instance, uint32_t prescaler, bool interrupt) {
  assert(instance == WDT_RWDT && prescaler == 0 && !interrupt && protected_registers);
  wdt->inst = instance; enabled = false; flashboot = false; configured = 0;
}
void wdt_hal_write_protect_disable(wdt_hal_context_t *wdt) {
  assert(wdt->inst == WDT_RWDT && protected_registers); protected_registers = false;
}
void wdt_hal_config_stage(wdt_hal_context_t *wdt, int stage, uint32_t ticks, int action) {
  assert(wdt->inst == WDT_RWDT && !protected_registers && !enabled);
  assert(ticks == 1360000);
  assert(stage == configured);
  assert(action == (stage == WDT_STAGE0 ? WDT_STAGE_ACTION_RESET_SYSTEM : WDT_STAGE_ACTION_RESET_RTC));
  ++configured;
}
void wdt_hal_set_flashboot_en(wdt_hal_context_t *wdt, bool value) {
  assert(wdt->inst == WDT_RWDT && !protected_registers && configured == 2 && value);
  flashboot = value;
}
void wdt_hal_enable(wdt_hal_context_t *wdt) {
  assert(wdt->inst == WDT_RWDT && !protected_registers && flashboot && configured == 2);
  enabled = true; ++armed;
}
void wdt_hal_write_protect_enable(wdt_hal_context_t *wdt) {
  assert(wdt->inst == WDT_RWDT && !protected_registers && enabled); protected_registers = true;
}
void assert_recovery_armed() {
  assert(enabled && protected_registers && flashboot && configured == 2);
}
int xPortGetCoreID() { return core; }
int xTaskCreatePinnedToCore(void (*fn)(void *), const char *name, uint32_t stack,
                            void *arg, unsigned priority, void *handle, int target) {
  assert_recovery_armed();
  assert(std::strcmp(name, "reboot") == 0 && stack == 8192 && !arg && !handle);
  assert(priority == 1 && target == 0);
  ++created; callback = fn; return create_result;
}
void vTaskSuspend(void *task) {
  assert(!task); assert_recovery_armed(); ++suspended;
  if (resume_once) { resume_once = false; return; }
  throw Suspended{};
}
extern "C" [[noreturn]] void __real_esp_restart() {
  assert(core == 0); assert_recovery_armed(); ++restarted; throw Restarted{};
}
extern "C" [[noreturn]] void __wrap_esp_restart();
int main() {
  try { __wrap_esp_restart(); } catch (const Restarted &) {}
  assert(restarted == 1 && created == 0 && suspended == 0 && armed == 1);
  core = 1;
  try { __wrap_esp_restart(); } catch (const Suspended &) {}
  assert(restarted == 1 && created == 1 && suspended == 1 && callback && armed == 2);
  core = 0;
  try { callback(nullptr); } catch (const Restarted &) {}
  assert(restarted == 2 && created == 1 && armed == 2);
  core = 1; create_result = 0;
  try { __wrap_esp_restart(); } catch (const Suspended &) {}
  assert(restarted == 2 && created == 2 && suspended == 2 && armed == 3);
  // Neither an allocation failure nor an unexpected resume may call IDF on core 1.
  resume_once = true;
  try { __wrap_esp_restart(); } catch (const Suspended &) {}
  assert(restarted == 2 && created == 3 && suspended == 4 && armed == 4);
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
print("Primary-core restart, watchdog-before-handoff, protected allocation failure, resumed caller and inactive profile checks passed")
