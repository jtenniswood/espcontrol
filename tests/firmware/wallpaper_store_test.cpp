#include <cassert>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <fstream>
#include <iterator>
#include "esp_partition.h"
#include "nvs.h"
#include "wallpaper.h"
#include "panel_flash_layout.h"
using namespace espcontrol;
static std::vector<uint8_t> flash(PANEL_DATA_PARTITION_BYTES, 0xff);
static esp_partition_t partition{PANEL_DATA_PARTITION_BYTES, 0x40, false};
static bool present = true, allocation_fails = false;
static int writes_until_failure = -1;
static int saved_opacity = -1, pending_opacity = -1, opacity_writes = 0;
static bool nvs_fails_open = false, nvs_fails_write = false, nvs_fails_commit = false;
esp_err_t nvs_open(const char *name, int, nvs_handle_t *handle) {
  assert(std::strcmp(name, "espcontrol_wall") == 0);
  *handle = 1; return nvs_fails_open ? -1 : ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t, const char *key, uint8_t *value) {
  assert(std::strcmp(key, "opacity") == 0);
  if (saved_opacity < 0) return -1;
  *value = saved_opacity; return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t, const char *, uint8_t value) {
  if (nvs_fails_write) return -1;
  pending_opacity = value; ++opacity_writes; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (nvs_fails_commit) return -1;
  saved_opacity = pending_opacity; return ESP_OK;
}
void nvs_close(nvs_handle_t) { pending_opacity = -1; }
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
  assert(store.opacity_percent() == 80);
  const auto wallpaper_revision = store.revision();
  const auto old_flash = flash;
  assert(!store.save_opacity_percent(-1) && !store.save_opacity_percent(101));
  assert(store.save_opacity_percent(50));
  assert(store.opacity_percent() == 50 && store.revision() == wallpaper_revision && old_flash == flash);
  WallpaperStore opacity_reboot; assert(opacity_reboot.opacity_percent() == 50);
  expect_image(opacity_reboot, first);
  nvs_fails_open = true; assert(!store.save_opacity_percent(60)); nvs_fails_open = false;
  nvs_fails_write = true; assert(!store.save_opacity_percent(60)); nvs_fails_write = false;
  nvs_fails_commit = true; assert(!store.save_opacity_percent(60)); nvs_fails_commit = false;
  assert(store.opacity_percent() == 50 && saved_opacity == 50);
  const int writes = opacity_writes;
  assert(store.save_opacity_percent(50) && opacity_writes == writes);
  assert(store.save_opacity_percent(0) && store.opacity_percent() == 0);
  assert(store.save_opacity_percent(100) && store.opacity_percent() == 100);
  saved_opacity = 200;
  WallpaperStore invalid_opacity; assert(invalid_opacity.opacity_percent() == 80);
  saved_opacity = 50;
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
  assert(removed.opacity_percent() == 50);
  assert(removed.save(first.data(), first.size()));
  allocation_fails = true;
  assert(!removed.load(size, revision) && size == first.size());
  allocation_fails = false;
  expect_image(removed, first);
  std::ifstream input(WALLPAPER_TEST_JPEG, std::ios::binary);
  std::vector<uint8_t> jpeg{std::istreambuf_iterator<char>(input), {}};
  std::vector<uint8_t> compressed(16);
  compressed.insert(compressed.end(), jpeg.begin(), jpeg.end());
  unsigned values[] = {WALLPAPER_JPEG_MAGIC, 32, 16, unsigned(jpeg.size())};
  for (int j = 0; j < 4; ++j) for (int i = 0; i < 4; ++i) compressed[4 * j + i] = values[j] >> (i * 8);
  for (int boundary = 0; boundary < 3; ++boundary) {
    writes_until_failure = boundary;
    assert(!removed.save(compressed.data(), compressed.size()));
    WallpaperStore failed_jpeg; expect_image(failed_jpeg, first);
  }
  writes_until_failure = -1;
  assert(removed.save(compressed.data(), compressed.size()));
  WallpaperStore jpeg_reboot; expect_image(jpeg_reboot, compressed);
  assert(jpeg_reboot.opacity_percent() == 50);
  // Resetting preferences clears the saved setting and restores the default.
  saved_opacity = -1;
  WallpaperStore reset; assert(reset.opacity_percent() == 80);
  assert(std::all_of(flash.begin(), flash.begin() + WALLPAPER_STORAGE_OFFSET, [](uint8_t b) { return b == 0xff; }));
  assert(std::all_of(flash.begin() + WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES * 2, flash.end(), [](uint8_t b) { return b == 0xff; }));
}
