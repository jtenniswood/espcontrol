#include "esphome/core/defines.h"

#ifdef ESPCONTROL_RESTART_ON_PRIMARY_CORE
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" [[noreturn]] void __real_esp_restart();

namespace {
void restart_on_primary_core(void *) { __real_esp_restart(); }
}  // namespace

// ESPHome finishes component shutdown in its core 1 loop task. On the P4-86
// engineering-sample chip, the following IDF reboot can fault while flushing
// the cache after resetting core 0. Hand only that final reboot to core 0;
// component teardown and preference flushing stay in their original task.
extern "C" [[noreturn]] void __wrap_esp_restart() {
  if (xPortGetCoreID() != 0 &&
      xTaskCreatePinnedToCore(restart_on_primary_core, "reboot", ESPHOME_LOOP_TASK_STACK_SIZE,
                              nullptr, tskIDLE_PRIORITY + 1, nullptr, 0) == pdPASS) {
    vTaskSuspend(nullptr);
  }
  // Keep the existing restart available if a low-memory device cannot create
  // the handoff task, or if the caller is already on the primary core.
  __real_esp_restart();
}
#endif
