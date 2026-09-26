#pragma once
#include <cstdint>
namespace esphome {
inline uint32_t endpoint_test_now = 1000;
inline uint32_t millis() { return endpoint_test_now; }
}
