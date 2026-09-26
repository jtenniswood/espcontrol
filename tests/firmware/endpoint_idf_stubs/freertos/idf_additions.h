#pragma once
#include "task.h"
inline void (*probe_test_task)(void*) = nullptr;
inline void *probe_test_argument = nullptr;
inline bool probe_test_allocation_fails = false;
inline BaseType_t xTaskCreateWithCaps(void (*fn)(void*), const char*, unsigned, void *arg,
                                      unsigned, TaskHandle_t *task, unsigned) {
  if (probe_test_allocation_fails) return 0;
  probe_test_task = fn; probe_test_argument = arg; *task = arg; return pdPASS;
}
inline void vTaskDeleteWithCaps(void*) {}
inline void probe_test_run_task() {
  const auto task = probe_test_task; const auto arg = probe_test_argument;
  probe_test_task = nullptr; probe_test_argument = nullptr; task(arg);
}
