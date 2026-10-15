#include <cassert>
#include <vector>
#include "wallpaper_format.h"
#include "panel_flash_layout.h"
using namespace espcontrol;
void put(std::vector<uint8_t> &data, size_t offset, uint32_t value) {
  for (int i = 0; i < 4; ++i) data[offset + i] = value >> (i * 8);
}
int main() {
  assert(wallpaper_opacity_byte(0) == 0);
  assert(wallpaper_opacity_byte(50) == 128);
  assert(wallpaper_opacity_byte(80) == 204);
  assert(wallpaper_opacity_byte(100) == 255);
  std::vector<uint8_t> data(16 + 480 * 320 * 2);
  put(data, 0, WALLPAPER_MAGIC); put(data, 4, 480); put(data, 8, 320); put(data, 12, 480 * 320 * 2);
  assert(wallpaper_valid(data.data(), data.size()));
  assert(!wallpaper_valid(data.data(), data.size() - 1));
  assert(!wallpaper_valid(nullptr, data.size()));
  put(data, 4, 481); assert(!wallpaper_valid(data.data(), data.size()));
  put(data, 4, UINT32_MAX); assert(!wallpaper_valid(data.data(), data.size()));
  data.resize(16);
  put(data, 4, 0); put(data, 8, 0); put(data, 12, 0);
  assert(wallpaper_is_removal(data.data(), data.size()));
  assert(!wallpaper_valid(data.data(), data.size()));
  put(data, 8, 1); assert(!wallpaper_is_removal(data.data(), data.size()));
  assert(2 * (44 * 1024) <= WALLPAPER_STORAGE_OFFSET);
  assert(WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_BYTES < panel_config_partition_bytes(PANEL_DATA_PARTITION_BYTES));
  assert(WALLPAPER_STORAGE_BYTES == 512 * 1024);
  assert(16 + WALLPAPER_MAX_BYTES <= WALLPAPER_STORAGE_SLOT_BYTES);
}
