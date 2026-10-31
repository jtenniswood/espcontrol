#pragma once
#include "esphome/core/preferences.h"
#include <cstdlib>

using nvs_handle_t = uint32_t;
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0;
constexpr esp_err_t ESP_FAIL = -1;
constexpr esp_err_t ESP_ERR_NVS_NOT_FOUND = 0x1102;

inline esp_err_t nvs_erase_key(nvs_handle_t handle, const char *key) {
  auto *prefs = esphome::global_preferences;
  assert(prefs && handle == prefs->nvs_handle);
  ++prefs->erases;
  if (prefs->fail_erase) return ESP_FAIL;
  const auto value = static_cast<uint32_t>(std::strtoul(key, nullptr, 10));
  if (!prefs->durable.erase(value)) return ESP_ERR_NVS_NOT_FOUND;
  prefs->operations.push_back("erase:" + std::to_string(value));
  return ESP_OK;
}

inline esp_err_t nvs_commit(nvs_handle_t handle) {
  auto *prefs = esphome::global_preferences;
  assert(prefs && handle == prefs->nvs_handle);
  ++prefs->commits;
  return prefs->fail_commit ? ESP_FAIL : ESP_OK;
}
