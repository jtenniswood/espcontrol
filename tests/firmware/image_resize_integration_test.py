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

source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "scanline_resampler.h"
#include "rgb565_scaler.h"
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
struct ArtworkImage {
 int decode_buffer_width_=1, decode_buffer_height_=1;
 int decode_content_width_=1, decode_content_height_=1;
 int decode_offset_x_=0, decode_offset_y_=0;
 bool big_endian = false;
 std::vector<uint8_t> bytes = std::vector<uint8_t>(3, 0x33);
 uint8_t *decode_buffer_ = bytes.data();
 size_t resize_(int, int) { return bytes.size(); }
 int get_bpp() { return 24; }  // RGB565 with an alpha byte.
 bool is_big_endian() { return big_endian; }
 int get_position_(int x, int y) { return (y * decode_buffer_width_ + x) * 3; }
 void draw_pixel_(int x, int y, Color c) {
   assert(x >= 0 && x < decode_buffer_width_ && y >= 0 && y < decode_buffer_height_);
   const uint16_t pixel = ((c.r & 0xf8) << 8) | ((c.g & 0xfc) << 3) | (c.b >> 3);
   const int p = get_position_(x,y);
   bytes[p] = big_endian ? pixel >> 8 : pixel;
   bytes[p+1] = big_endian ? pixel : pixel >> 8;
   bytes[p+2] = c.w;
 }
};
}}
'''
source += implementation + "\n}}\n"
source += r'''
using namespace esphome::artwork_image;
struct Decoder : ImageDecoder {
 using ImageDecoder::ImageDecoder;
 int decode(uint8_t *, size_t) override { return 0; }
};
int main() {
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
 assert(allocations == 0);  // Streaming scratch is released on destruction/cancellation.
 {
   ArtworkImage image;
   Decoder decoder(&image);
   assert(decoder.set_size(2,2));
   allocation_fails = true;
   const uint8_t input[8]{};
   decoder.draw_rgb565_frame(2,2,4,input);
   assert(decoder.has_failed());
   assert(image.bytes == std::vector<uint8_t>(3,0x33));
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
