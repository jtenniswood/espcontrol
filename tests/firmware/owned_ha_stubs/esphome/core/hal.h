#pragma once
#include <cstdint>
namespace esphome {
inline uint32_t owned_ha_test_now = 1000;
inline uint32_t millis() { return owned_ha_test_now; }
}
