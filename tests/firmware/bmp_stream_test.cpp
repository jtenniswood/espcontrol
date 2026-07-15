#include <cassert>

#include "bmp_stream.h"

using esphome::artwork_image::bmp::has_complete_header;
using esphome::artwork_image::bmp::has_complete_monochrome_palette;
using esphome::artwork_image::bmp::palette_offset;
using esphome::artwork_image::bmp::row_bytes;
using esphome::artwork_image::bmp::row_stride;

int main() {
  // A split HTTP response must retain the header until the pixel-data offset.
  assert(!has_complete_header(13, 54));
  assert(!has_complete_header(14, 54));
  assert(!has_complete_header(53, 54));
  assert(has_complete_header(54, 54));
  assert(!has_complete_header(54, 33));

  // Standard 1-bit Windows BMPs need two four-byte colour-table entries.
  assert(!has_complete_monochrome_palette(54, 40));
  assert(has_complete_monochrome_palette(62, 40));
  assert(palette_offset(40) == 54);
  assert(has_complete_monochrome_palette(130, 108));
  assert(!has_complete_monochrome_palette(61, 40));
  assert(!has_complete_monochrome_palette(62, 39));

  // Every BMP row is padded to a four-byte boundary, including 1-bit rows.
  assert(row_bytes(1, 1) == 1);
  assert(row_stride(1, 1) == 4);
  assert(row_bytes(9, 1) == 2);
  assert(row_stride(9, 1) == 4);
  assert(row_bytes(32, 1) == 4);
  assert(row_stride(32, 1) == 4);
  assert(row_bytes(33, 1) == 5);
  assert(row_stride(33, 1) == 8);

  assert(row_bytes(3, 24) == 9);
  assert(row_stride(3, 24) == 12);
}
