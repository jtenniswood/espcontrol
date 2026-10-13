#pragma once
#include <cstddef>
#include <cstdint>
using esp_err_t = int;
constexpr int ESP_OK = 0;
constexpr int ESP_PARTITION_TYPE_DATA = 1;
constexpr int ESP_PARTITION_SUBTYPE_ANY = 255;
struct esp_partition_t { size_t size; uint8_t subtype; bool encrypted; };
const esp_partition_t *esp_partition_find_first(int, int, const char *);
esp_err_t esp_partition_read(const esp_partition_t *, size_t, void *, size_t);
esp_err_t esp_partition_write(const esp_partition_t *, size_t, const void *, size_t);
esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t, size_t);
