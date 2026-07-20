#pragma once

#include <cstdint>

using SemaphoreHandle_t = void *;

inline SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() {
  return reinterpret_cast<SemaphoreHandle_t>(static_cast<uintptr_t>(1));
}
inline int xSemaphoreTakeRecursive(SemaphoreHandle_t, uint32_t) { return 1; }
inline int xSemaphoreGiveRecursive(SemaphoreHandle_t) { return 1; }
