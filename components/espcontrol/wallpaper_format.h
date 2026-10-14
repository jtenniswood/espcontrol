#pragma once
#include <cstddef>
#include <cstdint>
#ifdef ESP_PLATFORM
#include <sdkconfig.h>
#endif

namespace espcontrol {
constexpr uint8_t WALLPAPER_DEFAULT_OPACITY_PERCENT = 80;
struct WallpaperOpacity {
  uint8_t off = WALLPAPER_DEFAULT_OPACITY_PERCENT;
  uint8_t active = WALLPAPER_DEFAULT_OPACITY_PERCENT;
};
inline bool wallpaper_opacity_valid(int off, int active) {
  return off >= 0 && off <= 100 && active >= 0 && active <= 100;
}
inline uint8_t wallpaper_opacity_byte(unsigned percent) {
  return static_cast<uint8_t>((percent * 255 + 50) / 100);
}
constexpr size_t WALLPAPER_HEADER_BYTES = 16;
constexpr uint32_t WALLPAPER_MAGIC = 0x31505745;  // EWP1, little endian
constexpr uint32_t WALLPAPER_MAX_SIDE = 480;
constexpr uint32_t WALLPAPER_JPEG_MAGIC = 0x32505745;  // EWP2, baseline JPEG
#if defined(CONFIG_IDF_TARGET_ESP32P4)
constexpr uint32_t WALLPAPER_JPEG_MAX_SIDE = 1280;
constexpr uint32_t WALLPAPER_JPEG_MAX_PIXELS = 1280 * 800;
constexpr bool WALLPAPER_RGB888 = true;
#else
constexpr uint32_t WALLPAPER_JPEG_MAX_SIDE = 480;
constexpr uint32_t WALLPAPER_JPEG_MAX_PIXELS = 480 * 480;
constexpr bool WALLPAPER_RGB888 = false;
#endif
// Fit the single 512 KiB flash slot, including its 16-byte record.
constexpr size_t WALLPAPER_MAX_BYTES = 512 * 1024 - 16;
inline uint32_t wallpaper_u32(const uint8_t *data) {
  return uint32_t(data[0]) | (uint32_t(data[1]) << 8) |
         (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
}
inline bool wallpaper_is_removal(const uint8_t *data, size_t size) {
  return data && size == WALLPAPER_HEADER_BYTES && wallpaper_u32(data) == WALLPAPER_MAGIC &&
         wallpaper_u32(data + 4) == 0 && wallpaper_u32(data + 8) == 0 && wallpaper_u32(data + 12) == 0;
}
// Check the JPEG frame before handing any dimensions to the decoder. Baseline
// encoding avoids the large coefficient buffers required by progressive JPEG.
inline bool wallpaper_jpeg_frame(const uint8_t *data, size_t size, uint32_t width, uint32_t height) {
  if (size < 4 || data[0] != 0xff || data[1] != 0xd8 ||
      data[size - 2] != 0xff || data[size - 1] != 0xd9) return false;
  bool frame = false;
  size_t offset = 2;
  while (offset + 4 <= size) {
    if (data[offset++] != 0xff) return false;
    while (offset < size && data[offset] == 0xff) ++offset;
    if (offset + 3 > size) return false;
    const uint8_t marker = data[offset++];
    const size_t length = (size_t(data[offset]) << 8) | data[offset + 1];
    if (length < 2 || length > size - offset) return false;
    if (marker >= 0xc0 && marker <= 0xcf && marker != 0xc4 && marker != 0xc8 && marker != 0xcc) {
      if (marker != 0xc0 || frame || length < 8) return false;
      const auto *sof = data + offset;
      if (sof[2] != 8 || ((uint32_t(sof[3]) << 8) | sof[4]) != height ||
          ((uint32_t(sof[5]) << 8) | sof[6]) != width ||
          (sof[7] != 1 && sof[7] != 3) || length != size_t(8 + 3 * sof[7])) return false;
      frame = true;
    }
    if (marker == 0xda) return frame;  // Full entropy validation happens before saving.
    if (marker == 0 || marker == 0xd8 || marker == 0xd9) return false;
    offset += length;
  }
  return false;
}
inline bool wallpaper_valid(const uint8_t *data, size_t size) {
  if (!data || size < WALLPAPER_HEADER_BYTES || size > WALLPAPER_MAX_BYTES) return false;
  const auto width = wallpaper_u32(data + 4), height = wallpaper_u32(data + 8);
  if (wallpaper_u32(data) == WALLPAPER_MAGIC)
    return width > 0 && height > 0 && width <= WALLPAPER_MAX_SIDE && height <= WALLPAPER_MAX_SIDE &&
           wallpaper_u32(data + 12) == width * height * 2 &&
           size == WALLPAPER_HEADER_BYTES + width * height * 2;
  return wallpaper_u32(data) == WALLPAPER_JPEG_MAGIC && width > 0 && height > 0 &&
         width <= WALLPAPER_JPEG_MAX_SIDE && height <= WALLPAPER_JPEG_MAX_SIDE &&
         width * height <= WALLPAPER_JPEG_MAX_PIXELS &&
         wallpaper_u32(data + 12) == size - WALLPAPER_HEADER_BYTES &&
         wallpaper_jpeg_frame(data + WALLPAPER_HEADER_BYTES, size - WALLPAPER_HEADER_BYTES, width, height);
}
}  // namespace espcontrol
