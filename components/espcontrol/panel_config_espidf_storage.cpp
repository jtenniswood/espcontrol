#include "panel_config_espidf_storage.h"

#include <algorithm>
#include <cstring>

#ifdef USE_ESP32
#include <esp_err.h>
#include <nvs.h>
#include <esp_partition.h>
#endif

namespace espcontrol::configuration {

const char *EspIdfPanelConfigBlobStorage::slot_key(uint8_t slot) {
  return slot == 0 ? "slot_a" : (slot == 1 ? "slot_b" : nullptr);
}

bool EspIdfPanelConfigBlobStorage::begin() {
#ifdef USE_ESP32
  if (ready_) return true;
  if (nvs_open("espcontrol_cfg", NVS_READWRITE, &handle_) != ESP_OK)
    return false;
  ready_ = true;
  return true;
#else
  return false;
#endif
}

bool EspIdfPanelConfigBlobStorage::begin_card_images_partition(
    size_t slot_capacity) {
#ifdef USE_ESP32
  if (ready_ || slot_capacity == 0) return false;
  const esp_partition_t *const partition = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY,
      CARD_IMAGES_PARTITION_LABEL);
  if (partition == nullptr) return false;

  partition_slot_capacity_ = slot_capacity;
  if (partition_slot_stride() > partition->size / 2) {
    partition_slot_capacity_ = 0;
    return false;
  }
  partition_ = partition;
  partition_backed_ = true;
  ready_ = true;
  return true;
#else
  (void) slot_capacity;
  return false;
#endif
}

#ifdef USE_ESP32
size_t EspIdfPanelConfigBlobStorage::partition_slot_stride() const {
  const size_t unaligned = PARTITION_SLOT_HEADER_SIZE + partition_slot_capacity_;
  return ((unaligned + FLASH_ERASE_SIZE - 1) / FLASH_ERASE_SIZE) *
         FLASH_ERASE_SIZE;
}

size_t EspIdfPanelConfigBlobStorage::partition_slot_offset(uint8_t slot) const {
  return static_cast<size_t>(slot) * partition_slot_stride();
}

bool EspIdfPanelConfigBlobStorage::erase_partition_slot(uint8_t slot) {
  return partition_ != nullptr && slot < 2 &&
         esp_partition_erase_range(partition_, partition_slot_offset(slot),
                                   partition_slot_stride()) == ESP_OK;
}

bool EspIdfPanelConfigBlobStorage::load_partition_blob(uint8_t slot,
                                                        uint8_t *output,
                                                        size_t capacity) {
  if (partition_ == nullptr || slot >= 2 || output == nullptr ||
      capacity != partition_slot_capacity_)
    return false;
  uint32_t marker = 0;
  const size_t offset = partition_slot_offset(slot);
  if (esp_partition_read(partition_, offset, &marker, sizeof(marker)) != ESP_OK)
    return false;
  if (marker != PARTITION_COMMIT_MARKER) return false;
  return esp_partition_read(partition_, offset + PARTITION_SLOT_HEADER_SIZE,
                            output, capacity) == ESP_OK;
}

bool EspIdfPanelConfigBlobStorage::save_partition_blob(uint8_t slot,
                                                        const uint8_t *data,
                                                        size_t size) {
  if (partition_ == nullptr || slot >= 2 ||
      (size > 0 && data == nullptr) || size > partition_slot_capacity_)
    return false;

  const size_t offset = partition_slot_offset(slot);
  const bool unpublished = size == sizeof(uint32_t) && data != nullptr &&
                           data[0] == 0 && data[1] == 0 && data[2] == 0 &&
                           data[3] == 0;
  if (unpublished) return erase_partition_slot(slot);
  if (size < sizeof(uint32_t) || data == nullptr) return false;

  // ConfigurationStore persists payload, metadata and its own magic marker
  // through separate sync boundaries.  Keep our slot unpublished until the
  // final marker is written: a power loss at any earlier point is therefore
  // ignored and the other atomic slot remains authoritative.
  const bool store_magic_is_clear =
      data[0] == 0 && data[1] == 0 && data[2] == 0 && data[3] == 0;
  if (store_magic_is_clear) {
    const bool metadata_is_erased = size >= PARTITION_SLOT_HEADER_SIZE &&
        std::all_of(data + sizeof(uint32_t),
                    data + PARTITION_SLOT_HEADER_SIZE,
                    [](uint8_t value) { return value == 0xFF; });
    const size_t payload_offset = PARTITION_SLOT_HEADER_SIZE;
    if (metadata_is_erased) {
      if (size <= payload_offset) return true;
      return esp_partition_write(partition_,
                                 offset + PARTITION_SLOT_HEADER_SIZE +
                                     payload_offset,
                                 data + payload_offset,
                                 size - payload_offset) == ESP_OK;
    }
    return esp_partition_write(partition_,
                               offset + PARTITION_SLOT_HEADER_SIZE +
                                   sizeof(uint32_t),
                               data + sizeof(uint32_t),
                               size - sizeof(uint32_t)) == ESP_OK;
  }

  if (esp_partition_write(partition_, offset + PARTITION_SLOT_HEADER_SIZE,
                          data, size) != ESP_OK)
    return false;
  return esp_partition_write(partition_, offset, &PARTITION_COMMIT_MARKER,
                             sizeof(PARTITION_COMMIT_MARKER)) == ESP_OK;
}
#endif

BlobLoadResult EspIdfPanelConfigBlobStorage::load_blob(uint8_t slot,
                                                        uint8_t *output,
                                                        size_t capacity) {
#ifdef USE_ESP32
  const char *key = slot_key(slot);
  if (!ready_ || key == nullptr || (capacity > 0 && output == nullptr))
    return {BlobLoadStatus::FAILED};
  if (partition_backed_) {
    if (capacity != partition_slot_capacity_) return {BlobLoadStatus::FAILED};
    return load_partition_blob(slot, output, capacity)
               ? BlobLoadResult{BlobLoadStatus::OK, capacity}
               : BlobLoadResult{BlobLoadStatus::MISSING};
  }
  size_t stored_size = 0;
  esp_err_t result = nvs_get_blob(handle_, key, nullptr, &stored_size);
  if (result == ESP_ERR_NVS_NOT_FOUND) return {BlobLoadStatus::MISSING};
  if (result != ESP_OK || stored_size > capacity)
    return {BlobLoadStatus::FAILED};
  std::memset(output, 0xFF, capacity);
  result = nvs_get_blob(handle_, key, output, &stored_size);
  return result == ESP_OK ? BlobLoadResult{BlobLoadStatus::OK, stored_size}
                          : BlobLoadResult{BlobLoadStatus::FAILED};
#else
  (void) slot;
  (void) output;
  (void) capacity;
  return {BlobLoadStatus::FAILED};
#endif
}

bool EspIdfPanelConfigBlobStorage::save_blob(uint8_t slot, const uint8_t *data,
                                              size_t size) {
#ifdef USE_ESP32
  const char *key = slot_key(slot);
  if (partition_backed_)
    return ready_ && save_partition_blob(slot, data, size);
  return ready_ && key != nullptr && (size == 0 || data != nullptr) &&
         nvs_set_blob(handle_, key, data, size) == ESP_OK;
#else
  (void) slot;
  (void) data;
  (void) size;
  return false;
#endif
}

bool EspIdfPanelConfigBlobStorage::sync() {
#ifdef USE_ESP32
  if (partition_backed_) return ready_;
  return ready_ && nvs_commit(handle_) == ESP_OK;
#else
  return false;
#endif
}

}  // namespace espcontrol::configuration
