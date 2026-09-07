#pragma once

#include "esphome/core/helpers.h"

inline bool esp_ptr_external_ram(const void *pointer) {
  return fake_esphome_allocator::external_pointers.count(pointer) != 0;
}
