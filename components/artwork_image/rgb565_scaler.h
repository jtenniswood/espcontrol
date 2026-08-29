#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace esphome {
namespace artwork_image {

// Scale a packed RGB565 source block into a row-major target buffer while
// preserving the decoder's established forward-mapping and overwrite order.
// Keeping the operation here makes the hot path host-testable and avoids a
// two-byte memcpy plus a full position calculation for every destination
// pixel.
inline bool draw_scaled_rgb565_block(
    uint8_t *target, int target_width, int target_height, int target_bpp,
    int x_offset, int y_offset, double x_scale, double y_scale,
    int source_x, int source_y, int source_width, int source_height,
    const uint8_t *source) {
  if (!target || !source || target_width <= 0 || target_height <= 0 ||
      target_bpp < 2 || source_width <= 0 || source_height <= 0 ||
      x_scale <= 0.0 || y_scale <= 0.0) {
    return false;
  }

  for (int row = 0; row < source_height; row++) {
    for (int col = 0; col < source_width; col++) {
      const int src_x = source_x + col;
      const int src_y = source_y + row;
      const size_t src_offset =
          (static_cast<size_t>(row) * source_width + col) * 2u;
      const int target_x0 = std::max(
          0, x_offset + static_cast<int>(src_x * x_scale));
      const int target_y0 = std::max(
          0, y_offset + static_cast<int>(src_y * y_scale));
      const int target_x1 = std::min(
          target_width,
          x_offset + static_cast<int>(std::ceil((src_x + 1) * x_scale)));
      const int target_y1 = std::min(
          target_height,
          y_offset + static_cast<int>(std::ceil((src_y + 1) * y_scale)));
      if (target_x0 >= target_x1 || target_y0 >= target_y1) continue;

      for (int dy = target_y0; dy < target_y1; dy++) {
        uint8_t *destination =
            target + (static_cast<size_t>(dy) * target_width + target_x0) *
                         target_bpp;
        for (int dx = target_x0; dx < target_x1; dx++) {
          destination[0] = source[src_offset];
          destination[1] = source[src_offset + 1];
          if (target_bpp > 2) destination[2] = 0xFF;
          destination += target_bpp;
        }
      }
    }
  }
  return true;
}

}  // namespace artwork_image
}  // namespace esphome
