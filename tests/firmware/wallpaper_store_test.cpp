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
static bool present = true, allocation_fails = false, erase_fails = false;
static int saved_layout = -1, pending_layout = -1;
static std::vector<size_t> erased_offsets, written_offsets;
static int writes_until_failure = -1;
static int saved_background = -1, pending_background = -1, background_writes = 0;
static int saved_pair = -1, pending_pair = -1;
static int saved_opacity = -1, pending_opacity = -1, opacity_writes = 0;
static bool nvs_fails_open = false, nvs_fails_write = false, nvs_fails_commit = false;
esp_err_t nvs_open(const char *name, int, nvs_handle_t *handle) {
  assert(std::strcmp(name, "espcontrol_wall") == 0);
  *handle = 1; return nvs_fails_open ? -1 : ESP_OK;
}
esp_err_t nvs_get_u8(nvs_handle_t, const char *key, uint8_t *value) {
  if (std::strcmp(key, "layout") == 0) {
    if (saved_layout < 0) return -1;
    *value = saved_layout; return ESP_OK;
  }
  assert(std::strcmp(key, "opacity") == 0);
  if (saved_opacity < 0) return -1;
  *value = saved_opacity; return ESP_OK;
}
esp_err_t nvs_set_u8(nvs_handle_t, const char *key, uint8_t value) {
  if (nvs_fails_write) return -1;
  if (std::strcmp(key, "layout") == 0) { pending_layout = value; return ESP_OK; }
  pending_opacity = value; ++opacity_writes; return ESP_OK;
}
esp_err_t nvs_get_u16(nvs_handle_t, const char *key, uint16_t *value) {
  assert(std::strcmp(key, "opacities") == 0);
  if (saved_pair < 0) return -1;
  *value = saved_pair; return ESP_OK;
}
esp_err_t nvs_set_u16(nvs_handle_t, const char *key, uint16_t value) {
  assert(std::strcmp(key, "opacities") == 0);
  if (nvs_fails_write) return -1;
  pending_pair = value; ++opacity_writes; return ESP_OK;
}
esp_err_t nvs_get_u32(nvs_handle_t, const char *key, uint32_t *value) {
  assert(std::strcmp(key, "background") == 0);
  if (saved_background < 0) return -1;
  *value = saved_background; return ESP_OK;
}
esp_err_t nvs_set_u32(nvs_handle_t, const char *key, uint32_t value) {
  assert(std::strcmp(key, "background") == 0);
  if (nvs_fails_write) return -1;
  pending_background = value; ++background_writes; return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t) {
  if (nvs_fails_commit) return -1;
  if (pending_opacity >= 0) saved_opacity = pending_opacity;
  if (pending_pair >= 0) saved_pair = pending_pair;
  if (pending_layout >= 0) saved_layout = pending_layout;
  if (pending_background >= 0) saved_background = pending_background;
  return ESP_OK;
}
void nvs_close(nvs_handle_t) { pending_opacity = pending_pair = pending_layout = pending_background = -1; }
const esp_partition_t *esp_partition_find_first(int, int, const char *name) {
  assert(std::strcmp(name, "card_images") == 0);
  return present ? &partition : nullptr;
}
esp_err_t esp_partition_read(const esp_partition_t *, size_t offset, void *data, size_t size) {
  assert(offset + size <= flash.size()); std::memcpy(data, flash.data() + offset, size); return ESP_OK;
}
esp_err_t esp_partition_write(const esp_partition_t *, size_t offset, const void *data, size_t size) {
  assert(offset + size <= flash.size());
  written_offsets.push_back(offset);
  if (writes_until_failure == 0) return -1;
  if (writes_until_failure > 0) --writes_until_failure;
  const auto *bytes = static_cast<const uint8_t *>(data);
  for (size_t i = 0; i < size; ++i) { assert((flash[offset + i] & bytes[i]) == bytes[i]); flash[offset + i] &= bytes[i]; }
  return ESP_OK;
}
esp_err_t esp_partition_erase_range(const esp_partition_t *, size_t offset, size_t size) {
  assert(offset >= WALLPAPER_STORAGE_OFFSET && offset + size <= WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES * 2);
  assert(offset % 4096 == 0 && size % 4096 == 0);
  if (erase_fails) return -1;
  erased_offsets.push_back(offset);
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
  present = true;
  saved_opacity = 35;
  WallpaperStore migration;
  assert(migration.opacities().off == 35 && migration.opacities().active == 35);
  assert(migration.save_opacities(40, 90));
  WallpaperStore split_reboot;
  assert(split_reboot.opacities().off == 40 && split_reboot.opacities().active == 90);
  assert(split_reboot.save_opacity_percent(60));  // Original clients still set both.
  assert(split_reboot.opacities().off == 60 && split_reboot.opacities().active == 60);
  saved_opacity = saved_pair = -1;
  WallpaperStore store; assert(store.available());
  const auto first = image(1), second = image(2);
  assert(store.save(first.data(), first.size()));
  expect_image(store, first);
  assert(store.opacity_percent() == 80);
  assert(store.opacities().active == 98);
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
  assert(store.opacity_percent() == 50 && saved_pair == (50 | (50 << 8)));
  const int writes = opacity_writes;
  assert(store.save_opacity_percent(50) && opacity_writes == writes);
  assert(store.save_opacity_percent(0) && store.opacity_percent() == 0);
  assert(store.save_opacity_percent(100) && store.opacity_percent() == 100);
  saved_opacity = 200; saved_pair = (101 << 8);
  WallpaperStore invalid_opacity; assert(invalid_opacity.opacity_percent() == 80);
  assert(invalid_opacity.opacities().active == 98);
  saved_opacity = 50; saved_pair = -1;
  const auto unchanged_image = flash;
  assert(!store.save_opacities(101, 50) && !store.save_opacities(50, -1));
  assert(store.save_opacities(25, 85));
  assert(store.opacities().off == 25 && store.opacities().active == 85);
  WallpaperStore independent; assert(independent.opacities().off == 25 && independent.opacities().active == 85);
  nvs_fails_commit = true; assert(!store.save_opacities(75, 10)); nvs_fails_commit = false;
  assert(store.opacities().off == 25 && store.opacities().active == 85);
  WallpaperStore after_failure; assert(after_failure.opacities().off == 25 && after_failure.opacities().active == 85);
  assert(flash == unchanged_image && store.revision() == wallpaper_revision);
  assert(store.save_opacities(0, 100));
  assert(store.opacities().off == 0 && store.opacities().active == 100);
  assert(store.save_opacity_percent(50));
  // Background settings survive reboot without rewriting the image slot.
  assert(!store.background().colour && store.background().rgb == 0x212121);
  assert(store.save_background(true, 0x3C64DE));
  assert(store.background().colour && store.background().rgb == 0x3C64DE);
  WallpaperStore colour_reboot;
  assert(colour_reboot.background().colour && colour_reboot.background().rgb == 0x3C64DE);
  expect_image(colour_reboot, first);
  assert(store.revision() == wallpaper_revision && flash == unchanged_image);
  assert(!store.save_background(true, 0x1000000));
  nvs_fails_open = true; assert(!store.save_background(false, 0xABCDEF)); nvs_fails_open = false;
  nvs_fails_write = true; assert(!store.save_background(false, 0xABCDEF)); nvs_fails_write = false;
  nvs_fails_commit = true; assert(!store.save_background(false, 0xABCDEF)); nvs_fails_commit = false;
  assert(store.background().colour && store.background().rgb == 0x3C64DE);
  WallpaperStore failed_colour_reboot;
  assert(failed_colour_reboot.background().colour && failed_colour_reboot.background().rgb == 0x3C64DE);
  const auto background_write_count = background_writes;
  assert(store.save_background(true, 0x3C64DE) && background_writes == background_write_count);
  assert(store.save_background(false, 0x3C64DE));
  expect_image(store, first);
  assert(!store.background().colour && flash == unchanged_image);
  saved_background = 0x2000000;
  WallpaperStore invalid_background;
  assert(!invalid_background.background().colour && invalid_background.background().rgb == 0x212121);
  saved_background = -1;
  present = false;
  WallpaperStore colour_without_partition;
  assert(!colour_without_partition.available());
  assert(colour_without_partition.save_background(true, 0x123456));
  assert(colour_without_partition.save_opacities(80, 98));
  WallpaperStore no_partition_reboot;
  assert(no_partition_reboot.background().colour && no_partition_reboot.background().rgb == 0x123456);
  assert(no_partition_reboot.opacities().off == 80 && no_partition_reboot.opacities().active == 98);
  assert(no_partition_reboot.save_opacity_percent(50));
  present = true; saved_background = -1;
  // Once erasure succeeds, failures cannot retain the old image in one slot.
  for (int boundary = 0; boundary < 3; ++boundary) {
    writes_until_failure = -1; assert(store.save(first.data(), first.size()));
    const auto before = store.revision();
    writes_until_failure = boundary;
    assert(!store.save(second.data(), second.size()));
    size_t size; uint32_t revision;
    assert(!store.load(size, revision) && size == 0 && revision != before);
    WallpaperStore reboot; assert(!reboot.load(size, revision) && size == 0);
  }
  writes_until_failure = -1;
  assert(store.save(second.data(), second.size()));
  erase_fails = true;
  assert(!store.save(first.data(), first.size()));
  expect_image(store, second); // Failure before erase leaves existing bytes intact.
  erase_fails = false;
  WallpaperStore reboot; expect_image(reboot, second);
  // CRC rejects a damaged sole copy, instead of returning corrupt pixels.
  flash[WALLPAPER_STORAGE_OFFSET + 16 + 16] ^= 1;
  WallpaperStore corrupt; size_t damaged_size; uint32_t damaged_revision;
  assert(!corrupt.load(damaged_size, damaged_revision) && damaged_size == 0);
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
    writes_until_failure = -1; assert(removed.save(first.data(), first.size()));
    writes_until_failure = boundary;
    assert(!removed.save(compressed.data(), compressed.size()));
    WallpaperStore failed_jpeg; assert(!failed_jpeg.load(size, revision) && size == 0);
  }
  writes_until_failure = -1;
  assert(removed.save(compressed.data(), compressed.size()));
  WallpaperStore jpeg_reboot; expect_image(jpeg_reboot, compressed);
  assert(jpeg_reboot.opacity_percent() == 50);
  // Normal operation never reserves, erases or writes a second image slot.
  for (auto offset : erased_offsets) assert(offset == WALLPAPER_STORAGE_OFFSET);
  for (auto offset : written_offsets) assert(offset >= WALLPAPER_STORAGE_OFFSET &&
      offset < WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_BYTES);
  const size_t backup = WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_SLOT_BYTES;
  auto seed = [&](int slot, unsigned revision_value, const std::vector<uint8_t> &payload) {
    const size_t offset = WALLPAPER_STORAGE_OFFSET + slot * WALLPAPER_STORAGE_SLOT_BYTES;
    std::fill(flash.begin() + offset, flash.begin() + offset + WALLPAPER_STORAGE_SLOT_BYTES, 0xff);
    unsigned record[] = {0x31535057, revision_value, unsigned(payload.size()),
      payload.empty() ? 0 : esp_rom_crc32_le(0, payload.data(), payload.size())};
    std::memcpy(flash.data() + offset, record, 16);
    if (!payload.empty()) std::memcpy(flash.data() + offset + 16, payload.data(), payload.size());
  };
  saved_layout = -1; seed(0, 4, first); seed(1, 5, compressed);
  WallpaperStore migrated; expect_image(migrated, compressed);
  assert(saved_layout == 1);
  assert(std::all_of(flash.begin() + backup, flash.begin() + backup + WALLPAPER_STORAGE_SLOT_BYTES,
      [](uint8_t b) { return b == 0xff; }));
  WallpaperStore migrated_reboot; expect_image(migrated_reboot, compressed);
  // Released storage can be reused, even if it happens to resemble an old record.
  seed(1, 100, first);
  const auto reused_region = std::vector<uint8_t>(flash.begin() + backup,
      flash.begin() + backup + WALLPAPER_STORAGE_SLOT_BYTES);
  WallpaperStore reused; expect_image(reused, compressed);
  assert(reused.save(second.data(), second.size()));
  assert(std::equal(reused_region.begin(), reused_region.end(), flash.begin() + backup));
  // A newest first-slot image migrates without rewriting its sole current copy.
  saved_layout = -1; seed(0, 9, second); seed(1, 8, first);
  const auto primary_before = std::vector<uint8_t>(flash.begin() + WALLPAPER_STORAGE_OFFSET, flash.begin() + backup);
  WallpaperStore first_slot; expect_image(first_slot, second);
  assert(std::equal(primary_before.begin(), primary_before.end(), flash.begin() + WALLPAPER_STORAGE_OFFSET));
  // Failed migration retains the legacy source and can be retried after restart.
  for (int boundary = 0; boundary < 3; ++boundary) {
    saved_layout = -1; seed(0, 1, first); seed(1, 2, compressed);
    writes_until_failure = boundary;
    WallpaperStore migration_failed; assert(!migration_failed.available());
    assert(saved_layout < 0);
    writes_until_failure = -1;
    WallpaperStore migration_retry; expect_image(migration_retry, compressed);
  }
  // A failed layout commit keeps the old region intact until migration retries.
  saved_layout = -1; seed(0, 1, first); seed(1, 2, compressed);
  const auto legacy_source = std::vector<uint8_t>(flash.begin() + backup,
      flash.begin() + backup + WALLPAPER_STORAGE_SLOT_BYTES);
  nvs_fails_commit = true;
  WallpaperStore layout_failed; assert(!layout_failed.available());
  assert(saved_layout < 0 && std::equal(legacy_source.begin(), legacy_source.end(), flash.begin() + backup));
  nvs_fails_commit = false;
  WallpaperStore layout_retry; expect_image(layout_retry, compressed);
  // Cleanup failure leaves unused bytes, but the released region stays ignored.
  saved_layout = -1; seed(0, 3, second); seed(1, 2, first);
  erase_fails = true;
  WallpaperStore cleanup_failed; expect_image(cleanup_failed, second);
  assert(saved_layout == 1);
  erase_fails = false;
  WallpaperStore cleanup_reboot; expect_image(cleanup_reboot, second);
  // Legacy removal must not resurrect the older image during migration.
  saved_layout = -1; seed(0, 3, first); seed(1, 4, {});
  WallpaperStore migrated_removal; assert(!migrated_removal.load(size, revision) && size == 0);
  // Resetting preferences clears the saved setting and restores the default.
  saved_opacity = saved_pair = -1;
  WallpaperStore reset; assert(reset.opacities().off == 80 && reset.opacities().active == 98);
  assert(std::all_of(flash.begin(), flash.begin() + WALLPAPER_STORAGE_OFFSET, [](uint8_t b) { return b == 0xff; }));
  assert(std::all_of(flash.begin() + WALLPAPER_STORAGE_OFFSET + WALLPAPER_STORAGE_BYTES, flash.end(), [](uint8_t b) { return b == 0xff; }));
}
