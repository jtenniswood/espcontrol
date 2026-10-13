#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "esp_partition.h"
#include "wallpaper.h"
#include "panel_flash_layout.h"
using namespace espcontrol;
static std::vector<uint8_t> flash(PANEL_DATA_PARTITION_BYTES, 0xff);
static esp_partition_t partition{PANEL_DATA_PARTITION_BYTES, 0x40, false};
static bool present = true, allocation_fails = false;
static int writes_until_failure = -1;
const esp_partition_t *esp_partition_find_first(int, int, const char *name) {
  assert(std::strcmp(name, "card_images") == 0);
  return present ? &partition : nullptr;
}
esp_err_t esp_partition_read(const esp_partition_t *, size_t offset, void *data, size_t size) {
  assert(offset + size <= flash.size()); std::memcpy(data, flash.data() + offset, size); return ESP_OK;
}
esp_err_t esp_partition_write(const esp_partition_t *, size_t offset, const void *data, size_t size) {
  assert(offset + size <= flash.size());
  if (writes_until_failure == 0) return -1;
  if (writes_until_failure > 0) --writes_until_failure;
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0; i < size; ++i) { assert((flash[offset + i] & bytes[i]) == bytes[i]); flash[offset + i] &= bytes[i]; }
  return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t offset, size_t size) {
  assert(offset >= WALLPAPER_STORAGE_OFFSET && offset + size <= WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES * 2);
  assert(offset % 4096 == 0 && size % 4096 == 0);
  std::fill(flash.begin() + offset, flash.begin() + offset + size, 0xff); return ESP_OK;
}
void *heap_caps_malloc(size_t size, int) { return allocation_fails ? nullptr : std::malloc(size); }
uint32_t esp_rom_crc32_le(uint32_t seed, const uint8_t *data, size_t size) {
  uint32_t crc = ~seed;
  for (size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
  }
  return ~crc;
}
std::vector<uint8_t> image(uint8_t pixel) {
  return {0x45, 0x57, 0x50, 0x31, 1, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, pixel, pixel};
}
void expect_image(WallpaperStore &store, const std::vector<uint8_t> &expected) {
  size_t size; uint32_t revision;
  auto *data = store.load(size, revision);
  assert(data && size == expected.size() && std::memcmp(data, expected.data(), size) == 0);
  std::free(data);
}
int main() {
  present = false; WallpaperStore unsupported; assert(!unsupported.available());
  present = true; WallpaperStore store; assert(store.available());
  const auto first = image(1), second = image(2);
  assert(store.save(first.data(), first.size()));
  expect_image(store, first);
  // Simulate a failed write at each durability boundary; old slot survives reboot.
  for (int boundary = 0; boundary < 3; ++boundary) {
    writes_until_failure = boundary;
    assert(!store.save(second.data(), second.size()));
    WallpaperStore reboot; expect_image(reboot, first);
  }
  writes_until_failure = -1;
  assert(store.save(second.data(), second.size()));
  WallpaperStore reboot; expect_image(reboot, second);
  // Corrupt the new payload: CRC rejects it and the old slot is retained.
  flash[WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES + 16 + 16] ^= 1;
  WallpaperStore corrupt; expect_image(corrupt, first);
  assert(corrupt.save(nullptr, 0));
  WallpaperStore removed; size_t size; uint32_t revision;
  assert(removed.available()); assert(!removed.load(size, revision) && size == 0);
  assert(removed.save(first.data(), first.size()));
  allocation_fails = true;
  assert(!removed.load(size, revision) && size == first.size());
  allocation_fails = false;
  expect_image(removed, first);
  assert(std::all_of(flash.begin(), flash.begin() + WALLPAPER_STORAGE_OFFSET, [](uint8_t b) { return b == 0xff; }));
  assert(std::all_of(flash.begin() + WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES * 2, flash.end(), [](uint8_t b) { return b == 0xff; }));
}
