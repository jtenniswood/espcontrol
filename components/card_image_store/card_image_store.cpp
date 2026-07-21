#include "card_image_store.h"

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "esp_random.h"
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace esphome::card_image_store {

static const char *const TAG = "card_image_store";
static constexpr uint32_t CARD_IMAGE_MAGIC = 0x43494D47;  // CIMG
static constexpr uint32_t CARD_IMAGE_INDEX_MAGIC = 0x43494E58;  // CINX
static constexpr uint32_t CARD_IMAGE_INDEX_VERSION_LEGACY = 1;
static constexpr uint32_t CARD_IMAGE_INDEX_VERSION = 2;
static constexpr uint32_t CARD_IMAGE_CACHE_MAGIC = 0x4354484D;  // CTHM
static constexpr uint32_t CARD_IMAGE_CACHE_VERSION = 1;
static constexpr size_t CARD_IMAGE_RAM_CACHE_MAX_BYTES = 1536 * 1024;
static constexpr esp_partition_subtype_t CARD_IMAGE_PARTITION_SUBTYPE =
    static_cast<esp_partition_subtype_t>(0x40);

CardImageStore::CardImageStore() : mutex_(xSemaphoreCreateRecursiveMutex()) {}

CardImageReader::CardImageReader(CardImageStore *store, const CardImageInfo &info)
    : store_(store), info_(info) {
  this->status_code = 200;
  this->content_length = info.size;
}

int CardImageReader::read(uint8_t *buf, size_t max_len) {
  if (this->ended_ || this->store_ == nullptr || this->position_ >= this->info_.size) return 0;
  size_t count = std::min(max_len, this->info_.size - this->position_);
  int result = this->store_->read_at(this->info_, this->position_, buf, count);
  if (result > 0) {
    this->position_ += static_cast<size_t>(result);
    this->bytes_read_ += static_cast<size_t>(result);
  }
  return result;
}

void CardImageReader::end() {
  if (this->ended_) return;
  this->ended_ = true;
  if (this->store_ != nullptr) this->store_->close_reader(this->info_.id);
  this->store_ = nullptr;
}

const esp_partition_t *CardImageStore::partition_() {
  if (this->partition_attempted_) return this->partition_cache_;
  this->partition_attempted_ = true;
  this->partition_cache_ = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, CARD_IMAGE_PARTITION_SUBTYPE, "card_images");
  if (this->partition_cache_ == nullptr) {
    ESP_LOGW(TAG, "Dedicated card_images partition not found");
#ifdef ESPCONTROL_CARD_IMAGE_PARTITION_BYTES
  } else if (this->partition_cache_->size != ESPCONTROL_CARD_IMAGE_PARTITION_BYTES) {
    ESP_LOGE(TAG, "card_images partition size does not match the generated device capability");
    this->partition_cache_ = nullptr;
#endif
  } else if (this->partition_cache_->size < CARD_IMAGE_FLASH_SECTOR_SIZE) {
    ESP_LOGE(TAG, "card_images partition is too small");
    this->partition_cache_ = nullptr;
  }
  return this->partition_cache_;
}

bool CardImageStore::available() {
  LockGuard lock(this);
  return this->partition_() != nullptr;
}

size_t CardImageStore::data_capacity_() {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr) return 0;
  size_t index_bytes = CARD_IMAGE_INDEX_SECTORS * CARD_IMAGE_FLASH_SECTOR_SIZE;
  return partition->size > index_bytes ? partition->size - index_bytes : 0;
}

size_t CardImageStore::capacity() {
  LockGuard lock(this);
  return this->data_capacity_();
}

size_t CardImageStore::index_slot_offset_(size_t slot) {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr || slot >= CARD_IMAGE_INDEX_SECTORS) return 0;
  return partition->size - (CARD_IMAGE_INDEX_SECTORS - slot) * CARD_IMAGE_FLASH_SECTOR_SIZE;
}

size_t CardImageStore::record_size(size_t image_size) {
  size_t bytes = sizeof(CardImageHeader) + image_size;
  return ((bytes + CARD_IMAGE_FLASH_SECTOR_SIZE - 1) / CARD_IMAGE_FLASH_SECTOR_SIZE) *
         CARD_IMAGE_FLASH_SECTOR_SIZE;
}

bool CardImageStore::id_valid(const std::string &id) {
  if (id.empty() || id.size() > 40) return false;
  for (char ch : id) {
    if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-')) return false;
  }
  return true;
}

std::string CardImageStore::normalize_name(const std::string &value) {
  std::string out;
  out.reserve(std::min(value.size(), CARD_IMAGE_NAME_MAX_LENGTH));
  bool previous_space = false;
  for (char raw : value) {
    unsigned char ch = static_cast<unsigned char>(raw);
    if (ch < 0x20 || ch == 0x7F || raw == ',' || raw == ';') continue;
    if (std::isspace(ch)) {
      if (!out.empty() && !previous_space) out.push_back(' ');
      previous_space = true;
      continue;
    }
    out.push_back(raw);
    previous_space = false;
    if (out.size() >= CARD_IMAGE_NAME_MAX_LENGTH) break;
  }
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

uint32_t CardImageStore::crc32_update_(uint32_t crc, const uint8_t *data, size_t size) {
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320u : 0u);
  }
  return crc;
}

bool CardImageStore::read_header_(size_t offset, CardImageHeader &header) {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr || offset + sizeof(header) > partition->size) return false;
  return esp_partition_read(partition, offset, &header, sizeof(header)) == ESP_OK;
}

bool CardImageStore::header_valid_(const CardImageHeader &header, size_t offset) const {
  if (header.magic != CARD_IMAGE_MAGIC || header.version != CARD_IMAGE_FORMAT_VERSION) return false;
  if (header.size == 0 || header.size > CARD_IMAGE_MAX_BYTES) return false;
  if (header.id[sizeof(header.id) - 1] != '\0' || header.name[sizeof(header.name) - 1] != '\0') return false;
  if (!id_valid(header.id)) return false;
  return this->partition_cache_ != nullptr && offset + record_size(header.size) <= this->partition_cache_->size;
}

bool CardImageStore::cache_header_valid_(const CardImageCacheHeader &header, size_t offset) const {
  if (header.magic != CARD_IMAGE_CACHE_MAGIC || header.version != CARD_IMAGE_CACHE_VERSION) return false;
  if (header.size == 0 || header.size > CARD_IMAGE_CACHE_MAX_BYTES || header.width == 0 || header.height == 0) {
    return false;
  }
  if (header.size != static_cast<size_t>(header.width) * header.height * 2) return false;
  if (header.id[sizeof(header.id) - 1] != '\0' || !id_valid(header.id)) return false;
  return this->partition_cache_ != nullptr && offset + record_size(header.size) <= this->partition_cache_->size;
}

bool CardImageStore::verify_record_(const CardImageHeader &header, size_t offset) {
  uint8_t buffer[1024];
  uint32_t crc = 0xFFFFFFFFu;
  size_t remaining = header.size;
  size_t position = 0;
  while (remaining > 0) {
    size_t count = std::min(remaining, sizeof(buffer));
    if (esp_partition_read(this->partition_(), offset + sizeof(CardImageHeader) + position,
                           buffer, count) != ESP_OK) return false;
    crc = crc32_update_(crc, buffer, count);
    position += count;
    remaining -= count;
  }
  return (crc ^ 0xFFFFFFFFu) == header.crc32;
}

CardImageInfo CardImageStore::info_from_header_(const CardImageHeader &header, size_t offset) const {
  CardImageInfo info;
  info.id = header.id;
  info.name = normalize_name(header.name);
  if (info.name.empty()) info.name = info.id;
  info.size = header.size;
  info.offset = offset;
  info.crc32 = header.crc32;
  return info;
}

CardImageCacheInfo CardImageStore::cache_info_from_header_(const CardImageCacheHeader &header,
                                                           size_t offset) const {
  CardImageCacheInfo info;
  info.id = header.id;
  info.source_crc32 = header.source_crc32;
  info.data_crc32 = header.data_crc32;
  info.width = header.width;
  info.height = header.height;
  info.size = header.size;
  info.offset = offset;
  return info;
}

bool CardImageStore::read_index_slot_(size_t slot, CardImageIndexHeader &header,
                                      std::vector<uint32_t> &offsets,
                                      std::vector<std::string> &names) {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr || slot >= CARD_IMAGE_INDEX_SECTORS) return false;
  size_t slot_offset = this->index_slot_offset_(slot);
  if (esp_partition_read(partition, slot_offset, &header, sizeof(header)) != ESP_OK) return false;
  if (header.magic != CARD_IMAGE_INDEX_MAGIC ||
      (header.version != CARD_IMAGE_INDEX_VERSION_LEGACY &&
       header.version != CARD_IMAGE_INDEX_VERSION)) {
    return false;
  }
  size_t entry_size = header.version == CARD_IMAGE_INDEX_VERSION
                        ? sizeof(CardImageIndexEntry) : sizeof(uint32_t);
  size_t max_entries = (CARD_IMAGE_FLASH_SECTOR_SIZE - sizeof(header)) / entry_size;
  if (header.count > max_entries) {
    return false;
  }
  offsets.resize(header.count);
  names.assign(header.count, "");
  std::vector<CardImageIndexEntry> entries;
  const uint8_t *payload = nullptr;
  size_t payload_size = header.count * entry_size;
  if (header.version == CARD_IMAGE_INDEX_VERSION) {
    entries.resize(header.count);
    if (!entries.empty() &&
        esp_partition_read(partition, slot_offset + sizeof(header), entries.data(),
                           payload_size) != ESP_OK) {
      return false;
    }
    for (size_t i = 0; i < entries.size(); i++) {
      if (entries[i].name[sizeof(entries[i].name) - 1] != '\0') return false;
      offsets[i] = entries[i].offset;
      names[i] = normalize_name(entries[i].name);
    }
    payload = reinterpret_cast<const uint8_t *>(entries.data());
  } else {
    if (!offsets.empty() &&
        esp_partition_read(partition, slot_offset + sizeof(header), offsets.data(),
                           payload_size) != ESP_OK) {
      return false;
    }
    payload = reinterpret_cast<const uint8_t *>(offsets.data());
  }
  uint32_t crc = 0xFFFFFFFFu;
  crc = crc32_update_(crc, reinterpret_cast<const uint8_t *>(&header.generation), sizeof(header.generation));
  crc = crc32_update_(crc, reinterpret_cast<const uint8_t *>(&header.count), sizeof(header.count));
  if (payload_size > 0) crc = crc32_update_(crc, payload, payload_size);
  return (crc ^ 0xFFFFFFFFu) == header.crc32;
}

bool CardImageStore::load_index_() {
  CardImageIndexHeader headers[CARD_IMAGE_INDEX_SECTORS]{};
  std::vector<uint32_t> offsets[CARD_IMAGE_INDEX_SECTORS];
  std::vector<std::string> names[CARD_IMAGE_INDEX_SECTORS];
  bool valid[CARD_IMAGE_INDEX_SECTORS]{};
  for (size_t slot = 0; slot < CARD_IMAGE_INDEX_SECTORS; slot++) {
    valid[slot] = this->read_index_slot_(slot, headers[slot], offsets[slot], names[slot]);
  }
  // Preserve the newest readable journal position even when recovery must scan
  // the data records. The rebuilt index must be written to the other slot with
  // a higher generation, otherwise a stale surviving slot can win next boot.
  if (valid[0] || valid[1]) {
    size_t newest = valid[1] && (!valid[0] || headers[1].generation > headers[0].generation) ? 1 : 0;
    this->persistent_index_active_ = true;
    this->active_index_slot_ = newest;
    this->index_generation_ = headers[newest].generation;
  }
  // One valid slot means the journal is degraded. Rebuild it by scanning the
  // data records instead of silently accepting a potentially stale index.
  if (!valid[0] || !valid[1]) {
    this->recovered_index_names_.clear();
    size_t surviving = valid[1] ? 1 : 0;
    if (valid[surviving]) {
      for (size_t i = 0; i < offsets[surviving].size(); i++) {
        if (!names[surviving][i].empty()) {
          this->recovered_index_names_.push_back({offsets[surviving][i], names[surviving][i]});
        }
      }
    }
    return false;
  }

  size_t first = valid[1] && (!valid[0] || headers[1].generation > headers[0].generation) ? 1 : 0;
  size_t order[CARD_IMAGE_INDEX_SECTORS] = {first, 1 - first};
  for (size_t candidate : order) {
    if (!valid[candidate]) continue;
    std::vector<CardImageInfo> loaded;
    std::vector<CardImageCacheInfo> loaded_caches;
    loaded.reserve(offsets[candidate].size());
    bool records_valid = true;
    for (size_t entry_index = 0; entry_index < offsets[candidate].size(); entry_index++) {
      uint32_t raw_offset = offsets[candidate][entry_index];
      size_t offset = raw_offset;
      uint32_t magic = 0;
      if ((offset % CARD_IMAGE_FLASH_SECTOR_SIZE) != 0 || offset >= this->data_capacity_() ||
          esp_partition_read(this->partition_(), offset, &magic, sizeof(magic)) != ESP_OK) {
        records_valid = false;
        break;
      }
      if (magic == CARD_IMAGE_MAGIC) {
        CardImageHeader record{};
        if (!this->read_header_(offset, record) || !this->header_valid_(record, offset) ||
            offset + record_size(record.size) > this->data_capacity_() ||
            !this->verify_record_(record, offset)) {
          records_valid = false;
          break;
        }
        CardImageInfo info = this->info_from_header_(record, offset);
        if (!names[candidate][entry_index].empty()) info.name = names[candidate][entry_index];
        loaded.push_back(std::move(info));
      } else if (magic == CARD_IMAGE_CACHE_MAGIC) {
        CardImageCacheHeader record{};
        if (esp_partition_read(this->partition_(), offset, &record, sizeof(record)) != ESP_OK ||
            !this->cache_header_valid_(record, offset) ||
            offset + record_size(record.size) > this->data_capacity_()) {
          records_valid = false;
          break;
        }
        loaded_caches.push_back(this->cache_info_from_header_(record, offset));
      } else {
        records_valid = false;
        break;
      }
    }
    if (!records_valid) return false;
    this->images_ = std::move(loaded);
    this->caches_ = std::move(loaded_caches);
    this->persistent_index_active_ = true;
    this->index_generation_ = headers[candidate].generation;
    this->active_index_slot_ = candidate;
    ESP_LOGI(TAG, "Loaded %zu card images and %zu display caches from persistent index",
             this->images_.size(), this->caches_.size());
    return true;
  }
  return false;
}

bool CardImageStore::index_region_available_() const {
  if (this->partition_cache_ == nullptr) return false;
  size_t data_capacity = this->partition_cache_->size -
                         CARD_IMAGE_INDEX_SECTORS * CARD_IMAGE_FLASH_SECTOR_SIZE;
  for (const auto &image : this->images_) {
    if (image.offset + record_size(image.size) > data_capacity) return false;
  }
  for (const auto &cache : this->caches_) {
    if (cache.offset + record_size(cache.size) > data_capacity) return false;
  }
  return true;
}

bool CardImageStore::persist_index_(int name_override_index,
                                    const std::string *name_override) {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr || !this->index_region_available_()) return false;
  size_t max_entries = (CARD_IMAGE_FLASH_SECTOR_SIZE - sizeof(CardImageIndexHeader)) /
                       sizeof(CardImageIndexEntry);
  size_t entry_count = this->images_.size() + this->caches_.size();
  if (entry_count > max_entries) {
    ESP_LOGW(TAG, "Too many card image records for persistent index: %zu", entry_count);
    return false;
  }

  size_t slot = this->persistent_index_active_ ? 1 - this->active_index_slot_ : 0;
  std::vector<uint8_t> sector(CARD_IMAGE_FLASH_SECTOR_SIZE, 0xFF);
  auto *header = reinterpret_cast<CardImageIndexHeader *>(sector.data());
  header->magic = CARD_IMAGE_INDEX_MAGIC;
  header->version = CARD_IMAGE_INDEX_VERSION;
  header->generation = this->index_generation_ + 1;
  header->count = static_cast<uint32_t>(entry_count);
  auto *entries = reinterpret_cast<CardImageIndexEntry *>(sector.data() + sizeof(CardImageIndexHeader));
  size_t stored = 0;
  for (size_t image_index = 0; image_index < this->images_.size(); image_index++) {
    const auto &image = this->images_[image_index];
    const std::string &stored_name =
      name_override != nullptr && static_cast<int>(image_index) == name_override_index
        ? *name_override : image.name;
    entries[stored].offset = static_cast<uint32_t>(image.offset);
    memset(entries[stored].name, 0, sizeof(entries[stored].name));
    strlcpy(entries[stored].name, normalize_name(stored_name).c_str(), sizeof(entries[stored].name));
    stored++;
  }
  for (const auto &cache : this->caches_) {
    entries[stored].offset = static_cast<uint32_t>(cache.offset);
    memset(entries[stored].name, 0, sizeof(entries[stored].name));
    stored++;
  }
  uint32_t crc = 0xFFFFFFFFu;
  crc = crc32_update_(crc, reinterpret_cast<const uint8_t *>(&header->generation), sizeof(header->generation));
  crc = crc32_update_(crc, reinterpret_cast<const uint8_t *>(&header->count), sizeof(header->count));
  if (entry_count > 0) {
    crc = crc32_update_(crc, reinterpret_cast<const uint8_t *>(entries),
                        entry_count * sizeof(CardImageIndexEntry));
  }
  header->crc32 = crc ^ 0xFFFFFFFFu;

  size_t offset = this->index_slot_offset_(slot);
  esp_err_t err = esp_partition_erase_range(partition, offset, CARD_IMAGE_FLASH_SECTOR_SIZE);
  if (err == ESP_OK) err = esp_partition_write(partition, offset, sector.data(), sector.size());
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Could not persist card image index: %s", esp_err_to_name(err));
    return false;
  }
  this->persistent_index_active_ = true;
  this->active_index_slot_ = slot;
  this->index_generation_ = header->generation;
  return true;
}

void CardImageStore::scan_legacy_index_() {
  const esp_partition_t *partition = this->partition_();
  if (partition == nullptr) return;
  for (size_t offset = 0; offset + sizeof(CardImageHeader) <= partition->size;
       offset += CARD_IMAGE_FLASH_SECTOR_SIZE) {
    uint32_t magic = 0;
    if (esp_partition_read(partition, offset, &magic, sizeof(magic)) != ESP_OK) continue;
    if (magic == CARD_IMAGE_MAGIC) {
      CardImageHeader header{};
      if (!this->read_header_(offset, header) || !this->header_valid_(header, offset)) continue;
      if (!this->verify_record_(header, offset)) {
        ESP_LOGW(TAG, "Ignoring card image with invalid CRC at 0x%zx", offset);
        continue;
      }
      CardImageInfo info = this->info_from_header_(header, offset);
      for (const auto &name : this->recovered_index_names_) {
        if (name.first == offset && !name.second.empty()) {
          info.name = name.second;
          break;
        }
      }
      this->images_.push_back(std::move(info));
    } else if (magic == CARD_IMAGE_CACHE_MAGIC) {
      CardImageCacheHeader header{};
      if (esp_partition_read(partition, offset, &header, sizeof(header)) != ESP_OK ||
          !this->cache_header_valid_(header, offset)) continue;
      this->caches_.push_back(this->cache_info_from_header_(header, offset));
    }
  }
}

void CardImageStore::ensure_index_() {
  if (this->index_loaded_) return;
  this->index_loaded_ = true;
  this->images_.clear();
  this->caches_.clear();
  if (this->partition_() == nullptr) return;
  bool loaded = this->load_index_();
  if (!loaded) {
    uint32_t started_at = esphome::millis();
    this->scan_legacy_index_();
    ESP_LOGI(TAG, "Recovered %zu card images and %zu display caches by scanning flash in %lu ms",
             this->images_.size(), this->caches_.size(),
             static_cast<unsigned long>(esphome::millis() - started_at));
  }
  this->recovered_index_names_.clear();
  bool cache_layout_changed = this->purge_caches_outside_cache_region_();
  if ((!loaded || cache_layout_changed) &&
      !this->persist_index_() && !this->index_region_available_()) {
    ESP_LOGW(TAG, "Legacy card image overlaps reserved index area; full scan remains enabled");
  }
}

std::vector<CardImageInfo> CardImageStore::list() {
  LockGuard lock(this);
  this->ensure_index_();
  return this->images_;
}

int CardImageStore::find_index_(const std::string &id) const {
  for (size_t i = 0; i < this->images_.size(); i++) if (this->images_[i].id == id) return static_cast<int>(i);
  return -1;
}

bool CardImageStore::find(const std::string &id, CardImageInfo &out) {
  LockGuard lock(this);
  this->ensure_index_();
  int index = this->find_index_(id);
  if (index < 0) return false;
  out = this->images_[index];
  return true;
}

size_t CardImageStore::used_bytes() {
  LockGuard lock(this);
  this->ensure_index_();
  size_t used = 0;
  for (const auto &image : this->images_) used += record_size(image.size);
  for (const auto &cache : this->caches_) used += record_size(cache.size);
  return used;
}

size_t CardImageStore::free_bytes() {
  LockGuard lock(this);
  size_t total = this->capacity();
  size_t used = this->used_bytes();
  return total > used ? total - used : 0;
}

size_t CardImageStore::cache_region_start_() {
  return layout::cache_region_start(this->capacity(), CARD_IMAGE_FLASH_SECTOR_SIZE);
}

bool CardImageStore::purge_caches_outside_cache_region_() {
  size_t cache_start = this->cache_region_start_();
  size_t cache_end = this->capacity();
  bool changed = false;
  auto cache = this->caches_.begin();
  while (cache != this->caches_.end()) {
    layout::RecordSpan span{cache->offset, record_size(cache->size)};
    if (layout::span_within_region(span, cache_start, cache_end)) {
      ++cache;
      continue;
    }
    ESP_LOGI(TAG, "Discarding rebuildable RGB565 cache outside the cache region for %s",
             cache->id.c_str());
    esp_partition_erase_range(this->partition_(), cache->offset, span.size);
    cache = this->caches_.erase(cache);
    changed = true;
  }
  return changed;
}

int CardImageStore::find_empty_offset_(size_t image_size, bool cache_only) {
  this->ensure_index_();
  std::vector<layout::RecordSpan> records;
  records.reserve(this->images_.size() + this->caches_.size());
  for (const auto &image : this->images_) {
    records.push_back({image.offset, record_size(image.size)});
  }
  for (const auto &cache : this->caches_) {
    records.push_back({cache.offset, record_size(cache.size)});
  }
  size_t start = cache_only ? this->cache_region_start_() : 0;
  return layout::find_empty_offset(this->capacity(), CARD_IMAGE_FLASH_SECTOR_SIZE,
                                   record_size(image_size), records, start,
                                   this->capacity());
}

std::string CardImageStore::next_id_() {
  for (int attempt = 0; attempt < 8; attempt++) {
    uint8_t random[16];
    esp_fill_random(random, sizeof(random));
    char id[37];
    std::snprintf(id, sizeof(id),
                  "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                  random[0], random[1], random[2], random[3], random[4], random[5], random[6], random[7],
                  random[8], random[9], random[10], random[11], random[12], random[13], random[14], random[15]);
    if (this->find_index_(id) < 0) return id;
  }
  return "";
}

esp_err_t CardImageStore::begin_upload(size_t size, CardImageUpload &upload) {
  LockGuard lock(this);
  if (!this->available()) return ESP_ERR_NOT_FOUND;
  if (size == 0 || size > CARD_IMAGE_MAX_BYTES) return ESP_ERR_INVALID_SIZE;
  int offset = this->find_empty_offset_(size);
  while (offset < 0 && !this->caches_.empty()) {
    const auto evicted = this->caches_.front();
    ESP_LOGI(TAG, "Evicting rebuildable RGB565 cache for %s to accept image upload",
             evicted.id.c_str());
    esp_err_t erase_err = esp_partition_erase_range(
        this->partition_(), evicted.offset, record_size(evicted.size));
    if (erase_err != ESP_OK) return erase_err;
    this->caches_.erase(this->caches_.begin());
    if (!this->persist_index_()) return ESP_FAIL;
    offset = this->find_empty_offset_(size);
  }
  if (offset < 0) return ESP_ERR_NO_MEM;
  upload = {};
  upload.id = this->next_id_();
  if (upload.id.empty()) return ESP_FAIL;
  upload.offset = static_cast<size_t>(offset);
  upload.size = size;
  upload.record_size = record_size(size);
  return esp_partition_erase_range(this->partition_(), upload.offset, upload.record_size);
}

esp_err_t CardImageStore::write_upload(CardImageUpload &upload, const uint8_t *data, size_t size) {
  LockGuard lock(this);
  if (data == nullptr || size == 0 || upload.written + size > upload.size) return ESP_ERR_INVALID_ARG;
  for (size_t i = 0; i < size; i++) {
    if (upload.written + i < 2) upload.first_bytes[upload.written + i] = data[i];
    upload.last_bytes[0] = upload.last_bytes[1];
    upload.last_bytes[1] = data[i];
  }
  esp_err_t err = esp_partition_write(this->partition_(),
      upload.offset + sizeof(CardImageHeader) + upload.written, data, size);
  if (err != ESP_OK) return err;
  upload.crc32 = crc32_update_(upload.crc32, data, size);
  upload.written += size;
  return ESP_OK;
}

esp_err_t CardImageStore::commit_upload(CardImageUpload &upload, CardImageInfo &out) {
  LockGuard lock(this);
  if (upload.written != upload.size) return ESP_ERR_INVALID_SIZE;
  if (upload.first_bytes[0] != 0xFF || upload.first_bytes[1] != 0xD8 ||
      upload.last_bytes[0] != 0xFF || upload.last_bytes[1] != 0xD9) return ESP_ERR_INVALID_ARG;
  CardImageHeader header{};
  header.magic = CARD_IMAGE_MAGIC;
  header.version = CARD_IMAGE_FORMAT_VERSION;
  header.size = upload.size;
  header.crc32 = upload.crc32 ^ 0xFFFFFFFFu;
  strlcpy(header.id, upload.id.c_str(), sizeof(header.id));
  strlcpy(header.name, upload.id.c_str(), sizeof(header.name));
  esp_err_t err = esp_partition_write(this->partition_(), upload.offset, &header, sizeof(header));
  if (err != ESP_OK) return err;
  out = this->info_from_header_(header, upload.offset);
  this->images_.push_back(out);
  if (this->index_region_available_() && !this->persist_index_()) {
    this->images_.pop_back();
    esp_partition_erase_range(this->partition_(), upload.offset, upload.record_size);
    return ESP_FAIL;
  }
  upload = {};
  return ESP_OK;
}

void CardImageStore::abort_upload(CardImageUpload &upload) {
  LockGuard lock(this);
  if (this->partition_() && upload.record_size) {
    esp_partition_erase_range(this->partition_(), upload.offset, upload.record_size);
  }
  upload = {};
}

esp_err_t CardImageStore::rename(const std::string &id, const std::string &name, CardImageInfo &out) {
  LockGuard lock(this);
  this->ensure_index_();
  int index = this->find_index_(id);
  if (index < 0) return ESP_ERR_NOT_FOUND;
  std::string normalized = normalize_name(name);
  if (normalized.empty()) normalized = id;
  if (this->images_[index].name == normalized) {
    out = this->images_[index];
    return ESP_OK;
  }
  if (!this->persist_index_(index, &normalized)) return ESP_FAIL;
  this->images_[index].name = normalized;
  out = this->images_[index];
  return ESP_OK;
}

int CardImageStore::find_cache_index_(const std::string &id, uint32_t source_crc32,
                                      uint16_t width, uint16_t height) const {
  for (size_t i = 0; i < this->caches_.size(); i++) {
    const auto &cache = this->caches_[i];
    if (cache.id == id && cache.source_crc32 == source_crc32 &&
        cache.width == width && cache.height == height) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

int CardImageStore::find_ram_cache_index_(const std::string &id, uint32_t source_crc32,
                                          uint16_t width, uint16_t height) const {
  for (size_t i = 0; i < this->ram_caches_.size(); i++) {
    const auto &cache = this->ram_caches_[i];
    if (cache.id == id && cache.source_crc32 == source_crc32 &&
        cache.width == width && cache.height == height) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void CardImageStore::remember_rgb565_cache_(const std::string &id, uint32_t source_crc32,
                                             uint16_t width, uint16_t height,
                                             const uint8_t *buffer, size_t size) {
  if (buffer == nullptr || size == 0 || size > CARD_IMAGE_RAM_CACHE_MAX_BYTES) return;
  int existing = this->find_ram_cache_index_(id, source_crc32, width, height);
  if (existing >= 0) {
    this->ram_caches_[existing].last_used = ++this->ram_cache_clock_;
    return;
  }
  while (!this->ram_caches_.empty() &&
         this->ram_cache_bytes_ + size > CARD_IMAGE_RAM_CACHE_MAX_BYTES) {
    auto oldest = std::min_element(this->ram_caches_.begin(), this->ram_caches_.end(),
                                   [](const auto &a, const auto &b) {
                                     return a.last_used < b.last_used;
                                   });
    this->ram_cache_bytes_ -= oldest->size;
    heap_caps_free(oldest->data);
    this->ram_caches_.erase(oldest);
  }
  auto *copy = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (copy == nullptr) return;
  memcpy(copy, buffer, size);
  this->ram_caches_.push_back(
      {id, source_crc32, width, height, copy, size, ++this->ram_cache_clock_});
  this->ram_cache_bytes_ += size;
}

void CardImageStore::erase_ram_caches_for_id_(const std::string &id) {
  auto it = this->ram_caches_.begin();
  while (it != this->ram_caches_.end()) {
    if (it->id != id) {
      ++it;
      continue;
    }
    this->ram_cache_bytes_ -= it->size;
    heap_caps_free(it->data);
    it = this->ram_caches_.erase(it);
  }
}

void CardImageStore::erase_caches_for_id_(const std::string &id) {
  this->erase_ram_caches_for_id_(id);
  auto it = this->caches_.begin();
  while (it != this->caches_.end()) {
    if (it->id != id) {
      ++it;
      continue;
    }
    esp_partition_erase_range(this->partition_(), it->offset, record_size(it->size));
    it = this->caches_.erase(it);
  }
}

bool CardImageStore::read_rgb565_cache(const std::string &id, uint32_t source_crc32,
                                       uint16_t width, uint16_t height, uint8_t *buffer,
                                       size_t size) {
  LockGuard lock(this);
  this->ensure_index_();
  uint32_t started = millis();
  int ram_index = this->find_ram_cache_index_(id, source_crc32, width, height);
  if (ram_index >= 0 && buffer != nullptr && size == this->ram_caches_[ram_index].size) {
    memcpy(buffer, this->ram_caches_[ram_index].data, size);
    this->ram_caches_[ram_index].last_used = ++this->ram_cache_clock_;
    ESP_LOGI(TAG, "Loaded RGB565 card image %s from RAM in %u ms", id.c_str(), millis() - started);
    return true;
  }
  int index = this->find_cache_index_(id, source_crc32, width, height);
  if (index < 0 || buffer == nullptr || size != this->caches_[index].size) return false;
  const auto cache = this->caches_[index];
  if (esp_partition_read(this->partition_(), cache.offset + sizeof(CardImageCacheHeader),
                         buffer, size) != ESP_OK ||
      esp_rom_crc32_le(0, buffer, static_cast<uint32_t>(size)) != cache.data_crc32) {
    ESP_LOGW(TAG, "Discarding invalid RGB565 cache for %s (%ux%u)", id.c_str(), width, height);
    esp_partition_erase_range(this->partition_(), cache.offset, record_size(cache.size));
    this->caches_.erase(this->caches_.begin() + index);
    this->persist_index_();
    return false;
  }
  this->remember_rgb565_cache_(id, source_crc32, width, height, buffer, size);
  ESP_LOGI(TAG, "Loaded RGB565 card image %s from flash in %u ms", id.c_str(), millis() - started);
  return true;
}

esp_err_t CardImageStore::write_rgb565_cache(const std::string &id, uint32_t source_crc32,
                                             uint16_t width, uint16_t height,
                                             const uint8_t *buffer, size_t size) {
  LockGuard lock(this);
  this->ensure_index_();
  if (!id_valid(id) || width == 0 || height == 0 || buffer == nullptr ||
      size != static_cast<size_t>(width) * height * 2 || size > CARD_IMAGE_CACHE_MAX_BYTES) {
    return ESP_ERR_INVALID_ARG;
  }
  if (this->find_cache_index_(id, source_crc32, width, height) >= 0) return ESP_OK;

  bool removed_stale_cache = false;
  auto stale = this->caches_.begin();
  while (stale != this->caches_.end()) {
    if (stale->id == id && stale->width == width && stale->height == height) {
      esp_partition_erase_range(this->partition_(), stale->offset, record_size(stale->size));
      stale = this->caches_.erase(stale);
      removed_stale_cache = true;
    } else {
      ++stale;
    }
  }
  if (removed_stale_cache && !this->persist_index_()) return ESP_FAIL;

  int offset = this->find_empty_offset_(size, true);
  while (offset < 0 && !this->caches_.empty()) {
    const auto evicted = this->caches_.front();
    ESP_LOGI(TAG, "Evicting rebuildable RGB565 cache for %s to free image space", evicted.id.c_str());
    esp_err_t erase_err = esp_partition_erase_range(
        this->partition_(), evicted.offset, record_size(evicted.size));
    if (erase_err != ESP_OK) return erase_err;
    this->caches_.erase(this->caches_.begin());
    if (!this->persist_index_()) return ESP_FAIL;
    offset = this->find_empty_offset_(size, true);
  }
  if (offset < 0) return ESP_ERR_NO_MEM;
  size_t stored_size = record_size(size);
  esp_err_t err = esp_partition_erase_range(this->partition_(), offset, stored_size);
  if (err == ESP_OK) {
    err = esp_partition_write(this->partition_(), offset + sizeof(CardImageCacheHeader), buffer, size);
  }
  CardImageCacheHeader header{};
  if (err == ESP_OK) {
    header.magic = CARD_IMAGE_CACHE_MAGIC;
    header.version = CARD_IMAGE_CACHE_VERSION;
    header.size = static_cast<uint32_t>(size);
    header.source_crc32 = source_crc32;
    header.data_crc32 = esp_rom_crc32_le(0, buffer, static_cast<uint32_t>(size));
    header.width = width;
    header.height = height;
    strlcpy(header.id, id.c_str(), sizeof(header.id));
    err = esp_partition_write(this->partition_(), offset, &header, sizeof(header));
  }
  if (err != ESP_OK) {
    esp_partition_erase_range(this->partition_(), offset, stored_size);
    return err;
  }

  this->caches_.push_back(this->cache_info_from_header_(header, offset));
  if (!this->persist_index_()) {
    this->caches_.pop_back();
    esp_partition_erase_range(this->partition_(), offset, stored_size);
    return ESP_FAIL;
  }
  this->remember_rgb565_cache_(id, source_crc32, width, height, buffer, size);
  ESP_LOGI(TAG, "Cached RGB565 card image %s at %ux%u (%zu bytes)", id.c_str(), width, height, size);
  return ESP_OK;
}

esp_err_t CardImageStore::erase(const std::string &id) {
  LockGuard lock(this);
  this->ensure_index_();
  int index = this->find_index_(id);
  if (index < 0) return ESP_ERR_NOT_FOUND;
  for (const auto &reader : this->readers_) if (reader.first == id && reader.second > 0) return ESP_ERR_INVALID_STATE;
  const auto image = this->images_[index];
  esp_err_t err = esp_partition_erase_range(this->partition_(), image.offset, record_size(image.size));
  if (err == ESP_OK) {
    this->images_.erase(this->images_.begin() + index);
    this->erase_caches_for_id_(id);
    if (this->index_region_available_()) this->persist_index_();
  }
  return err;
}

std::shared_ptr<http_request::HttpContainer> CardImageStore::open(const std::string &id) {
  LockGuard lock(this);
  CardImageInfo info;
  if (!this->find(id, info)) return nullptr;
  auto it = std::find_if(this->readers_.begin(), this->readers_.end(), [&id](const auto &entry) { return entry.first == id; });
  if (it == this->readers_.end()) this->readers_.push_back({id, 1}); else it->second++;
  return std::make_shared<CardImageReader>(this, info);
}

int CardImageStore::read_at(const CardImageInfo &info, size_t position, uint8_t *buffer, size_t size) {
  LockGuard lock(this);
  if (buffer == nullptr || position >= info.size) return 0;
  size = std::min(size, info.size - position);
  esp_err_t err = esp_partition_read(this->partition_(),
      info.offset + sizeof(CardImageHeader) + position, buffer, size);
  return err == ESP_OK ? static_cast<int>(size) : -1;
}

void CardImageStore::close_reader(const std::string &id) {
  LockGuard lock(this);
  auto it = std::find_if(this->readers_.begin(), this->readers_.end(), [&id](const auto &entry) { return entry.first == id; });
  if (it == this->readers_.end()) return;
  if (it->second > 1) it->second--; else this->readers_.erase(it);
}

}  // namespace esphome::card_image_store
