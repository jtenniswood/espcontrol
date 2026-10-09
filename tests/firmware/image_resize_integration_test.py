"""Exercise production decoder resize methods with a small host image buffer."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
component = root / "components/artwork_image"
header = re.sub(r"^#(?:include|pragma).*\n", "", (component / "image_decoder.h").read_text(), flags=re.M)
implementation = (component / "image_decoder.cpp").read_text().split("DownloadBuffer::DownloadBuffer", 1)[0]
implementation = re.sub(r"^#include.*\n", "", implementation, flags=re.M)
artwork_implementation = (component / "artwork_image.cpp").read_text()
resize_geometry = artwork_implementation.split(
    "size_t ArtworkImage::resize_(int width_in, int height_in) {", 1)[1].split("  size_t new_size", 1)[0]
fit_background = "\n".join(re.findall(
    r"^(?:void|bool) ArtworkImage::(?:set_fit_background_color|fill_fit_background_)\([^)]*\) \{\n.*?^\}",
    artwork_implementation, re.M | re.S))

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "scanline_resampler.h"
#include "rgb565_scaler.h"
#include "image_pipeline_policy.h"
#define ESP_LOGI(tag, ...) (void)(tag)
#define ESP_LOGE(tag, ...) (void)(tag)
bool allocation_fails = false;
int allocations = 0;
namespace esphome {
template<typename T> struct RAMAllocator {
 T *allocate(size_t size) {
   if (allocation_fails) return nullptr;
   ++allocations; return static_cast<T *>(calloc(size, sizeof(T)));
 }
 void deallocate(T *p, size_t) { if (p) { --allocations; free(p); } }
};
struct Color {
 uint8_t r, g, b, w;
 Color(uint8_t r, uint8_t g, uint8_t b, uint8_t w=0) : r(r),g(g),b(b),w(w) {}
};
struct Application { void feed_wdt() {} } App;
}
'''
source += header
source += r'''
namespace esphome { namespace artwork_image {
enum class ImageResizeMode { FIT, COVER };
struct ArtworkImage {
 int fixed_width_=1, fixed_height_=1;
 ImageResizeMode resize_mode_ = ImageResizeMode::FIT;
 uint32_t fit_background_color_ = 0;
 void fill_fit_background_();
 bool fill_fit_background_(uint8_t *, int, int, int, int, int, int);
 void set_fit_background_color(uint32_t color);
 int cache_invalidations = 0;
 void invalidate_lvgl_cache_() { ++cache_invalidations; }
 uint8_t *buffer_ = nullptr;
 int buffer_width_=0, buffer_height_=0;
 int buffer_content_width_=0, buffer_content_height_=0;
 int buffer_offset_x_=0, buffer_offset_y_=0;
 int decode_buffer_width_=1, decode_buffer_height_=1;
 int decode_content_width_=1, decode_content_height_=1;
 int decode_offset_x_=0, decode_offset_y_=0;
 bool big_endian = false;
 int bytes_per_pixel = 3;
 std::vector<uint8_t> bytes = std::vector<uint8_t>(3, 0x33);
 uint8_t *decode_buffer_ = bytes.data();
 bool is_auto_resize_() { return fixed_width_ == 0 || fixed_height_ == 0; }
 size_t resize_(int width_in, int height_in) {
''' + resize_geometry + r'''
   decode_buffer_width_ = width; decode_buffer_height_ = height;
   decode_content_width_ = content_width; decode_content_height_ = content_height;
   decode_offset_x_ = offset_x; decode_offset_y_ = offset_y;
   bytes.assign(static_cast<size_t>(width) * height * bytes_per_pixel, 0);
   decode_buffer_ = bytes.data();
   fill_fit_background_();
   return bytes.size();
 }
 int get_bpp() { return bytes_per_pixel * 8; }
 bool has_transparency() const { return bytes_per_pixel == 3; }
 bool is_big_endian() { return big_endian; }
 int get_position_(int x, int y) { return (y * decode_buffer_width_ + x) * bytes_per_pixel; }
 void draw_pixel_(int x, int y, Color c) {
   assert(x >= 0 && x < decode_buffer_width_ && y >= 0 && y < decode_buffer_height_);
   draw_pixel_to_buffer_(decode_buffer_, decode_buffer_width_, x, y, c);
 }
 void draw_pixel_to_buffer_(uint8_t *buffer, int width, int x, int y, Color c) {
   const uint16_t pixel = ((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | (c.b >> 3);
   const int p = (y * width + x) * bytes_per_pixel;
   buffer[p] = big_endian ? pixel >> 8 : pixel;
   buffer[p+1] = big_endian ? pixel : pixel >> 8;
   if (bytes_per_pixel == 3) buffer[p+2] = c.w;
 }
};
}}
'''
source += implementation + fit_background + "\n}}\n"
source += r'''
using namespace esphome::artwork_image;
struct Decoder : ImageDecoder {
 using ImageDecoder::ImageDecoder;
 int decode(uint8_t *, size_t) override { return 0; }
};
int main() {
 // The reported 1920x1080 snapshot on a square card must retain both edges
 // in FIT. COVER intentionally removes them. Run actual resize geometry and
 // the complete-frame decoder, with both firmware pixel byte orders.
 for (bool big_endian : {false, true}) {
  for (uint32_t background : {0u, 0x304860u}) {
  for (auto mode : {ImageResizeMode::FIT, ImageResizeMode::COVER}) {
   ArtworkImage image;
   image.big_endian = big_endian;
   image.fixed_width_ = image.fixed_height_ = 320;
   image.resize_mode_ = mode;
   image.fit_background_color_ = background;
   Decoder decoder(&image);
   assert(decoder.set_size(1920,1080));
   std::vector<uint8_t> frame(1920 * 1080 * 2);
   for (int y = 0; y < 1080; ++y) {
    for (int x = 0; x < 1920; ++x) {
     const uint16_t color = x < 120 ? 0xf800 : x >= 1800 ? 0x001f : 0x07e0;
     const size_t p = (y * 1920 + x) * 2;
     frame[p] = big_endian ? color >> 8 : color;
     frame[p+1] = big_endian ? color : color >> 8;
    }
   }
   decoder.draw_rgb565_frame(1920,1080,1920*2,frame.data());
   assert(!decoder.has_failed());
   auto pixel = [&](int x, int y) {
    const int p = image.get_position_(x,y);
    return big_endian ? (image.bytes[p] << 8) | image.bytes[p+1]
                      : image.bytes[p] | (image.bytes[p+1] << 8);
   };
   if (mode == ImageResizeMode::FIT) {
    assert(image.decode_content_width_ == 320 && image.decode_content_height_ == 180);
    assert(pixel(0,160) == 0xf800 && pixel(319,160) == 0x001f);
    const uint16_t margin = background == 0 ? 0 : 0x324c;
    assert(pixel(160,0) == margin && pixel(160,319) == margin);
    assert(image.bytes[image.get_position_(160,0) + 2] == 255);
   } else {
    assert(pixel(0,160) == 0x07e0 && pixel(319,160) == 0x07e0);
   }
   // A theme change repaints both the displayed frame and its replacement.
   // Photo pixels stay identical, and repeated refreshes do no cache work.
   const auto before = image.bytes;
   auto active = before;
   image.buffer_ = active.data();
   image.buffer_width_ = image.decode_buffer_width_;
   image.buffer_height_ = image.decode_buffer_height_;
   image.buffer_content_width_ = image.decode_content_width_;
   image.buffer_content_height_ = image.decode_content_height_;
   image.buffer_offset_x_ = image.decode_offset_x_;
   image.buffer_offset_y_ = image.decode_offset_y_;
   image.set_fit_background_color(0xffffff);
   assert(active == image.bytes);
   for (int y = 0; y < 320; ++y) {
    for (int x = 0; x < 320; ++x) {
     const int p = image.get_position_(x,y);
     if (mode == ImageResizeMode::FIT && (y < 70 || y >= 250)) {
      assert(pixel(x,y) == 0xffff && image.bytes[p+2] == 255);
     } else {
      assert(image.bytes[p] == before[p] && image.bytes[p+1] == before[p+1] &&
             image.bytes[p+2] == before[p+2]);
     }
    }
   }
   assert(image.cache_invalidations == (mode == ImageResizeMode::FIT ? 1 : 0));
   image.set_fit_background_color(0xffffff);
   assert(image.cache_invalidations == (mode == ImageResizeMode::FIT ? 1 : 0));
   image.set_fit_background_color(background);
   assert(active == before && image.bytes == before);
   assert(allocations == 0);
  }
  }
 }
 // Portrait images retain the configured surface color in both side margins.
 for (bool big_endian : {false, true}) {
   ArtworkImage image;
   image.big_endian = big_endian;
   image.fixed_width_ = image.fixed_height_ = 32;
   image.fit_background_color_ = 0x304860;
   Decoder decoder(&image);
   assert(decoder.set_size(20,40));
   std::vector<uint8_t> frame(20 * 40 * 2);
   for (size_t p = 0; p < frame.size(); p += 2) {
     frame[p] = big_endian ? 0xf8 : 0;
     frame[p+1] = big_endian ? 0 : 0xf8;
   }
   decoder.draw_rgb565_frame(20,40,40,frame.data());
   assert(!decoder.has_failed());
   for (int x : {0,16,31}) {
     const int p = image.get_position_(x,16);
     const uint16_t pixel = big_endian ? (image.bytes[p] << 8) | image.bytes[p+1]
                                      : image.bytes[p] | (image.bytes[p+1] << 8);
     assert(pixel == (x == 16 ? 0xf800 : 0x324c));
     assert(image.bytes[p+2] == 255);
   }
 }
 for (bool big_endian : {false, true}) {
   ArtworkImage image;
   image.big_endian = big_endian;
   Decoder decoder(&image);
   assert(decoder.set_size(2,2));
   // Padded RGB565 rows: black/white, white/black. Padding is poison.
   const uint8_t input[] = {0,0,255,255,0x42,0x42, 255,255,0,0,0x42,0x42};
   decoder.draw_rgb565_frame(2,2,6,input);
   assert(!decoder.has_failed());
   const uint16_t pixel = big_endian ? (image.bytes[0] << 8) | image.bytes[1]
                                    : image.bytes[0] | (image.bytes[1] << 8);
   assert(pixel == 0x8410);  // Average grey, not a selected black/white pixel.
   assert(image.bytes[2] == 255);  // JPEG output remains opaque.
   assert(allocations == 0);      // Complete-frame scratch is released immediately.
   const uint16_t native_pixels[] = {0, 65535, 65535, 0};
   assert(decoder.prepare_filtered_resize(2,2));
   decoder.draw_fast_filtered_rgb565_row(0, native_pixels);
   decoder.draw_fast_filtered_rgb565_row(1, native_pixels + 2);
   assert(image.bytes[0] == (big_endian ? 0x84 : 0x10));
   assert(image.bytes[1] == (big_endian ? 0x10 : 0x84));
   assert(image.bytes[2] == 255);  // Native GIF canvas input also stays opaque.
   uint8_t colors[8];
   for (int i = 0; i < 4; i++) {
     const uint16_t color = i < 2 ? 0xf800 : 0x001f;  // Red and blue rows.
     colors[i*2] = big_endian ? color >> 8 : color;
     colors[i*2+1] = big_endian ? color : color >> 8;
   }
   decoder.draw_rgb565_frame(2,2,4,colors);
   const uint16_t mixed = big_endian ? (image.bytes[0] << 8) | image.bytes[1]
                                    : image.bytes[0] | (image.bytes[1] << 8);
   assert(mixed == 0x8010 && image.bytes[2] == 255);
 }
 {
   ArtworkImage image;
   Decoder decoder(&image);
   assert(decoder.set_size(2,2));
   assert(decoder.prepare_filtered_resize(2,2));
   const uint8_t red[] = {255,0,0, 255,0,0};
   decoder.draw_filtered_rgb888_row(0,red);
   decoder.draw_filtered_rgb888_row(1,red);
   assert(image.bytes[0] == 0 && image.bytes[1] == 0xf8 && image.bytes[2] == 255);
 }
 for (bool big_endian : {false, true}) {
   ArtworkImage image;
   image.big_endian = big_endian; image.bytes_per_pixel = 2;
   image.bytes.assign(2, 0x33); image.decode_buffer_ = image.bytes.data();
   Decoder decoder(&image);
   assert(decoder.set_size(2,2) && decoder.prepare_filtered_resize(2,2));
   const uint16_t pixels[] = {0, 65535, 65535, 0};
   decoder.draw_fast_filtered_rgb565_row(0, pixels);
   decoder.draw_fast_filtered_rgb565_row(1, pixels + 2);
   assert(image.bytes[0] == (big_endian ? 0x84 : 0x10));
   assert(image.bytes[1] == (big_endian ? 0x10 : 0x84));
 }
 // The production writer must yield when a single source row covers a display.
 // Test both byte orders and the general alpha path as well as opaque RGB565.
 for (bool big_endian : {false, true}) for (int bytes_per_pixel : {2, 3}) {
   ArtworkImage image;
   image.big_endian = big_endian; image.bytes_per_pixel = bytes_per_pixel;
   image.fixed_width_ = 1280; image.fixed_height_ = 800;
   image.resize_mode_ = ImageResizeMode::COVER;
   Decoder decoder(&image);
   assert(decoder.set_size(1,1) && decoder.prepare_filtered_resize(1,1));
   const uint16_t red[] = {0xf800};
   auto result = decoder.draw_fast_filtered_rgb565_row(0, red, 2);
   assert(result == ScanlineResampler::RowResult::MORE);
   assert(image.bytes[image.get_position_(0,2)] == 0 && image.bytes[image.get_position_(0,2)+1] == 0);
   int batches = 1;
   while (result == ScanlineResampler::RowResult::MORE) {
     result = decoder.draw_fast_filtered_rgb565_row(0, red, 2); ++batches;
   }
   assert(result == ScanlineResampler::RowResult::DONE && batches == 400 && !decoder.has_failed());
   for (int y = 0; y < 800; ++y) for (int x = 0; x < 1280; ++x) {
     const auto pos = image.get_position_(x,y);
     assert(image.bytes[pos] == (big_endian ? 0xf8 : 0));
     assert(image.bytes[pos+1] == (big_endian ? 0 : 0xf8));
     if (bytes_per_pixel == 3) assert(image.bytes[pos+2] == 255);
   }
 }
 assert(allocations == 0);  // Streaming scratch is released on destruction/cancellation.
 {
   ArtworkImage image;
   Decoder decoder(&image);
   assert(decoder.set_size(2,2));
   allocation_fails = true;
   const uint8_t input[8]{};
   decoder.draw_rgb565_frame(2,2,4,input);
   assert(decoder.has_failed());
   assert(image.bytes == std::vector<uint8_t>(3,0));
 }
 assert(allocations == 0);
}
'''

with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / "test.cpp"
    binary = Path(tmp) / "test"
    cpp.write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17",
                    "-Wall", "-Wextra", "-Werror", "-I", str(component), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
