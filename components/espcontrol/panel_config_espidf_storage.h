#pragma once

#include <cstddef>
#include <cstdint>

#include "panel_config_storage_backend.h"

#ifdef USE_ESP32
#include <nvs.h>
#include <esp_partition.h>
#endif

namespace espcontrol::configuration {

// ESP-IDF NVS binding. It uses its own namespace in ESPHome's existing NVS
// partition, so it neither collides with restored text entities nor requires
// a partition-table migration during the compatibility releases.
class EspIdfPanelConfigBlobStorage final : public BlobStorage {
 public:
  // Existing profiles keep their compact NVS backing. Profiles with a large
  // fixed-capacity document can opt into the reserved beginning of the
  // already-deployed card_images partition without changing its layout.
  bool begin();
  bool begin_card_images_partition(size_t slot_capacity);
  BlobLoadResult load_blob(uint8_t slot, uint8_t *output,
                           size_t capacity) override;
  bool save_blob(uint8_t slot, const uint8_t *data, size_t size) override;
  bool sync() override;

 private:
  static const char *slot_key(uint8_t slot);

#ifdef USE_ESP32
  static constexpr const char *CARD_IMAGES_PARTITION_LABEL = "card_images";
  static constexpr size_t FLASH_ERASE_SIZE = 4096;
  static constexpr size_t PARTITION_SLOT_HEADER_SIZE = 16;
  static constexpr uint32_t PARTITION_COMMIT_MARKER = 0x50434647;

  size_t partition_slot_stride() const;
  size_t partition_slot_offset(uint8_t slot) const;
  bool load_partition_blob(uint8_t slot, uint8_t *output, size_t capacity);
  bool save_partition_blob(uint8_t slot, const uint8_t *data, size_t size);
  bool erase_partition_slot(uint8_t slot);

  nvs_handle_t handle_{0};
  const esp_partition_t *partition_{nullptr};
  size_t partition_slot_capacity_{0};
  bool partition_backed_{false};
#endif
  bool ready_{false};
};

}  // namespace espcontrol::configuration
