#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
using esp_err_t = int;
using nvs_handle_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NVS_NOT_FOUND = 1, NVS_READWRITE = 1;
inline std::vector<uint8_t> identity_flash, identity_pending;
inline bool fail_open = false, fail_write = false, fail_commit = false;
inline int nvs_open(const char *, int, nvs_handle_t *handle) { *handle = 1; return fail_open ? 2 : ESP_OK; }
inline int nvs_get_blob(nvs_handle_t, const char *, void *target, size_t *length) {
  if (identity_flash.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if (*length < identity_flash.size()) return 2;
  *length = identity_flash.size();
  std::memcpy(target, identity_flash.data(), *length);
  return ESP_OK;
}
inline int nvs_set_blob(nvs_handle_t, const char *, const void *source, size_t length) {
  if (fail_write) return 2;
  const auto *bytes = static_cast<const uint8_t *>(source);
  identity_pending.assign(bytes, bytes + length);
  return ESP_OK;
}
inline int nvs_commit(nvs_handle_t) { if (fail_commit) return 2; identity_flash = identity_pending; return ESP_OK; }
inline void nvs_close(nvs_handle_t) { identity_pending.clear(); }
