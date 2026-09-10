#pragma once

template<typename... Args>
inline void fake_esp_loge(const char *tag, const char *format, Args... args) {
  (void) tag;
  (void) format;
  ((void) args, ...);
}

#define ESP_LOGE(...) fake_esp_loge(__VA_ARGS__)
