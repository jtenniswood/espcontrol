#pragma once

#include "esphome/components/http_request/http_request.h"
#include "card_image_store_layout.h"

#ifdef USE_ESP32
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace esphome::card_image_store {

static constexpr size_t CARD_IMAGE_MAX_BYTES = 64 * 1024;
static constexpr size_t CARD_IMAGE_CACHE_MAX_BYTES = 2 * 1024 * 1024;
static constexpr size_t CARD_IMAGE_NAME_MAX_LENGTH = 40;
static constexpr size_t CARD_IMAGE_FLASH_SECTOR_SIZE = 4096;
static constexpr uint32_t CARD_IMAGE_FORMAT_VERSION = 2;
static constexpr size_t CARD_IMAGE_INDEX_SECTORS = 2;
static constexpr size_t CARD_IMAGE_INDEX_HEADER_SIZE = 5 * sizeof(uint32_t);
static constexpr size_t CARD_IMAGE_INDEX_ENTRY_SIZE = sizeof(uint32_t) + 48;
static constexpr size_t CARD_IMAGE_INDEX_MAX_RECORDS =
    (CARD_IMAGE_FLASH_SECTOR_SIZE - CARD_IMAGE_INDEX_HEADER_SIZE) /
    CARD_IMAGE_INDEX_ENTRY_SIZE;

struct CardImageInfo {
  std::string id;
  std::string name;
  size_t size{0};
  size_t offset{0};
  uint32_t crc32{0};
};

struct CardImageUpload {
  std::string id;
  size_t offset{0};
  size_t size{0};
  size_t written{0};
  size_t record_size{0};
  uint32_t crc32{0xFFFFFFFFu};
  uint8_t first_bytes[2]{0, 0};
  uint8_t last_bytes[2]{0, 0};
};

struct CardImageCacheInfo {
  std::string id;
  uint32_t source_crc32{0};
  uint32_t data_crc32{0};
  uint16_t width{0};
  uint16_t height{0};
  size_t size{0};
  size_t offset{0};
};

class CardImageStore;

class CardImageReader : public http_request::HttpContainer {
 public:
  CardImageReader(CardImageStore *store, const CardImageInfo &info);
  ~CardImageReader() override { this->end(); }

  int read(uint8_t *buf, size_t max_len) override;
  void end() override;

 protected:
  CardImageStore *store_{nullptr};
  CardImageInfo info_{};
  size_t position_{0};
  bool ended_{false};
};

class CardImageStore {
 public:
  CardImageStore();

  bool available();
  size_t capacity();
  size_t used_bytes();
  size_t free_bytes();
  std::vector<CardImageInfo> list();
  bool find(const std::string &id, CardImageInfo &out);

  esp_err_t begin_upload(size_t size, CardImageUpload &upload);
  esp_err_t write_upload(CardImageUpload &upload, const uint8_t *data, size_t size);
  esp_err_t commit_upload(CardImageUpload &upload, CardImageInfo &out);
  void abort_upload(CardImageUpload &upload);
  esp_err_t rename(const std::string &id, const std::string &name, CardImageInfo &out);
  esp_err_t reserve_erase(const std::string &id);
  void cancel_erase(const std::string &id);
  esp_err_t erase(const std::string &id);

  bool read_rgb565_cache(const std::string &id, uint32_t source_crc32,
                         uint16_t width, uint16_t height, uint8_t *buffer,
                         size_t size);
  esp_err_t write_rgb565_cache(const std::string &id, uint32_t source_crc32,
                               uint16_t width, uint16_t height,
                               const uint8_t *buffer, size_t size);

  std::shared_ptr<http_request::HttpContainer> open(const std::string &id);
  int read_at(const CardImageInfo &info, size_t position, uint8_t *buffer, size_t size);
  void close_reader(const std::string &id);

  static bool id_valid(const std::string &id);
  static std::string normalize_name(const std::string &value);
  static size_t record_size(size_t image_size);

 protected:
  class LockGuard {
   public:
    explicit LockGuard(CardImageStore *store) : store_(store) {
      if (this->store_ != nullptr && this->store_->mutex_ != nullptr) {
        xSemaphoreTakeRecursive(this->store_->mutex_, portMAX_DELAY);
      }
    }
    ~LockGuard() {
      if (this->store_ != nullptr && this->store_->mutex_ != nullptr) {
        xSemaphoreGiveRecursive(this->store_->mutex_);
      }
    }

   protected:
    CardImageStore *store_{nullptr};
  };

  struct CardImageHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t crc32;
    char id[48];
    char name[48];
    uint8_t padding[16];
  };
  static_assert(sizeof(CardImageHeader) == 128, "Card image header must remain 128 bytes");

  struct CardImageIndexHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t generation;
    uint32_t count;
    uint32_t crc32;
  };
  static_assert(sizeof(CardImageIndexHeader) == CARD_IMAGE_INDEX_HEADER_SIZE,
                "Card image index header must remain stable");

  struct CardImageIndexEntry {
    uint32_t offset;
    char name[48];
  };
  static_assert(sizeof(CardImageIndexEntry) == CARD_IMAGE_INDEX_ENTRY_SIZE,
                "Card image index entry must remain stable");

  struct CardImageCacheHeader {
    uint32_t magic;
    uint32_t version;
    uint32_t size;
    uint32_t source_crc32;
    uint32_t data_crc32;
    uint16_t width;
    uint16_t height;
    char id[48];
    uint8_t padding[56];
  };
  static_assert(sizeof(CardImageCacheHeader) == 128, "Card image cache header must remain 128 bytes");

  const esp_partition_t *partition_();
  size_t data_capacity_();
  size_t index_slot_offset_(size_t slot);
  bool load_index_();
  bool read_index_slot_(size_t slot, CardImageIndexHeader &header,
                        std::vector<uint32_t> &offsets,
                        std::vector<std::string> &names);
  bool persist_index_(int name_override_index = -1,
                      const std::string *name_override = nullptr);
  bool index_region_available_() const;
  void scan_legacy_index_();
  void ensure_index_();
  bool read_header_(size_t offset, CardImageHeader &header);
  bool header_valid_(const CardImageHeader &header, size_t offset) const;
  bool verify_record_(const CardImageHeader &header, size_t offset);
  int find_empty_offset_(size_t image_size, bool cache_only = false);
  size_t cache_region_start_();
  bool purge_caches_outside_cache_region_();
  std::string next_id_();
  CardImageInfo info_from_header_(const CardImageHeader &header, size_t offset) const;
  bool cache_header_valid_(const CardImageCacheHeader &header, size_t offset) const;
  CardImageCacheInfo cache_info_from_header_(const CardImageCacheHeader &header,
                                             size_t offset) const;
  int find_cache_index_(const std::string &id, uint32_t source_crc32,
                        uint16_t width, uint16_t height) const;
  int find_ram_cache_index_(const std::string &id, uint32_t source_crc32,
                            uint16_t width, uint16_t height) const;
  void remember_rgb565_cache_(const std::string &id, uint32_t source_crc32,
                              uint16_t width, uint16_t height,
                              const uint8_t *buffer, size_t size);
  void erase_ram_caches_for_id_(const std::string &id);
  void erase_caches_for_id_(const std::string &id);
  bool upload_reserved_(const CardImageUpload &upload) const;
  void release_upload_(const CardImageUpload &upload);
  int find_index_(const std::string &id) const;
  static uint32_t crc32_update_(uint32_t crc, const uint8_t *data, size_t size);

  const esp_partition_t *partition_cache_{nullptr};
  bool partition_attempted_{false};
  bool index_loaded_{false};
  bool persistent_index_active_{false};
  uint32_t index_generation_{0};
  size_t active_index_slot_{0};
  std::vector<CardImageInfo> images_{};
  std::vector<CardImageCacheInfo> caches_{};
  struct UploadReservation {
    std::string id;
    layout::RecordSpan span;
  };
  std::vector<UploadReservation> upload_reservations_{};
  std::vector<std::pair<size_t, std::string>> recovered_index_names_{};
  struct RamImageCache {
    std::string id;
    uint32_t source_crc32{0};
    uint16_t width{0};
    uint16_t height{0};
    uint8_t *data{nullptr};
    size_t size{0};
    uint32_t last_used{0};
  };
  std::vector<RamImageCache> ram_caches_{};
  size_t ram_cache_bytes_{0};
  uint32_t ram_cache_clock_{0};
  std::vector<std::pair<std::string, size_t>> readers_{};
  std::string erase_reservation_{};
  SemaphoreHandle_t mutex_{nullptr};
};

}  // namespace esphome::card_image_store
