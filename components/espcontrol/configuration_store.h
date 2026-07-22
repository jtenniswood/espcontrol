#pragma once

#include <cstddef>
#include <cstdint>

namespace espcontrol::configuration {

constexpr uint8_t CONFIGURATION_SLOT_COUNT = 2;
constexpr size_t CONFIGURATION_ENVELOPE_HEADER_SIZE = 20;
constexpr uint16_t CONFIGURATION_ENVELOPE_VERSION = 1;

enum class StoreStatus : uint8_t {
  OK,
  EMPTY,
  INVALID_ARGUMENT,
  BUFFER_TOO_SMALL,
  PAYLOAD_TOO_LARGE,
  READ_FAILED,
  WRITE_FAILED,
  SYNC_FAILED,
  VERIFY_FAILED,
};

struct LoadResult {
  StoreStatus status{StoreStatus::EMPTY};
  uint32_t generation{0};
  size_t payload_size{0};
  uint8_t slot{UINT8_MAX};

  bool ok() const { return status == StoreStatus::OK; }
};

struct CommitResult {
  StoreStatus status{StoreStatus::WRITE_FAILED};
  uint32_t generation{0};
  size_t payload_size{0};
  uint8_t slot{UINT8_MAX};

  bool ok() const { return status == StoreStatus::OK; }
};

// Storage adapter used by the platform integration. Each slot must expose the
// same fixed capacity. Writes do not need to be atomic: ConfigurationStore
// writes payload bytes first and publishes the checksummed header last.
class StorageBackend {
 public:
  virtual ~StorageBackend() = default;

  virtual size_t slot_capacity() const = 0;
  virtual bool read(uint8_t slot, size_t offset, uint8_t *output,
                    size_t size) = 0;
  virtual bool write(uint8_t slot, size_t offset, const uint8_t *data,
                     size_t size) = 0;
  virtual bool sync() = 0;
};

// Durable opaque-payload store for the future versioned configuration
// document. The store deliberately knows nothing about JSON or card fields;
// document validation belongs at the configuration service boundary.
class ConfigurationStore {
 public:
  explicit ConfigurationStore(StorageBackend &backend) : backend_(backend) {}

  // Inspect the newest valid slot without copying its payload. This is used by
  // revision-aware writers before committing a replacement document.
  LoadResult inspect();
  LoadResult load(uint8_t *output, size_t output_capacity);
  CommitResult commit(const uint8_t *payload, size_t payload_size);

  size_t maximum_payload_size() const;

 private:
  enum class SlotState : uint8_t { VALID, INVALID, IO_ERROR };

  struct SlotMetadata {
    SlotState state{SlotState::INVALID};
    uint32_t generation{0};
    uint32_t checksum{0};
    size_t payload_size{0};
    uint8_t slot{UINT8_MAX};
  };

  SlotMetadata inspect_slot(uint8_t slot);
  bool checksum_slot_payload(uint8_t slot, const uint8_t *header,
                             size_t payload_size, uint32_t *checksum);
  static bool generation_is_newer(uint32_t candidate, uint32_t reference);
  static uint32_t checksum(uint32_t generation, const uint8_t *data,
                           size_t size);

  StorageBackend &backend_;
};

}  // namespace espcontrol::configuration
