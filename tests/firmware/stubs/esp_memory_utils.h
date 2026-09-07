#pragma once

#include "esphome/core/helpers.h"

inline bool esp_ptr_external_ram(const void *pointer) {
  return pointer == fake_esphome_allocator::last_external_pointer;
}
