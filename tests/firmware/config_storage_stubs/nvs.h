#pragma once
#include "esp_err.h"
#include "storage_fixture.h"
using nvs_handle_t = int;
constexpr int NVS_READWRITE = 1;
inline esp_err_t nvs_open(const char *, int, nvs_handle_t *handle) {
  *handle = 1;
  return ESP_OK;
}
inline esp_err_t nvs_get_blob(nvs_handle_t, const char *key, void *output,
                              size_t *size) {
  using namespace storage_fixture;
  ++read_calls;
  if (fail_read)
    return ESP_FAIL;
  const auto it = blobs.find(key);
  if (it == blobs.end())
    return ESP_ERR_NVS_NOT_FOUND;
  if (output != nullptr) {
    if (*size < it->second.size())
      return ESP_FAIL;
    std::memcpy(output, it->second.data(), it->second.size());
  }
  *size = it->second.size();
  return ESP_OK;
}
inline esp_err_t nvs_set_blob(nvs_handle_t, const char *key, const void *input,
                              size_t size) {
  using namespace storage_fixture;
  const auto *data = static_cast<const uint8_t *>(input);
  const size_t count = prefix(size);
  blobs[key].assign(data, data + count);
  maybe_cut();
  return ESP_OK;
}
inline esp_err_t nvs_commit(nvs_handle_t) {
  using namespace storage_fixture;
  prefix(0);
  maybe_cut();
  return fail_commit_at == mutation ? ESP_FAIL : ESP_OK;
}
