#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

using esp_err_t = int;
using esp_partition_subtype_t = uint8_t;

constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_FAIL = -1;
constexpr esp_err_t ESP_ERR_NO_MEM = 0x101;
constexpr esp_err_t ESP_ERR_INVALID_ARG = 0x102;
constexpr esp_err_t ESP_ERR_INVALID_SIZE = 0x103;
constexpr esp_err_t ESP_ERR_INVALID_STATE = 0x104;
constexpr esp_err_t ESP_ERR_NOT_FOUND = 0x105;
constexpr int ESP_PARTITION_TYPE_DATA = 1;

struct esp_partition_t {
  size_t size{0};
};

const esp_partition_t *esp_partition_find_first(int type, esp_partition_subtype_t subtype,
                                                 const char *label);
esp_err_t esp_partition_read(const esp_partition_t *partition, size_t offset,
                             void *destination, size_t size);
esp_err_t esp_partition_write(const esp_partition_t *partition, size_t offset,
                              const void *source, size_t size);
esp_err_t esp_partition_erase_range(const esp_partition_t *partition, size_t offset,
                                    size_t size);

inline const char *esp_err_to_name(esp_err_t error) {
  return error == ESP_OK ? "ESP_OK" : "ESP_FAIL";
}

inline size_t strlcpy(char *destination, const char *source, size_t size) {
  size_t length = std::strlen(source);
  if (size > 0) {
    size_t copied = length < size - 1 ? length : size - 1;
    if (copied > 0) std::memcpy(destination, source, copied);
    destination[copied] = '\0';
  }
  return length;
}
