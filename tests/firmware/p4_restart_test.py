"""Exercise the production reset path against a busy shared cache model."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/espcontrol/p4_restart.cpp").read_text()
functions = source[source.index("static void IRAM_ATTR"):source.index("#endif  // ESP32-P4 workaround")]
prefix = r'''
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <vector>
#define IRAM_ATTR
#define CACHE_INVALIDATE_ENA 1u
#define CACHE_CLEAN_ENA 2u
#define CACHE_WRITEBACK_ENA 4u
#define CACHE_WRITEBACK_INVALIDATE_ENA 8u
#define CACHE_SYNC_CTRL_REG 0x98u
#define CACHE_MAP_L1_DCACHE 16u
#define HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_REG 0x100u
#define HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_DMA 1u
#define HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_CPU 2u
#define CONFIG_IDF_ENV_FPGA 0
#define WDT_RWDT 0
#define WDT_MWDT0 1
#define WDT_MWDT1 2
#define WDT_STAGE0 0
#define WDT_STAGE1 1
#define WDT_STAGE_ACTION_RESET_SYSTEM 10
#define WDT_STAGE_ACTION_RESET_RTC 11
struct ResetComplete {};
struct WatchdogReset {};
enum Event { INTERRUPTS, RTC_WATCHDOG, STALL, STACK, WAIT_SYNC, WRITEBACK,
             CACHE_OFF, PERIPHERALS, CLOCK, ENTRY_POINT, REMAP, RESET0, RESET1, UNSTALL0 };
std::vector<Event> events;
uint32_t core, busy_reads, sync_kind;
bool interrupts_disabled, other_stalled, cache_enabled, rtc_armed, external_stack;
bool stack_switched, stuck_cache;
int TIMERG0, TIMERG1, _bss_end;
struct wdt_hal_context_t { int inst; int *mwdt_dev; };
void require(bool value) { if (!value) throw std::logic_error("unsafe reset sequence"); }
void rv_utils_intr_global_disable() {
  interrupts_disabled = true; events.push_back(INTERRUPTS);
}
uint32_t esp_cpu_get_core_id() { return core; }
void esp_cpu_stall(uint32_t cpu) {
  require(interrupts_disabled && cpu != core);
  other_stalled = true; events.push_back(STALL);
}
void esp_cpu_reset(uint32_t cpu) {
  // Resetting a live peer can interrupt its cache operation, reproducing the
  // reported hardware fault. Final resets must follow cache/peripheral shutdown.
  require(!cache_enabled);
  events.push_back(cpu ? RESET1 : RESET0);
  if (cpu == core) throw ResetComplete{};
}
void esp_cpu_unstall(uint32_t cpu) { require(cpu == 0 && !cache_enabled); events.push_back(UNSTALL0); }
void wdt_hal_init(wdt_hal_context_t *ctx, int inst, int, bool) { ctx->inst = inst; }
void wdt_hal_write_protect_disable(wdt_hal_context_t *) {}
void wdt_hal_write_protect_enable(wdt_hal_context_t *) {}
uint32_t rtc_clk_slow_freq_get_hz() { return 32768; }
void wdt_hal_config_stage(wdt_hal_context_t *ctx, int stage, uint32_t ticks, int action) {
  require(ctx->inst == WDT_RWDT && ticks == 32768);
  require((stage == WDT_STAGE0 && action == WDT_STAGE_ACTION_RESET_SYSTEM) ||
          (stage == WDT_STAGE1 && action == WDT_STAGE_ACTION_RESET_RTC));
}
void wdt_hal_set_flashboot_en(wdt_hal_context_t *ctx, bool enabled) {
  require(ctx->inst == WDT_RWDT && enabled);
  rtc_armed = true; events.push_back(RTC_WATCHDOG);
}
void wdt_hal_disable(wdt_hal_context_t *ctx) { require(ctx->mwdt_dev == &TIMERG0 || ctx->mwdt_dev == &TIMERG1); }
uintptr_t esp_cpu_get_sp() { return external_stack ? 0x48001000u : 0x4ff01000u; }
bool esp_ptr_external_ram(uintptr_t value) { return value == 0x48001000u; }
void rv_utils_set_sp(void *value) {
  require(reinterpret_cast<uintptr_t>(value) == (reinterpret_cast<uintptr_t>(&_bss_end) & ~uintptr_t{15}));
  external_stack = false; stack_switched = true; events.push_back(STACK);
}
uint32_t read_sync(uint32_t address) {
  require(address == CACHE_SYNC_CTRL_REG && interrupts_disabled && !external_stack);
#if !CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
  require(other_stalled);
#endif
#if SOC_RTC_WDT_SUPPORTED
  require(rtc_armed);
#endif
  events.push_back(WAIT_SYNC);
  if (stuck_cache) {
    if (--busy_reads == 0) {
      require(rtc_armed); throw WatchdogReset{};
    }
    return sync_kind;
  }
  return busy_reads ? (--busy_reads, sync_kind) : 0;
}
#define REG_READ(address) read_sync(address)
void Cache_WriteBack_All(uint32_t map) {
  require(map == CACHE_MAP_L1_DCACHE && !busy_reads && !external_stack);
  events.push_back(WRITEBACK);
}
void Cache_Disable_L2_Cache() {
  require(!busy_reads && !external_stack);
  cache_enabled = false; events.push_back(CACHE_OFF);
}
void esp_system_reset_modules_on_exit() { require(!cache_enabled); events.push_back(PERIPHERALS); }
void rtc_clk_cpu_set_to_default_config() { require(!cache_enabled); events.push_back(CLOCK); }
void ets_set_appcpu_boot_addr(int address) { require(!address && !cache_enabled); events.push_back(ENTRY_POINT); }
void clear_remap(uint32_t address, uint32_t mask) {
  require(!cache_enabled && address == HP_SYS_HP_PSRAM_FLASH_ADDR_INTERCHANGE_REG && mask == 3);
  events.push_back(REMAP);
}
#define REG_CLR_BIT(address, mask) clear_remap(address, mask)
#define ESP_INFINITE_LOOP() throw std::logic_error("reset returned")
'''
suffix = r'''
void initialize(uint32_t cpu, uint32_t pending, uint32_t kind, bool external) {
  events.clear(); core = cpu; busy_reads = pending; sync_kind = kind;
  interrupts_disabled = other_stalled = rtc_armed = stack_switched = stuck_cache = false;
  cache_enabled = true; external_stack = external;
}
int main() {
  try {
    for (uint32_t cpu = 0; cpu < (CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE ? 1u : 2u); ++cpu) {
      for (uint32_t operation : {0u, CACHE_INVALIDATE_ENA, CACHE_CLEAN_ENA,
                                CACHE_WRITEBACK_ENA, CACHE_WRITEBACK_INVALIDATE_ENA}) {
        for (bool external : {false, true}) {
#ifndef CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM
          if (external) continue;
#endif
          initialize(cpu, operation ? 4 : 0, operation, external);
          try { __wrap_esp_restart_noos(); }
          catch (const ResetComplete &) {
            require(!cache_enabled && !busy_reads && !external_stack);
            require(stack_switched == external);
            const auto n = events.size();
#if CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE
            require(events[n-1] == RESET0);
#else
            if (cpu == 0) require(events[n-2] == RESET1 && events[n-1] == RESET0);
            else require(events[n-3] == RESET0 && events[n-2] == UNSTALL0 && events[n-1] == RESET1);
#endif
          }
        }
      }
    }
#if SOC_RTC_WDT_SUPPORTED
    initialize(0, 8, CACHE_WRITEBACK_ENA, false); stuck_cache = true;
    try { __wrap_esp_restart_noos(); }
    catch (const WatchdogReset &) {
      require(rtc_armed && cache_enabled); // No writeback/reset while busy.
      return 0;
    }
    return 1;
#else
    return 0;
#endif
  } catch (const std::logic_error &) { return 1; }
}
'''
variants = (
    ("dual-psram-external", 0, 1, 1, 1),
    ("dual-internal", 0, 0, 0, 1),
    ("single-psram", 1, 1, 1, 1),
    ("dual-no-watchdog", 0, 1, 0, 0),
)
mutations = {
    "reset-before-stall": functions.replace(
        "  esp_cpu_stall(core_id == 0 ? 1 : 0);",
        "  esp_cpu_reset(core_id == 0 ? 1 : 0);\n  esp_cpu_stall(core_id == 0 ? 1 : 0);",
    ),
    "missing-sync-wait": functions.replace(
        "  while (REG_READ(CACHE_SYNC_CTRL_REG) & sync_operations) {}",
        "  (void) sync_operations;",
    ),
}
assert all(changed != functions for changed in mutations.values())
with tempfile.TemporaryDirectory(prefix="espcontrol-p4-restart-") as temporary:
    directory = Path(temporary)
    for name, single, psram, external, watchdog in variants:
        defines = (f"#define CONFIG_ESP_SYSTEM_SINGLE_CORE_MODE {single}\n"
                   f"#define CONFIG_SPIRAM {psram}\n"
                   f"#define CONFIG_SPIRAM_INSTRUCTIONS_RODATA {psram}\n"
                   f"#define SOC_RTC_WDT_SUPPORTED {watchdog}\n"
                   f"#define SOC_WDT_SUPPORTED {watchdog}\n")
        if external:
            defines += "#define CONFIG_FREERTOS_TASK_CREATE_ALLOW_EXT_MEM 1\n"
        cases = {"production": functions}
        if name == "dual-psram-external":
            cases.update(mutations)
        for case, implementation in cases.items():
            cpp = directory / f"{name}-{case}.cpp"
            executable = cpp.with_suffix("")
            cpp.write_text(defines + prefix + implementation + suffix)
            subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
                "-std=c++20", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                str(cpp), "-o", str(executable)], check=True)
            result = subprocess.run([str(executable)], check=False, timeout=10)
            assert (result.returncode == 0) == (case == "production"), (name, case, result.returncode)
print("P4 reset path: CPU origins/cache operations/stack/watchdog variants pass; unsafe mutations rejected.")
