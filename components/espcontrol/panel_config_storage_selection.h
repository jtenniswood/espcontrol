#pragma once

#include <cstddef>

namespace espcontrol::configuration {

// Prefer the larger card-images partition when a profile requests it, but
// preserve native configuration on OTA-upgraded panels whose older partition
// table does not contain that optional partition. Their existing NVS
// partition is large enough for the two configuration blobs.
template<typename Storage>
bool begin_panel_config_storage(Storage &storage, bool prefer_card_images,
                                size_t slot_capacity,
                                bool *used_nvs_fallback = nullptr) {
  if (used_nvs_fallback != nullptr) *used_nvs_fallback = false;
  if (!prefer_card_images) return storage.begin();
  if (storage.begin_card_images_partition(slot_capacity)) return true;
  if (used_nvs_fallback != nullptr) *used_nvs_fallback = true;
  return storage.begin();
}

}  // namespace espcontrol::configuration
