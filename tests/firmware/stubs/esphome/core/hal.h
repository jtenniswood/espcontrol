#pragma once

#include <cstdint>

namespace esphome {

inline uint32_t millis() {
  static uint32_t now = 0;
  return ++now;
}

}  // namespace esphome
