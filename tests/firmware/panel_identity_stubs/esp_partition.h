#pragma once
#include <cstdint>
struct esp_partition_t {
  uint32_t address{0xe00000}, size{0x200000};
  int subtype{0x40};
  bool encrypted{false};
  char label[17]{"card_images"};
};
constexpr int ESP_PARTITION_TYPE_DATA = 1, ESP_PARTITION_SUBTYPE_ANY = 255,
              ESP_PARTITION_SUBTYPE_DATA_NVS = 2;
inline bool has_data_partition = false;
inline esp_partition_t data_partition;
inline const esp_partition_t *esp_partition_find_first(int, int, const char *) {
  return has_data_partition ? &data_partition : nullptr;
}
