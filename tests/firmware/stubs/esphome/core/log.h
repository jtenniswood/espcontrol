#pragma once

namespace esphome {

template<typename... Args> inline void host_log(const char *, const char *, Args &&...) {}

}  // namespace esphome

#define ESP_LOGD(...) ::esphome::host_log(__VA_ARGS__)
#define ESP_LOGI(...) ::esphome::host_log(__VA_ARGS__)
#define ESP_LOGW(...) ::esphome::host_log(__VA_ARGS__)
#define ESP_LOGE(...) ::esphome::host_log(__VA_ARGS__)
