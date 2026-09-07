#pragma once

#include "espcontrol_app_core.h"

// Temporary facade for existing card/YAML entry points. Production state has
// an application owner; the fallback is compiled only for isolated host tests.
inline CompanionRuntimeService &companion_runtime_service() {
  if (auto *core = espcontrol::active_espcontrol_app_core()) return core->companion_runtime();
#if defined(USE_ESP32)
  std::abort();
#else
  static CompanionRuntimeService host_test_runtime;
  return host_test_runtime;
#endif
}
