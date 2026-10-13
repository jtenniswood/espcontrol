#include "wallpaper.h"
#include "panel_flash_layout.h"
#ifdef USE_ESP32
#include <esp_partition.h>
#include <esp_heap_caps.h>
#include <esp_rom_crc.h>
#include <nvs.h>
#endif
#include <cstdlib>

namespace espcontrol {
namespace {
// Configuration occupies two 44 KiB slots at the front; identity uses the
// final 16 KiB. Keep a separate pair of atomic 512 KiB wallpaper slots.
constexpr size_t OFFSET = WALLPAPER_STORAGE_OFFSET;
constexpr size_t STRIDE = WALLPAPER_STORAGE_SLOT_BYTES;
constexpr uint32_t MARKER = 0x31535057;
struct Record { uint32_t marker, revision, size, crc; };
static_assert(sizeof(Record) + WALLPAPER_MAX_BYTES <= STRIDE);
#ifdef USE_ESP32
uint8_t *allocate(size_t size) {
  return static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
}
#endif
}
WallpaperStore &wallpaper_store() { static WallpaperStore store; return store; }
bool WallpaperStore::begin_locked() {
#ifdef USE_ESP32
  if (initialized_) return partition_ != nullptr;
  initialized_ = true;
  auto *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "card_images");
  if (!partition || partition->subtype != 0x40 || partition->encrypted ||
      panel_config_partition_bytes(partition->size) < OFFSET + STRIDE * 2) return false;
  partition_ = partition;
  nvs_handle_t settings;
  if (nvs_open("espcontrol_wall", NVS_READWRITE, &settings) == ESP_OK) {
    uint8_t opacity = WALLPAPER_DEFAULT_OPACITY_PERCENT;
    if (nvs_get_u8(settings, "opacity", &opacity) == ESP_OK && opacity <= 100)
      opacity_percent_ = opacity;
    nvs_close(settings);
  }
  for (int slot = 0; slot < 2; ++slot) {
    Record record{};
    if (esp_partition_read(partition, OFFSET + slot * STRIDE, &record, sizeof(record)) != ESP_OK ||
        record.marker != MARKER || record.size > WALLPAPER_MAX_BYTES) continue;
    bool valid = record.size == 0 && record.crc == 0;
    if (record.size) {
      auto *data = allocate(record.size);
      if (!data) { initialized_ = false; partition_ = nullptr; return false; }
      valid = esp_partition_read(partition, OFFSET + slot * STRIDE + sizeof(record), data, record.size) == ESP_OK &&
              wallpaper_valid(data, record.size) && esp_rom_crc32_le(0, data, record.size) == record.crc;
      free(data);
    }
    if (valid && (slot_ < 0 || static_cast<int32_t>(record.revision - revision_) > 0)) {
      slot_ = slot; revision_ = record.revision; size_ = record.size;
    }
  }
  return true;
#else
  return false;
#endif
}
bool WallpaperStore::available() { std::lock_guard<std::mutex> lock(mutex_); return begin_locked(); }
uint32_t WallpaperStore::revision() { std::lock_guard<std::mutex> lock(mutex_); begin_locked(); return revision_; }
uint8_t WallpaperStore::opacity_percent() {
  std::lock_guard<std::mutex> lock(mutex_);
  begin_locked();
  return opacity_percent_;
}
bool WallpaperStore::save_opacity_percent(int percent) {
  if (percent < 0 || percent > 100) return false;
  std::lock_guard<std::mutex> lock(mutex_);
#ifdef USE_ESP32
  if (!begin_locked()) return false;
  if (percent == opacity_percent_) return true;
  nvs_handle_t settings;
  if (nvs_open("espcontrol_wall", NVS_READWRITE, &settings) != ESP_OK) return false;
  auto result = nvs_set_u8(settings, "opacity", static_cast<uint8_t>(percent));
  if (result == ESP_OK) result = nvs_commit(settings);
  nvs_close(settings);
  if (result != ESP_OK) return false;
  opacity_percent_ = static_cast<uint8_t>(percent);
  return true;
#else
  return false;
#endif
}
bool WallpaperStore::save(const uint8_t *data, size_t size) {
  if (size && !wallpaper_valid(data, size)) return false;
  std::lock_guard<std::mutex> lock(mutex_);
#ifdef USE_ESP32
  if (!begin_locked()) return false;
  const auto *partition = static_cast<const esp_partition_t *>(partition_);
  const int slot = slot_ == 0 ? 1 : 0;
  const size_t offset = OFFSET + slot * STRIDE;
  Record record{MARKER, revision_ + 1, static_cast<uint32_t>(size), size ? esp_rom_crc32_le(0, data, size) : 0};
  if (esp_partition_erase_range(partition, offset, STRIDE) != ESP_OK ||
      (size && esp_partition_write(partition, offset + sizeof(record), data, size) != ESP_OK) ||
      esp_partition_write(partition, offset + sizeof(record.marker), reinterpret_cast<uint8_t *>(&record) + sizeof(record.marker),
                          sizeof(record) - sizeof(record.marker)) != ESP_OK ||
      esp_partition_write(partition, offset, &record.marker, sizeof(record.marker)) != ESP_OK) return false;
  slot_ = slot; revision_ = record.revision; size_ = size;
  return true;
#else
  (void) data; (void) size; return false;
#endif
}
uint8_t *WallpaperStore::load(size_t &size, uint32_t &revision) {
  std::lock_guard<std::mutex> lock(mutex_);
  size = 0;
  revision = revision_;
#ifdef USE_ESP32
  if (!begin_locked()) return nullptr;
  revision = revision_;
  if (!size_) return nullptr;
  size = size_;
  auto *data = allocate(size_);
  if (!data) return nullptr;
  if (esp_partition_read(static_cast<const esp_partition_t *>(partition_),
                         OFFSET + slot_ * STRIDE + sizeof(Record), data, size_) != ESP_OK) {
    free(data); return nullptr;
  }
  size = size_;
  return data;
#else
  return nullptr;
#endif
}
}  // namespace espcontrol
