#pragma once
#include <cstddef>
#include <cstdint>

namespace espcontrol {
constexpr size_t WALLPAPER_HEADER_BYTES = 16;
constexpr uint32_t WALLPAPER_MAGIC = 0x31505745;  // EWP1, little endian
constexpr uint32_t WALLPAPER_MAX_SIDE = 480;
constexpr size_t WALLPAPER_MAX_BYTES = WALLPAPER_HEADER_BYTES + 2 * WALLPAPER_MAX_SIDE * WALLPAPER_MAX_SIDE;
inline uint32_t wallpaper_u32(const uint8_t *data) {
  return uint32_t(data[0]) | (uint32_t(data[1]) << 8) |
         (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
}
inline bool wallpaper_is_removal(const uint8_t *data, size_t size) {
  return data && size == WALLPAPER_HEADER_BYTES && wallpaper_u32(data) == WALLPAPER_MAGIC &&
         wallpaper_u32(data + 4) == 0 && wallpaper_u32(data + 8) == 0 && wallpaper_u32(data + 12) == 0;
}
inline bool wallpaper_valid(const uint8_t *data, size_t size) {
  if (!data || size < WALLPAPER_HEADER_BYTES || size > WALLPAPER_MAX_BYTES ||
      wallpaper_u32(data) != WALLPAPER_MAGIC) return false;
  const auto width = wallpaper_u32(data + 4), height = wallpaper_u32(data + 8);
  return width > 0 && height > 0 && width <= WALLPAPER_MAX_SIDE && height <= WALLPAPER_MAX_SIDE &&
         wallpaper_u32(data + 12) == width * height * 2 &&
         size == WALLPAPER_HEADER_BYTES + width * height * 2;
}
}  // namespace espcontrol
