#pragma once

#include <cstddef>

namespace espcontrol::configuration {

// NVS stores variable-length blobs as 32-byte entries plus index/chunk
// metadata. Keep a conservative entry budget for each 1 KiB configuration
// chunk and leave a fixed reserve for ESPHome preferences and other users of
// the shared default NVS partition.
struct NvsConfigurationCapacity {
  static constexpr size_t RETAINED_SLOT_COUNT = 2;
  static constexpr size_t CHUNK_SIZE = 1024;
  static constexpr size_t MAX_SLOT_CAPACITY = 64 * 1024;
  static constexpr size_t MAX_CHUNKS_PER_SLOT =
      MAX_SLOT_CAPACITY / CHUNK_SIZE;
  static constexpr size_t NVS_ENTRY_SIZE = 32;
  static constexpr size_t NVS_ENTRIES_PER_CHUNK =
      2 + (CHUNK_SIZE + NVS_ENTRY_SIZE - 1) / NVS_ENTRY_SIZE;
  static constexpr size_t RESERVED_SHARED_ENTRIES = 512;

  static constexpr size_t slot_capacity_for_entry_budget(
      size_t reclaimable_entries) {
    if (reclaimable_entries <= RESERVED_SHARED_ENTRIES) return 0;
    const size_t configuration_entries =
        reclaimable_entries - RESERVED_SHARED_ENTRIES;
    size_t chunks_per_slot =
        configuration_entries /
        (RETAINED_SLOT_COUNT * NVS_ENTRIES_PER_CHUNK);
    if (chunks_per_slot > MAX_CHUNKS_PER_SLOT) {
      chunks_per_slot = MAX_CHUNKS_PER_SLOT;
    }
    return chunks_per_slot * CHUNK_SIZE;
  }

  static constexpr size_t entries_for_capacity(size_t slot_capacity) {
    const size_t chunks_per_slot =
        (slot_capacity + CHUNK_SIZE - 1) / CHUNK_SIZE;
    return RETAINED_SLOT_COUNT * chunks_per_slot * NVS_ENTRIES_PER_CHUNK;
  }
};

}  // namespace espcontrol::configuration
