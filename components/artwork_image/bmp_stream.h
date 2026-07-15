#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace artwork_image {
namespace bmp {

static constexpr size_t FILE_HEADER_SIZE = 14;
static constexpr size_t REQUIRED_HEADER_SIZE = 34;
static constexpr size_t INFO_HEADER_SIZE = 40;
static constexpr size_t PALETTE_ENTRY_SIZE = 4;
static constexpr size_t MONOCHROME_PALETTE_SIZE = 2 * PALETTE_ENTRY_SIZE;

constexpr bool has_complete_header(size_t buffered_size, size_t data_offset) {
  return data_offset >= REQUIRED_HEADER_SIZE && buffered_size >= data_offset;
}

constexpr size_t row_bytes(size_t width, uint16_t bits_per_pixel) {
  return (width * bits_per_pixel + 7) / 8;
}

constexpr size_t row_stride(size_t width, uint16_t bits_per_pixel) {
  return ((width * bits_per_pixel + 31) / 32) * 4;
}

constexpr bool has_complete_monochrome_palette(size_t data_offset, size_t dib_header_size) {
  return dib_header_size >= INFO_HEADER_SIZE &&
         data_offset >= FILE_HEADER_SIZE + MONOCHROME_PALETTE_SIZE &&
         dib_header_size <= data_offset - FILE_HEADER_SIZE - MONOCHROME_PALETTE_SIZE;
}

constexpr size_t palette_offset(size_t dib_header_size) {
  return FILE_HEADER_SIZE + dib_header_size;
}

}  // namespace bmp
}  // namespace artwork_image
}  // namespace esphome
