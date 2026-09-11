#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "rgb565_scaler.h"

using esphome::artwork_image::draw_scaled_rgb565_block;

static void draw_reference(
    std::vector<uint8_t> &target, int target_width, int target_height,
    int target_bpp, int x_offset, int y_offset, double x_scale,
    double y_scale, int source_x, int source_y, int source_width,
    int source_height, const std::vector<uint8_t> &source) {
  for (int row = 0; row < source_height; row++) {
    for (int col = 0; col < source_width; col++) {
      int src_x = source_x + col;
      int src_y = source_y + row;
      size_t src_offset =
          (static_cast<size_t>(row) * source_width + col) * 2u;
      int target_x0 = std::max(
          0, x_offset + static_cast<int>(src_x * x_scale));
      int target_y0 = std::max(
          0, y_offset + static_cast<int>(src_y * y_scale));
      int target_x1 = std::min(
          target_width,
          x_offset + static_cast<int>(std::ceil((src_x + 1) * x_scale)));
      int target_y1 = std::min(
          target_height,
          y_offset + static_cast<int>(std::ceil((src_y + 1) * y_scale)));
      for (int dy = target_y0; dy < target_y1; dy++) {
        for (int dx = target_x0; dx < target_x1; dx++) {
          size_t dst =
              (static_cast<size_t>(dy) * target_width + dx) * target_bpp;
          target[dst] = source[src_offset];
          target[dst + 1] = source[src_offset + 1];
          if (target_bpp > 2) target[dst + 2] = 0xFF;
        }
      }
    }
  }
}

static void assert_case(int target_width, int target_height, int target_bpp,
                        int x_offset, int y_offset, double x_scale,
                        double y_scale, int source_width, int source_height) {
  std::vector<uint8_t> source(
      static_cast<size_t>(source_width) * source_height * 2u);
  for (size_t i = 0; i < source.size(); i++) source[i] = i * 37u + 11u;
  std::vector<uint8_t> expected(
      static_cast<size_t>(target_width) * target_height * target_bpp, 0x33);
  std::vector<uint8_t> actual = expected;
  draw_reference(expected, target_width, target_height, target_bpp,
                 x_offset, y_offset, x_scale, y_scale, 0, 0,
                 source_width, source_height, source);
  assert(draw_scaled_rgb565_block(
      actual.data(), target_width, target_height, target_bpp,
      x_offset, y_offset, x_scale, y_scale, 0, 0,
      source_width, source_height, source.data()));
  assert(actual == expected);
}

int main() {
  assert_case(8, 8, 2, 0, 0, 2.0, 2.0, 4, 4);
  assert_case(7, 5, 2, -2, 0, 1.75, 1.25, 6, 4);
  assert_case(9, 6, 3, 1, -1, 1.4, 1.8, 5, 5);
  assert(!draw_scaled_rgb565_block(
      nullptr, 1, 1, 2, 0, 0, 1.0, 1.0, 0, 0, 1, 1, nullptr));
  return 0;
}
