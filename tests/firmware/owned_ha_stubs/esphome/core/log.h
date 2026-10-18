#pragma once
template<typename... Args> inline void owned_ha_test_log(Args &&...) {}
#define ESP_LOGD(...) owned_ha_test_log(__VA_ARGS__)
#define ESP_LOGW(...) owned_ha_test_log(__VA_ARGS__)
#define ESP_LOGE(...) owned_ha_test_log(__VA_ARGS__)
