#pragma once
#include "nvs.h"
#include "esp_partition.h"
inline int fail_init = 0;
inline esp_partition_t initialized_partition;
inline int nvs_flash_init_partition_ptr(const esp_partition_t *partition) {
  initialized_partition = *partition;
  return fail_init;
}
