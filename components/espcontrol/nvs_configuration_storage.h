#pragma once

#include <cstddef>
#include <cstdint>

#include "configuration_store.h"
#include "nvs.h"

namespace espcontrol::configuration {

// A fixed-capacity, chunked NVS backend for the two-slot configuration store.
// The capacity is deliberately independent of the current document size so a
// long edit session cannot grow the allocator state or the number of keys.
class NvsConfigurationStorage final : public StorageBackend {
 public:
  static constexpr size_t CHUNK_SIZE = 1024;
  static constexpr size_t SLOT_CAPACITY = 64 * 1024;
  static constexpr size_t CHUNKS_PER_SLOT = SLOT_CAPACITY / CHUNK_SIZE;

  NvsConfigurationStorage() = default;
  ~NvsConfigurationStorage() override;

  bool setup();
  void close();
  bool ready() const { return handle_ != 0; }

  size_t slot_capacity() const override { return SLOT_CAPACITY; }
  bool read(uint8_t slot, size_t offset, uint8_t *output,
            size_t size) override;
  bool write(uint8_t slot, size_t offset, const uint8_t *data,
             size_t size) override;
  bool sync() override;

 private:
  bool valid_range(uint8_t slot, size_t offset, size_t size) const;
  bool read_chunk(uint8_t slot, size_t chunk);
  bool write_chunk(uint8_t slot, size_t chunk);
  void make_key(uint8_t slot, size_t chunk, char *output,
                size_t output_size) const;

  nvs_handle_t handle_{0};
  uint8_t chunk_[CHUNK_SIZE]{};
};

}  // namespace espcontrol::configuration
