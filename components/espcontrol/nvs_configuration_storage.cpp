#include "nvs_configuration_storage.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "esp_err.h"
#include "esphome/core/log.h"

namespace espcontrol::configuration {
namespace {

constexpr char TAG[] = "espcontrol.config";
constexpr char NVS_NAMESPACE[] = "espctl_cfg";

}  // namespace

NvsConfigurationStorage::~NvsConfigurationStorage() { close(); }

bool NvsConfigurationStorage::setup() {
  if (ready()) return true;
  const esp_err_t result = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle_);
  if (result != ESP_OK) {
    handle_ = 0;
    ESP_LOGE(TAG, "Unable to open configuration storage: %s",
             esp_err_to_name(result));
    return false;
  }
  return true;
}

void NvsConfigurationStorage::close() {
  if (!ready()) return;
  nvs_close(handle_);
  handle_ = 0;
}

bool NvsConfigurationStorage::valid_range(uint8_t slot, size_t offset,
                                          size_t size) const {
  return ready() && slot < CONFIGURATION_SLOT_COUNT &&
         offset <= SLOT_CAPACITY && size <= SLOT_CAPACITY - offset;
}

void NvsConfigurationStorage::make_key(uint8_t slot, size_t chunk,
                                       char *output,
                                       size_t output_size) const {
  std::snprintf(output, output_size, "s%u_%02x",
                static_cast<unsigned>(slot),
                static_cast<unsigned>(chunk));
}

bool NvsConfigurationStorage::read_chunk(uint8_t slot, size_t chunk) {
  char key[8];
  make_key(slot, chunk, key, sizeof(key));
  size_t stored_size = sizeof(chunk_);
  const esp_err_t result = nvs_get_blob(handle_, key, chunk_, &stored_size);
  if (result == ESP_ERR_NVS_NOT_FOUND) {
    std::memset(chunk_, 0xFF, sizeof(chunk_));
    return true;
  }
  if (result != ESP_OK || stored_size != sizeof(chunk_)) {
    ESP_LOGE(TAG, "Unable to read configuration chunk %s: %s", key,
             esp_err_to_name(result));
    return false;
  }
  return true;
}

bool NvsConfigurationStorage::write_chunk(uint8_t slot, size_t chunk) {
  char key[8];
  make_key(slot, chunk, key, sizeof(key));
  const esp_err_t result = nvs_set_blob(handle_, key, chunk_, sizeof(chunk_));
  if (result != ESP_OK) {
    ESP_LOGE(TAG, "Unable to write configuration chunk %s: %s", key,
             esp_err_to_name(result));
    return false;
  }
  return true;
}

bool NvsConfigurationStorage::read(uint8_t slot, size_t offset,
                                   uint8_t *output, size_t size) {
  if (!valid_range(slot, offset, size) || (size > 0 && output == nullptr)) {
    return false;
  }
  while (size > 0) {
    const size_t chunk_index = offset / CHUNK_SIZE;
    const size_t chunk_offset = offset % CHUNK_SIZE;
    const size_t amount = std::min(size, CHUNK_SIZE - chunk_offset);
    if (!read_chunk(slot, chunk_index)) return false;
    std::memcpy(output, chunk_ + chunk_offset, amount);
    output += amount;
    offset += amount;
    size -= amount;
  }
  return true;
}

bool NvsConfigurationStorage::write(uint8_t slot, size_t offset,
                                    const uint8_t *data, size_t size) {
  if (!valid_range(slot, offset, size) || (size > 0 && data == nullptr)) {
    return false;
  }
  while (size > 0) {
    const size_t chunk_index = offset / CHUNK_SIZE;
    const size_t chunk_offset = offset % CHUNK_SIZE;
    const size_t amount = std::min(size, CHUNK_SIZE - chunk_offset);
    if (chunk_offset == 0 && amount == CHUNK_SIZE) {
      std::memcpy(chunk_, data, CHUNK_SIZE);
    } else {
      if (!read_chunk(slot, chunk_index)) return false;
      std::memcpy(chunk_ + chunk_offset, data, amount);
    }
    if (!write_chunk(slot, chunk_index)) return false;
    data += amount;
    offset += amount;
    size -= amount;
  }
  return true;
}

bool NvsConfigurationStorage::sync() {
  return ready() && nvs_commit(handle_) == ESP_OK;
}

}  // namespace espcontrol::configuration
