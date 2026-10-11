#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
using esp_err_t = int;
using nvs_handle_t = int;
constexpr int ESP_OK = 0, ESP_ERR_NVS_NOT_FOUND = 1, NVS_READWRITE = 1, NVS_READONLY = 0, ESP_ERR_INVALID_ARG = 3;
inline std::vector<uint8_t> identity_flash, identity_pending, dedicated_flash, dedicated_pending;
inline bool shared_full = false;
inline bool fail_open = false, fail_write = false, fail_commit = false;
inline int nvs_open(const char *, int, nvs_handle_t *handle) { *handle = 1; return fail_open ? 2 : ESP_OK; }
inline int nvs_open_from_partition(const char *, const char *, int, nvs_handle_t *handle) { *handle = 2; return fail_open ? 2 : ESP_OK; }
inline int nvs_get_blob(nvs_handle_t handle, const char *, void *target, size_t *length) {
  const auto &identity_flash = handle == 2 ? dedicated_flash : ::identity_flash;
  if (identity_flash.empty()) return ESP_ERR_NVS_NOT_FOUND;
  if (*length < identity_flash.size()) return 2;
  *length = identity_flash.size();
  std::memcpy(target, identity_flash.data(), *length);
  return ESP_OK;
}
inline int nvs_set_blob(nvs_handle_t handle, const char *, const void *source, size_t length) {
  if (fail_write || (shared_full && handle == 1)) return 2;
  auto &identity_pending = handle == 2 ? dedicated_pending : ::identity_pending;
  const auto *bytes = static_cast<const uint8_t *>(source);
  identity_pending.assign(bytes, bytes + length);
  return ESP_OK;
}
inline int nvs_commit(nvs_handle_t handle) {
  if (fail_commit) return 2;
  if (handle == 2) dedicated_flash = dedicated_pending;
  else identity_flash = identity_pending;
  return ESP_OK;
}
inline void nvs_close(nvs_handle_t handle) {
  if (handle == 2) dedicated_pending.clear();
  else identity_pending.clear();
}
