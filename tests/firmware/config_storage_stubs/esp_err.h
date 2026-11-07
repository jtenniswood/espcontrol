#pragma once
using esp_err_t = int;
constexpr esp_err_t ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_NVS_NOT_FOUND = 1;

constexpr esp_err_t ESP_ERR_NVS_NOT_ENOUGH_SPACE = 0x1105;
constexpr esp_err_t ESP_ERR_NVS_VALUE_TOO_LONG = 0x110e;
