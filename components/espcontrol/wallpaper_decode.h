#pragma once
#include "wallpaper_format.h"

namespace espcontrol {
struct DecodedWallpaper {
  uint8_t *pixels = nullptr;  // Caller owns this allocation.
  uint32_t width = 0, height = 0;
  size_t size = 0;
  bool rgb888 = false;
};
// Decode once into PSRAM; LVGL subsequently draws the shared pixel buffer.
bool wallpaper_decode(const uint8_t *packet, size_t size, DecodedWallpaper &image);
}  // namespace espcontrol
