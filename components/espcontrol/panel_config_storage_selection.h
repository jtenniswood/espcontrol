#pragma once

#include <cstddef>

namespace espcontrol::configuration {

struct PanelConfigStorageSelection {
  bool ready{false};
  size_t slot_capacity{0};
  bool used_nvs_fallback{false};
};

// Prefer the larger card-images partition when a profile requests it, but
// preserve native configuration on OTA-upgraded panels whose older partition
// table does not contain that optional partition. NVS uses a smaller capacity
// so its two atomic blobs leave room for NVS metadata and ESPHome preferences.
template <typename Storage>
PanelConfigStorageSelection begin_panel_config_storage(
    Storage &storage, bool prefer_card_images,
    size_t card_images_slot_capacity, size_t nvs_slot_capacity) {
  if (!prefer_card_images) {
    const bool ready = nvs_slot_capacity > 0 && storage.begin();
    return {ready, ready ? nvs_slot_capacity : 0, false};
  }
  if (storage.begin_card_images_partition(card_images_slot_capacity))
    return {true, card_images_slot_capacity, false};
  const bool ready = nvs_slot_capacity > 0 && storage.begin();
  return {ready, ready ? nvs_slot_capacity : 0, true};
}

}  // namespace espcontrol::configuration
