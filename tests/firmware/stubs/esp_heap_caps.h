#pragma once

#include <cstddef>
#include <cstdlib>

constexpr unsigned MALLOC_CAP_SPIRAM = 1;
constexpr unsigned MALLOC_CAP_8BIT = 2;

inline size_t heap_caps_outstanding_allocations = 0;
inline void *heap_caps_malloc(size_t size, unsigned) {
  auto *memory = std::malloc(size);
  if (memory) ++heap_caps_outstanding_allocations;
  return memory;
}
inline void heap_caps_free(void *memory) {
  if (memory) --heap_caps_outstanding_allocations;
  std::free(memory);
}
