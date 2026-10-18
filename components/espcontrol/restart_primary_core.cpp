#include "esphome/core/defines.h"

#ifdef ESPCONTROL_RESTART_ON_PRIMARY_CORE
#include "esp_private/rtc_clk.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/wdt_hal.h"

extern "C" [[noreturn]] void __real_esp_restart();

namespace {
void restart_on_primary_core(void *) { __real_esp_restart(); }

void arm_restart_timeout() {
  // IDF arms this watchdog in esp_restart_noos(), after its shutdown handlers.
  // Cover the task handoff and those handlers too: neither may leave a panel
  // waiting forever after ESPHome has already shut its components down.
  wdt_hal_context_t watchdog;
  wdt_hal_init(&watchdog, WDT_RWDT, 0, false);
  const uint32_t timeout_ticks = static_cast<uint32_t>(10ULL * rtc_clk_slow_freq_get_hz());
  wdt_hal_write_protect_disable(&watchdog);
  wdt_hal_config_stage(&watchdog, WDT_STAGE0, timeout_ticks, WDT_STAGE_ACTION_RESET_SYSTEM);
  wdt_hal_config_stage(&watchdog, WDT_STAGE1, timeout_ticks, WDT_STAGE_ACTION_RESET_RTC);
  wdt_hal_set_flashboot_en(&watchdog, true);
  wdt_hal_enable(&watchdog);
  wdt_hal_write_protect_enable(&watchdog);
}
}  // namespace

// ESPHome finishes component shutdown in its core 1 loop task. On the P4-86
// engineering-sample chip, the following IDF reboot can fault while flushing
// the cache after resetting core 0. Hand only that final reboot to core 0;
// component teardown and preference flushing stay in their original task.
extern "C" [[noreturn]] void __wrap_esp_restart() {
  arm_restart_timeout();
  if (xPortGetCoreID() == 0) {
    __real_esp_restart();
  }
  xTaskCreatePinnedToCore(restart_on_primary_core, "reboot", ESPHOME_LOOP_TASK_STACK_SIZE,
                          nullptr, tskIDLE_PRIORITY + 1, nullptr, 0);
  // A failed allocation must not fall back to the core-1 cache-flush fault.
  // Let the independently running hardware watchdog recover the panel instead.
  // Also stay stopped if another task unexpectedly resumes this caller.
  for (;;) {
    vTaskSuspend(nullptr);
  }
}
#endif
