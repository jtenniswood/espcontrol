#pragma once
#include "esp_partition.h"
using nvs_handle_t = unsigned;
constexpr int NVS_READWRITE = 1;
esp_err_t nvs_open(const char *, int, nvs_handle_t *);
esp_err_t nvs_get_u8(nvs_handle_t, const char *, uint8_t *);
esp_err_t nvs_set_u8(nvs_handle_t, const char *, uint8_t);
esp_err_t nvs_get_u16(nvs_handle_t, const char *, uint16_t *);
esp_err_t nvs_set_u16(nvs_handle_t, const char *, uint16_t);
esp_err_t nvs_commit(nvs_handle_t);
void nvs_close(nvs_handle_t);
