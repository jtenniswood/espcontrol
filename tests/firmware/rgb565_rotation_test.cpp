#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <utility>
#include <vector>

#include "rgb565_rotation.h"

using esphome::mipi_rgb::rotation::Angle;
using esphome::mipi_rgb::rotation::Area;
using esphome::mipi_rgb::rotation::copy_rgb565;
using esphome::mipi_rgb::rotation::flush_rgb565;

constexpr uint16_t GUARD = 0xD00D;
constexpr size_t GUARD_SIZE = 17;
constexpr std::array<Angle, 4> ANGLES{Angle::DEG_0, Angle::DEG_90, Angle::DEG_180, Angle::DEG_270};

// Independent per-pixel reference, using global coordinates rather than strips
// or the production area transform. Rectangular panels expose swapped-axis bugs.
std::pair<int, int> reference_point(int x, int y, int width, int height, Angle angle) {
  switch (angle) {
    case Angle::DEG_90: return {width - 1 - y, x};
    case Angle::DEG_180: return {width - 1 - x, height - 1 - y};
    case Angle::DEG_270: return {y, height - 1 - x};
    default: return {x, y};
  }
}

void check_copy(int width, int height, Angle angle, int padding) {
  const bool quarter = angle == Angle::DEG_90 || angle == Angle::DEG_270;
  const int output_width = quarter ? height : width;
  const int output_height = quarter ? width : height;
  const size_t source_stride = width + padding;
  const size_t destination_stride = output_width + padding + 1;
  std::vector<uint16_t> source(GUARD_SIZE * 2 + source_stride * height, GUARD);
  for (int y = 0; y < height; ++y)
    for (int x = 0; x < width; ++x)
      source[GUARD_SIZE + y * source_stride + x] = static_cast<uint16_t>((y * 137 + x * 29 + 0xAB12) ^ (x << 9));
  const auto original_source = source;
  std::vector<uint16_t> destination(GUARD_SIZE * 2 + destination_stride * output_height, GUARD);
  auto expected = destination;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const auto point = reference_point(x, y, output_width, output_height, angle);
      expected[GUARD_SIZE + point.second * destination_stride + point.first] =
          source[GUARD_SIZE + y * source_stride + x];
    }
  }
  assert(copy_rgb565(source.data() + GUARD_SIZE, source_stride, destination.data() + GUARD_SIZE,
                     destination_stride, width, height, angle));
  assert(destination == expected);  // Includes guards and padding in every row.
  assert(source == original_source);
}

void check_flush(int panel_width, int panel_height, Area logical, Angle angle, size_t capacity) {
  const bool quarter = angle == Angle::DEG_90 || angle == Angle::DEG_270;
  const int logical_width = quarter ? panel_height : panel_width;
  const int logical_height = quarter ? panel_width : panel_height;
  const int source_x_offset = 3;
  const int source_y_offset = 2;
  const size_t source_stride = source_x_offset + logical.width + 7;
  std::vector<uint16_t> source(source_stride * (logical.height + source_y_offset + 1), GUARD);
  const size_t first = source_y_offset * source_stride + source_x_offset;
  for (int y = 0; y < logical.height; ++y)
    for (int x = 0; x < logical.width; ++x)
      source[first + y * source_stride + x] = static_cast<uint16_t>(0x1200 + x * 37 + y * 149);
  const auto original_source = source;
  std::vector<uint16_t> scratch(GUARD_SIZE * 2 + capacity, GUARD);
  std::vector<uint16_t> framebuffer(GUARD_SIZE * 2 + panel_width * panel_height, GUARD);
  auto expected = framebuffer;
  for (int y = 0; y < logical.height; ++y) {
    for (int x = 0; x < logical.width; ++x) {
      const int global_x = logical.x + x;
      const int global_y = logical.y + y;
      if (global_x < 0 || global_y < 0 || global_x >= logical_width || global_y >= logical_height)
        continue;
      const auto point = reference_point(global_x, global_y, panel_width, panel_height, angle);
      expected[GUARD_SIZE + point.second * panel_width + point.first] = source[first + y * source_stride + x];
    }
  }
  assert(flush_rgb565(source.data() + first, source_stride, scratch.data() + GUARD_SIZE, capacity,
                      logical, panel_width, panel_height, angle,
                      [&](Area area, const uint16_t *pixels, size_t stride) {
    assert(area.x >= 0 && area.y >= 0);
    assert(area.x + area.width <= panel_width && area.y + area.height <= panel_height);
    for (int y = 0; y < area.height; ++y)
      for (int x = 0; x < area.width; ++x)
        framebuffer[GUARD_SIZE + (area.y + y) * panel_width + area.x + x] = pixels[y * stride + x];
  }));
  assert(framebuffer == expected);
  assert(source == original_source);
  for (size_t i = 0; i < GUARD_SIZE; ++i) {
    assert(scratch[i] == GUARD);
    assert(scratch[GUARD_SIZE + capacity + i] == GUARD);
  }
}

int main() {
  for (int width : {1, 2, 3, 15, 31, 32, 33, 59, 60, 61, 65})
    for (int height : {1, 2, 3, 15, 31, 32, 33, 59, 60, 61, 65})
      for (Angle angle : ANGLES)
        for (int padding : {0, 1, 7})
          check_copy(width, height, angle, padding);

  for (auto panel : {std::pair{7, 11}, std::pair{480, 480}, std::pair{480, 320}}) {
    for (Angle angle : ANGLES) {
      const bool quarter = angle == Angle::DEG_90 || angle == Angle::DEG_270;
      const int width = quarter ? panel.second : panel.first;
      const int height = quarter ? panel.first : panel.second;
      for (Area area : {Area{0, 0, width, height}, Area{1, 2, width - 1, height - 2},
                        Area{-3, -2, width + 5, height + 4}, Area{width - 3, height - 2, 7, 5},
                        Area{-9, -8, 3, 4}, Area{width + 2, height + 1, 5, 3}}) {
        const size_t normal_capacity = std::max(panel.first * panel.second / 8, std::max(panel.first, panel.second));
        check_flush(panel.first, panel.second, area, angle, normal_capacity);
        check_flush(panel.first, panel.second, area, angle, width);  // One source row per strip.
      }
    }
  }

  uint16_t source[8]{};
  uint16_t destination[8]{};
  assert(!copy_rgb565(nullptr, 2, destination, 2, 2, 2, Angle::DEG_0));
  assert(!copy_rgb565(source, 2, nullptr, 2, 2, 2, Angle::DEG_0));
  assert(!copy_rgb565(source, 1, destination, 2, 2, 2, Angle::DEG_0));
  assert(!copy_rgb565(source, 2, destination, 1, 2, 2, Angle::DEG_90));
  assert(!copy_rgb565(source, 2, destination, 2, 0, 2, Angle::DEG_0));
  assert(!copy_rgb565(source, 2, destination, 2, 2, 2, static_cast<Angle>(45)));
  int calls = 0;
  const auto draw = [&](Area, const uint16_t *, size_t) { ++calls; };
  assert(!flush_rgb565(source, 2, nullptr, 0, {0, 0, 2, 2}, 2, 2, Angle::DEG_90, draw));
  assert(!flush_rgb565(source, 2, destination, 1, {0, 0, 2, 2}, 2, 2, Angle::DEG_270, draw));
  assert(calls == 0);
  assert(flush_rgb565(source, 2, nullptr, 0, {0, 0, 2, 2}, 2, 2, Angle::DEG_0, draw));
  assert(calls == 1);
  std::cout << "RGB565 rotation, clipping, strides, strip capacity, and guards passed\n";
}
