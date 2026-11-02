#pragma once
#include <cstdint>
namespace esphome { inline uint32_t fake_now = 0; inline uint32_t millis() { return fake_now; } }
