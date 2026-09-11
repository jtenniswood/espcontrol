#pragma once
#include <cstddef>
namespace espcontrol {
// The deployed 2 MiB card_images partition stores configuration at the front.
// Reserve the final four sectors for an independent NVS store for panel identity.
constexpr size_t PANEL_IDENTITY_STORAGE_BYTES = 16 * 1024;
constexpr size_t PANEL_DATA_PARTITION_BYTES = 2 * 1024 * 1024;
inline size_t panel_config_partition_bytes(size_t size) {
  return size == PANEL_DATA_PARTITION_BYTES ? size - PANEL_IDENTITY_STORAGE_BYTES : 0;
}
}  // namespace espcontrol
