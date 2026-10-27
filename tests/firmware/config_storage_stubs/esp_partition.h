#pragma once
#include "esp_err.h"
#include "storage_fixture.h"
struct esp_partition_t {
  size_t size{2 * 1024 * 1024};
};
constexpr int ESP_PARTITION_TYPE_DATA = 1, ESP_PARTITION_SUBTYPE_ANY = 255;
inline esp_partition_t test_partition;
inline const esp_partition_t *esp_partition_find_first(int, int, const char *) {
  return storage_fixture::has_partition ? &test_partition : nullptr;
}
inline esp_err_t esp_partition_read(const esp_partition_t *, size_t offset,
                                    void *output, size_t size) {
  using namespace storage_fixture;
  ++read_calls;
  if (fail_read || offset > flash.size() || size > flash.size() - offset)
    return ESP_FAIL;
  std::memcpy(output, flash.data() + offset, size);
  return ESP_OK;
}
inline esp_err_t esp_partition_write(const esp_partition_t *, size_t offset,
                                     const void *input, size_t size) {
  using namespace storage_fixture;
  if (offset > flash.size() || size > flash.size() - offset)
    return ESP_FAIL;
  const auto *data = static_cast<const uint8_t *>(input);
  const size_t count = prefix(size);
  for (size_t i = 0; i < count; ++i) {
    if ((flash[offset + i] & data[i]) != data[i])
      return ESP_FAIL;
    flash[offset + i] &= data[i];
  }
  maybe_cut();
  return ESP_OK;
}
inline esp_err_t esp_partition_erase_range(const esp_partition_t *,
                                           size_t offset, size_t size) {
  using namespace storage_fixture;
  if (offset % 4096 || size % 4096 || offset > flash.size() ||
      size > flash.size() - offset)
    return ESP_FAIL;
  const size_t count = prefix(size);
  std::fill_n(flash.data() + offset, count, 0xFF);
  maybe_cut();
  return ESP_OK;
}
