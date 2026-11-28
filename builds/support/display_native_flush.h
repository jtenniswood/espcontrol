#pragma once

#include <cstddef>
#include <cstdint>

namespace espcontrol::display_native_flush {

struct Region {
  int x;
  int y;
  int width;
  int height;
};

// Match the V3's clockwise rotation and the two-pixel ESPHome draw rounding.
// Return false whenever the original ESPHome flush must handle the region.
inline bool plan_clockwise_region(Region input, int panel_width, int panel_height, size_t capacity,
                                  Region &output) {
  if (input.x < 0 || input.y < 0 || input.width <= 0 || input.height <= 0 || input.height % 2 != 0 ||
      input.width > panel_height || input.height > panel_width ||
      input.x > panel_height - input.width || input.y > panel_width - input.height) return false;

  output = {panel_width - input.y - input.height, input.x, input.height, input.width};
  return static_cast<size_t>(output.width) * output.height * sizeof(uint16_t) <= capacity;
}

}  // namespace espcontrol::display_native_flush
