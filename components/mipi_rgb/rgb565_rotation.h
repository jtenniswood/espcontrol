#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome::mipi_rgb::rotation {

enum class Angle : uint16_t { DEG_0 = 0, DEG_90 = 90, DEG_180 = 180, DEG_270 = 270 };

struct Area {
  int x;
  int y;
  int width;
  int height;
};

inline bool quarter_turn(Angle angle) { return angle == Angle::DEG_90 || angle == Angle::DEG_270; }

inline Area physical_area(int panel_width, int panel_height, Area logical, Angle angle) {
  switch (angle) {
    case Angle::DEG_90:
      return {panel_width - logical.y - logical.height, logical.x, logical.height, logical.width};
    case Angle::DEG_180:
      return {panel_width - logical.x - logical.width, panel_height - logical.y - logical.height,
              logical.width, logical.height};
    case Angle::DEG_270:
      return {logical.y, panel_height - logical.x - logical.width, logical.height, logical.width};
    default:
      return logical;
  }
}

// Clip logical coordinates before rotating, retaining the original source stride.
inline Area clip_area(Area area, int width, int height) {
  const int right = std::min(area.x + area.width, width);
  const int bottom = std::min(area.y + area.height, height);
  area.x = std::max(area.x, 0);
  area.y = std::max(area.y, 0);
  area.width = std::max(0, right - area.x);
  area.height = std::max(0, bottom - area.y);
  return area;
}

// Pixels are opaque RGB565 words: copying preserves the caller's byte order.
// Buffers must not overlap. Strides are in pixels, allowing a packed scratch
// buffer now and direct framebuffer output in a later trial.
inline bool copy_rgb565(const uint16_t *source, size_t source_stride, uint16_t *destination,
                        size_t destination_stride, int width, int height, Angle angle) {
  if (source == nullptr || destination == nullptr || width <= 0 || height <= 0 ||
      source_stride < static_cast<size_t>(width))
    return false;
  if (angle != Angle::DEG_0 && angle != Angle::DEG_90 && angle != Angle::DEG_180 && angle != Angle::DEG_270)
    return false;
  const int output_width = quarter_turn(angle) ? height : width;
  if (destination_stride < static_cast<size_t>(output_width))
    return false;

  if (angle == Angle::DEG_0) {
    for (int y = 0; y < height; ++y)
      std::memcpy(destination + y * destination_stride, source + y * source_stride, width * sizeof(uint16_t));
    return true;
  }
  if (angle == Angle::DEG_180) {
    for (int y = 0; y < height; ++y) {
      const uint16_t *input = source + y * source_stride;
      uint16_t *output = destination + (height - y - 1) * destination_stride;
      for (int x = 0; x < width; ++x)
        output[width - x - 1] = input[x];
    }
    return true;
  }

  // A 32x32 tile touches a small group of source/destination cache lines instead
  // of traversing all destination rows for every source row. No extra tile
  // storage is needed, including when a final tile is partial or odd-sized.
  constexpr int TILE_SIZE = 32;
  for (int top = 0; top < height; top += TILE_SIZE) {
    const int bottom = std::min(top + TILE_SIZE, height);
    for (int left = 0; left < width; left += TILE_SIZE) {
      const int right = std::min(left + TILE_SIZE, width);
      for (int y = top; y < bottom; ++y) {
        const uint16_t *input = source + y * source_stride;
        if (angle == Angle::DEG_90) {
          for (int x = left; x < right; ++x)
            destination[x * destination_stride + height - y - 1] = input[x];
        } else {
          for (int x = left; x < right; ++x)
            destination[(width - x - 1) * destination_stride + y] = input[x];
        }
      }
    }
  }
  return true;
}

// The callback receives a physical area, pixels, and their stride. Splitting
// source strips bounds scratch memory even if LVGL uses a larger draw buffer.
template<typename Draw> bool flush_rgb565(const uint16_t *source, size_t source_stride,
                                          uint16_t *scratch, size_t scratch_pixels, Area logical,
                                          int panel_width, int panel_height, Angle angle, Draw draw) {
  if (source == nullptr || logical.width <= 0 || logical.height <= 0 || panel_width <= 0 || panel_height <= 0 ||
      source_stride < static_cast<size_t>(logical.width))
    return false;
  if (angle != Angle::DEG_0 && angle != Angle::DEG_90 && angle != Angle::DEG_180 && angle != Angle::DEG_270)
    return false;
  const int logical_width = quarter_turn(angle) ? panel_height : panel_width;
  const int logical_height = quarter_turn(angle) ? panel_width : panel_height;
  const auto clipped = clip_area(logical, logical_width, logical_height);
  if (clipped.width == 0 || clipped.height == 0)
    return true;
  source += (clipped.y - logical.y) * source_stride + clipped.x - logical.x;
  if (angle == Angle::DEG_0) {
    draw(clipped, source, source_stride);
    return true;
  }
  if (scratch == nullptr || scratch_pixels < static_cast<size_t>(clipped.width))
    return false;
  const int strip_height = std::min(scratch_pixels / clipped.width, static_cast<size_t>(clipped.height));
  for (int row = 0; row < clipped.height; row += strip_height) {
    const int rows = std::min(strip_height, clipped.height - row);
    const Area strip{clipped.x, clipped.y + row, clipped.width, rows};
    const auto physical = physical_area(panel_width, panel_height, strip, angle);
    if (!copy_rgb565(source + row * source_stride, source_stride, scratch, physical.width,
                     strip.width, strip.height, angle))
      return false;
    draw(physical, scratch, physical.width);
  }
  return true;
}

}  // namespace esphome::mipi_rgb::rotation
