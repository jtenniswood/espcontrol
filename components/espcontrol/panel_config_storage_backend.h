#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "configuration_store.h"

namespace espcontrol::configuration {

// Narrow persistence port for the ESP-IDF NVS binding. Keeping it separate
// lets the two-slot protocol have host coverage without pulling ESP-IDF into
// firmware unit tests.
enum class BlobLoadStatus : uint8_t { OK, MISSING, FAILED };

struct BlobLoadResult {
  BlobLoadStatus status{BlobLoadStatus::FAILED};
  size_t size{0};
};

class BlobStorage {
 public:
  virtual ~BlobStorage() = default;

  // Loads at most capacity bytes and reports the durable blob length. The
  // remaining buffer bytes must be filled with an erased value by the binding.
  virtual BlobLoadResult load_blob(uint8_t slot, uint8_t *output,
                                   size_t capacity) = 0;
  virtual bool save_blob(uint8_t slot, const uint8_t *data, size_t size) = 0;
  virtual bool sync() = 0;
};

// Adapts full-slot blobs to the small byte-range operations used by
// ConfigurationStore. Both slots live in fixed memory; writes are accumulated
// until ConfigurationStore's explicit sync boundary makes them durable.
template <size_t SlotCapacity>
class BufferedBlobStorageBackend final : public StorageBackend {
 public:
  static_assert(SlotCapacity >= CONFIGURATION_ENVELOPE_HEADER_SIZE,
                "Configuration slot must hold its envelope");

  explicit BufferedBlobStorageBackend(BlobStorage &storage)
      : storage_(storage) {}

  bool begin(uint8_t *slot_storage, size_t slot_storage_size,
             size_t slot_capacity = SlotCapacity) {
    if (slot_storage == nullptr ||
        slot_capacity < CONFIGURATION_ENVELOPE_HEADER_SIZE ||
        slot_capacity > SlotCapacity ||
        slot_storage_size < CONFIGURATION_SLOT_COUNT * slot_capacity)
      return false;
    slot_capacity_ = slot_capacity;
    for (uint8_t slot = 0; slot < CONFIGURATION_SLOT_COUNT; ++slot) {
      slots_[slot] = slot_storage + static_cast<size_t>(slot) * slot_capacity_;
    }
    loaded_.fill(false);
    dirty_.fill(false);
    stored_sizes_.fill(0);
    return true;
  }

  size_t slot_capacity() const override { return slot_capacity_; }

  bool read(uint8_t slot, size_t offset, uint8_t *output,
            size_t size) override {
    if (!ready() || !range_is_valid(slot, offset, size) ||
        (size > 0 && output == nullptr) ||
        !load_slot(slot)) {
      return false;
    }
    if (size > 0) std::memcpy(output, slots_[slot] + offset, size);
    return true;
  }

  bool write(uint8_t slot, size_t offset, const uint8_t *data,
             size_t size) override {
    if (!ready() || !range_is_valid(slot, offset, size) ||
        (size > 0 && data == nullptr) ||
        !load_slot(slot)) {
      return false;
    }
    // ConfigurationStore clears the publication marker before replacing a
    // slot. Persist that small invalid marker immediately rather than retain a
    // previous, potentially much larger blob allocation in NVS.
    if (offset == 0 && size == sizeof(uint32_t) &&
        std::all_of(data, data + size,
                    [](uint8_t value) { return value == 0; })) {
      // A full-blob binding may need to rewrite an erased flash slot during
      // the following sync boundaries.  Do not retain stale header bytes in
      // the in-memory image after ConfigurationStore withdraws publication.
      // Besides keeping compact NVS writes compact, this makes the next
      // payload and metadata writes safe for a raw-flash implementation.
      std::memset(slots_[slot], 0xFF, slot_capacity_);
      stored_sizes_[slot] = 0;
    }
    if (size > 0) std::memcpy(slots_[slot] + offset, data, size);
    stored_sizes_[slot] = std::max(stored_sizes_[slot], offset + size);
    dirty_[slot] = true;
    return true;
  }

  bool sync() override {
    for (uint8_t slot = 0; slot < CONFIGURATION_SLOT_COUNT; ++slot) {
      if (!dirty_[slot]) continue;
      if (!storage_.save_blob(slot, slots_[slot], stored_sizes_[slot]))
        return false;
    }
    if (!storage_.sync()) return false;
    dirty_.fill(false);
    return true;
  }

 private:
  bool range_is_valid(uint8_t slot, size_t offset, size_t size) const {
    return slot < CONFIGURATION_SLOT_COUNT && offset <= slot_capacity_ &&
           size <= slot_capacity_ - offset;
  }

  bool ready() const {
    for (uint8_t slot = 0; slot < CONFIGURATION_SLOT_COUNT; ++slot) {
      if (slots_[slot] == nullptr) return false;
    }
    return true;
  }

  bool load_slot(uint8_t slot) {
    if (loaded_[slot]) return true;
    const BlobLoadResult result =
        storage_.load_blob(slot, slots_[slot], slot_capacity_);
    if (result.status == BlobLoadStatus::FAILED ||
        result.size > slot_capacity_) {
      return false;
    }
    if (result.status == BlobLoadStatus::MISSING)
      std::memset(slots_[slot], 0xFF, slot_capacity_);
    stored_sizes_[slot] = result.size;
    loaded_[slot] = true;
    return true;
  }

  BlobStorage &storage_;
  size_t slot_capacity_{SlotCapacity};
  std::array<uint8_t *, CONFIGURATION_SLOT_COUNT> slots_{};
  std::array<bool, CONFIGURATION_SLOT_COUNT> loaded_{};
  std::array<bool, CONFIGURATION_SLOT_COUNT> dirty_{};
  std::array<size_t, CONFIGURATION_SLOT_COUNT> stored_sizes_{};
};

}  // namespace espcontrol::configuration
