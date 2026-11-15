#pragma once
#include <cstddef>
#include "esp_err.h"
struct esp_partition_t { size_t size; };
constexpr int ESP_PARTITION_TYPE_DATA = 1;
constexpr int ESP_PARTITION_SUBTYPE_ANY = -1;
const esp_partition_t *esp_partition_find_first(int, int, const char *);
esp_err_t esp_partition_read(const esp_partition_t *, size_t, void *, size_t);
esp_err_t esp_partition_write(const esp_partition_t *, size_t, const void *, size_t);
esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t, size_t);
