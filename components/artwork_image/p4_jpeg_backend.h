#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace artwork_image {

struct HardwareJpegFrame {
  const uint8_t *data{nullptr};
  int width{0};
  int height{0};
  size_t stride_bytes{0};
  bool ppa_scaled{false};
};

// Returns true only when the ESP32-P4 hardware path produced a complete frame.
// Unsupported/progressive inputs and every hardware error deliberately return
// false so the portable libjpeg decoder remains the compatibility fallback.
bool try_decode_p4_jpeg(const uint8_t *data, size_t size, int target_width, int target_height,
                       bool cover_mode, bool big_endian, HardwareJpegFrame &frame);
void release_p4_jpeg_workspace();

}  // namespace artwork_image
}  // namespace esphome
