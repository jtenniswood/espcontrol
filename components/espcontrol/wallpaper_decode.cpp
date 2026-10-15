#include "wallpaper_decode.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <new>
#ifdef USE_ESP32
#include <esp_heap_caps.h>
#include "esphome/core/defines.h"
#endif
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
#include <csetjmp>
#include "jpeglib.h"
#endif

namespace espcontrol {
namespace {
uint8_t *allocate_pixels(size_t size) {
#ifdef USE_ESP32
  return static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
  return static_cast<uint8_t *>(malloc(size));
#endif
}
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
struct Decoder {
  jpeg_decompress_struct info{};
  jpeg_error_mgr error{};
  jmp_buf jump;
  uint8_t *pixels = nullptr;
  ~Decoder() { jpeg_destroy_decompress(&info); free(pixels); }
};
void decode_error(j_common_ptr info) {
  auto *decoder = reinterpret_cast<Decoder *>(info->client_data);
  longjmp(decoder->jump, 1);
}
void decode_message(j_common_ptr info, int level) {
  // Reject damaged/truncated images rather than accepting recovered pixels.
  if (level < 0) decode_error(info);
}
#endif
}
bool wallpaper_decode(const uint8_t *packet, size_t size, DecodedWallpaper &image) {
  image = {};
  if (!wallpaper_valid(packet, size)) return false;
  const uint32_t width = wallpaper_u32(packet + 4), height = wallpaper_u32(packet + 8);
  if (wallpaper_u32(packet) == WALLPAPER_MAGIC) {
    auto *pixels = allocate_pixels(size - WALLPAPER_HEADER_BYTES);
    if (!pixels) return false;
    std::memcpy(pixels, packet + WALLPAPER_HEADER_BYTES, size - WALLPAPER_HEADER_BYTES);
    image = {pixels, width, height, size - WALLPAPER_HEADER_BYTES, false};
    return true;
  }
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
  // Keep mutable decoder state on the heap: it must survive libjpeg longjmp.
  auto *decoder = new (std::nothrow) Decoder();
  if (!decoder) return false;
  decoder->info.err = jpeg_std_error(&decoder->error);
  decoder->error.error_exit = decode_error;
  decoder->error.emit_message = decode_message;
  decoder->info.client_data = decoder;
  if (setjmp(decoder->jump)) { delete decoder; return false; }
  jpeg_create_decompress(&decoder->info);
  jpeg_mem_src(&decoder->info, packet + WALLPAPER_HEADER_BYTES, size - WALLPAPER_HEADER_BYTES);
  if (jpeg_read_header(&decoder->info, TRUE) != JPEG_HEADER_OK || decoder->info.image_width != width ||
      decoder->info.image_height != height || decoder->info.data_precision != 8 || decoder->info.progressive_mode) {
    delete decoder; return false;
  }
  decoder->info.out_color_space = WALLPAPER_RGB888 ? JCS_EXT_BGR : JCS_RGB565;
  decoder->info.dither_mode = JDITHER_ORDERED;
  decoder->info.dct_method = JDCT_ISLOW;
  if (!jpeg_start_decompress(&decoder->info)) { delete decoder; return false; }
  const size_t stride = width * (WALLPAPER_RGB888 ? 3 : 2);
  decoder->pixels = allocate_pixels(stride * height);
  if (!decoder->pixels) { delete decoder; return false; }
  while (decoder->info.output_scanline < height) {
    JSAMPROW row = decoder->pixels + decoder->info.output_scanline * stride;
    if (jpeg_read_scanlines(&decoder->info, &row, 1) != 1) { delete decoder; return false; }
  }
  if (!jpeg_finish_decompress(&decoder->info)) { delete decoder; return false; }
  image = {decoder->pixels, width, height, stride * height, WALLPAPER_RGB888};
  decoder->pixels = nullptr;
  delete decoder;
  return true;
#else
  return false;
#endif
}
}  // namespace espcontrol
