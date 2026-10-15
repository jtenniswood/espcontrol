#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include "wallpaper_decode.h"
using namespace espcontrol;
bool allocation_fails = false;
void *heap_caps_malloc(size_t size, int) { return allocation_fails ? nullptr : malloc(size); }
std::vector<uint8_t> jpeg_packet(const char *path, unsigned width, unsigned height) {
  std::ifstream input(path, std::ios::binary);
  std::vector<uint8_t> jpeg{std::istreambuf_iterator<char>(input), {}};
  assert(jpeg.size() > 100);
  std::vector<uint8_t> packet(16);
  packet.insert(packet.end(), jpeg.begin(), jpeg.end());
  unsigned values[] = {WALLPAPER_JPEG_MAGIC, width, height, unsigned(jpeg.size())};
  for (int j = 0; j < 4; ++j) for (int i = 0; i < 4; ++i) packet[4 * j + i] = values[j] >> (8 * i);
  return packet;
}
void put(std::vector<uint8_t> &data, size_t offset, uint32_t value) {
  for (int i = 0; i < 4; ++i) data[offset + i] = value >> (i * 8);
}
int main() {
  auto packet = jpeg_packet(WALLPAPER_TEST_JPEG, 32, 16);
  DecodedWallpaper image;
  assert(wallpaper_valid(packet.data(), packet.size()));
  assert(wallpaper_decode(packet.data(), packet.size(), image));
  assert(image.width == 32 && image.height == 16 && image.rgb888 == WALLPAPER_RGB888);
  assert(image.size == 32 * 16 * (WALLPAPER_RGB888 ? 3 : 2));
  if (image.rgb888) {
    assert(abs(int(image.pixels[0]) - 53) <= 3);  // LVGL RGB888 is BGR byte order.
    assert(abs(int(image.pixels[1]) - 109) <= 3);
    assert(abs(int(image.pixels[2]) - 227) <= 3);
  } else {
    const unsigned rgb = image.pixels[0] | unsigned(image.pixels[1]) << 8;
    assert(abs(int((rgb >> 11) * 255 / 31) - 227) <= 18);
  }
  free(image.pixels);
  allocation_fails = true;
  assert(!wallpaper_decode(packet.data(), packet.size(), image) && !image.pixels);
  allocation_fails = false;
  auto large = jpeg_packet(WALLPAPER_TEST_LARGE_JPEG, 1280, 800);
  assert(wallpaper_decode(large.data(), large.size(), image) == WALLPAPER_RGB888);
  if (WALLPAPER_RGB888) { assert(image.size == 1280 * 800 * 3); free(image.pixels); }
  put(large, 8, 801); assert(!wallpaper_valid(large.data(), large.size()));
  // Progressive and high-precision frames are outside the bounded upload format.
  auto progressive = packet;
  for (size_t i = 16; i + 1 < progressive.size(); ++i)
    if (progressive[i] == 0xff && progressive[i + 1] == 0xc0) { progressive[i + 1] = 0xc2; break; }
  assert(!wallpaper_decode(progressive.data(), progressive.size(), image));
  put(packet, 4, 33); assert(!wallpaper_decode(packet.data(), packet.size(), image));
  put(packet, 4, UINT32_MAX); assert(!wallpaper_valid(packet.data(), packet.size()));
  put(packet, 4, 32);
  packet.erase(packet.end() - 20, packet.end() - 2); // Keep EOI but truncate entropy.
  put(packet, 12, packet.size() - 16);
  assert(!wallpaper_decode(packet.data(), packet.size(), image));
  packet.assign(18, 0);
  put(packet, 0, WALLPAPER_MAGIC); put(packet, 4, 1); put(packet, 8, 1); put(packet, 12, 2);
  packet[17] = 0xf8;
  assert(wallpaper_decode(packet.data(), packet.size(), image));
  assert(!image.rgb888 && image.pixels[0] == 0 && image.pixels[1] == 0xf8);
  free(image.pixels);
}
