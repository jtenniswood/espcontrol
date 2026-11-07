#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "panel_config_storage_backend.h"

namespace espcontrol::configuration {

// Raw flash needs no slot-sized RAM. NVS exposes only whole blobs, so borrow
// one slot image during a store operation and release it before returning.
// No document, preference key, publication boundary or flash layout changes.
template <size_t MaximumCapacity>
class TransientBlobStorageBackend final : public StorageBackend {
public:
  using Allocate = uint8_t *(*)(size_t);
  using Release = void (*)(uint8_t *);

  TransientBlobStorageBackend(BlobStorage &storage, Allocate allocate,
                              Release release)
      : storage_(storage), allocate_(allocate), release_(release) {}
  TransientBlobStorageBackend(const TransientBlobStorageBackend &) = delete;
  TransientBlobStorageBackend &
  operator=(const TransientBlobStorageBackend &) = delete;
  ~TransientBlobStorageBackend() override { end_operation(); }

  bool begin(size_t capacity) {
    if (active_ || capacity < CONFIGURATION_ENVELOPE_HEADER_SIZE ||
        capacity > MaximumCapacity || allocate_ == nullptr ||
        release_ == nullptr)
      return false;
    capacity_ = capacity;
    return true;
  }

  size_t slot_capacity() const override { return capacity_; }

  bool begin_operation() override {
    operation_failure_ = StoreStatus::READ_FAILED;
    if (active_ || capacity_ == 0)
      return false;
    if (!storage_.supports_range_io()) {
      buffer_ = allocate_(capacity_);
      if (buffer_ == nullptr) {
        operation_failure_ = StoreStatus::ALLOCATION_FAILED;
        return false;
      }
    }
    active_ = true;
    return true;
  }

  StoreStatus operation_failure() const override { return operation_failure_; }

  void end_operation() override {
    if (buffer_ != nullptr)
      release_(buffer_);
    buffer_ = nullptr;
    active_ = false;
    dirty_ = false;
    loaded_slot_ = UINT8_MAX;
    stored_size_ = 0;
  }

  bool read(uint8_t slot, size_t offset, uint8_t *output,
            size_t size) override {
    if (!valid(slot, offset, size) || (size > 0 && output == nullptr))
      return false;
    if (storage_.supports_range_io())
      return storage_.read_range(slot, offset, output, size);
    if (!load(slot))
      return false;
    if (size > 0)
      std::memcpy(output, buffer_ + offset, size);
    return true;
  }

  bool write(uint8_t slot, size_t offset, const uint8_t *data,
             size_t size) override {
    if (!valid(slot, offset, size) || (size > 0 && data == nullptr))
      return false;
    if (storage_.supports_range_io())
      return storage_.write_range(slot, offset, data, size);
    if (!load(slot))
      return false;
    if (offset == 0 && size == sizeof(uint32_t) &&
        std::all_of(data, data + size,
                    [](uint8_t value) { return value == 0; })) {
      std::memset(buffer_, 0xFF, capacity_);
      stored_size_ = 0;
    }
    if (size > 0)
      std::memcpy(buffer_ + offset, data, size);
    stored_size_ = std::max(stored_size_, offset + size);
    dirty_ = true;
    return true;
  }

  bool sync() override {
    if (!active_)
      return false;
    if (dirty_ && !storage_.save_blob(loaded_slot_, buffer_, stored_size_))
      return false;
    if (!storage_.sync())
      return false;
    dirty_ = false;
    return true;
  }

private:
  bool valid(uint8_t slot, size_t offset, size_t size) const {
    return active_ && slot < CONFIGURATION_SLOT_COUNT && offset <= capacity_ &&
           size <= capacity_ - offset;
  }

  bool load(uint8_t slot) {
    if (loaded_slot_ == slot)
      return true;
    // ConfigurationStore inspects both slots before writing one target. Do
    // not accidentally discard a caller's unsynchronized different slot.
    if (dirty_ || buffer_ == nullptr)
      return false;
    const auto result = storage_.load_blob(slot, buffer_, capacity_);
    loaded_slot_ = UINT8_MAX;
    if (result.status == BlobLoadStatus::FAILED || result.size > capacity_)
      return false;
    if (result.status == BlobLoadStatus::MISSING)
      std::memset(buffer_, 0xFF, capacity_);
    stored_size_ = result.status == BlobLoadStatus::MISSING ? 0 : result.size;
    loaded_slot_ = slot;
    return true;
  }

  BlobStorage &storage_;
  Allocate allocate_;
  Release release_;
  uint8_t *buffer_{nullptr};
  size_t capacity_{0};
  size_t stored_size_{0};
  uint8_t loaded_slot_{UINT8_MAX};
  StoreStatus operation_failure_{StoreStatus::READ_FAILED};
  bool active_{false};
  bool dirty_{false};
};

} // namespace espcontrol::configuration
