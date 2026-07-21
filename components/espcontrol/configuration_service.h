#pragma once

#include <cstddef>
#include <cstdint>

#include "configuration_store.h"

namespace espcontrol::configuration {

constexpr uint16_t CURRENT_CONFIGURATION_DOCUMENT_VERSION = 1;
constexpr size_t CONFIGURATION_DOCUMENT_HEADER_SIZE = 8;

enum class LegacyStatus : uint8_t {
  OK,
  EMPTY,
  BUFFER_TOO_SMALL,
  READ_FAILED,
};

struct LegacyLoadResult {
  LegacyStatus status{LegacyStatus::EMPTY};
  uint16_t document_version{CURRENT_CONFIGURATION_DOCUMENT_VERSION};
  size_t document_size{0};
};

// Compatibility adapter for the existing ESPHome preference-backed entities.
// During the rollout it assembles those fields into one document on first
// boot, and mirrors later document saves so older firmware can still read the
// latest configuration.
class LegacyConfigurationAdapter {
 public:
  virtual ~LegacyConfigurationAdapter() = default;

  virtual LegacyLoadResult load(uint8_t *output, size_t output_capacity) = 0;
  virtual bool mirror(uint16_t document_version, const uint8_t *document,
                      size_t document_size) = 0;
};

enum class ServiceStatus : uint8_t {
  OK,
  IMPORTED_LEGACY,
  EMPTY,
  INVALID_ARGUMENT,
  BUFFER_TOO_SMALL,
  UNSUPPORTED_VERSION,
  INVALID_DOCUMENT,
  REVISION_CONFLICT,
  STORE_FAILED,
  LEGACY_READ_FAILED,
  LEGACY_MIRROR_FAILED,
};

struct ServiceLoadResult {
  ServiceStatus status{ServiceStatus::EMPTY};
  StoreStatus store_status{StoreStatus::EMPTY};
  uint16_t document_version{0};
  uint32_t generation{0};
  size_t document_size{0};

  bool ok() const {
    return status == ServiceStatus::OK ||
           status == ServiceStatus::IMPORTED_LEGACY;
  }
  bool imported_legacy() const {
    return status == ServiceStatus::IMPORTED_LEGACY;
  }
};

struct ServiceSaveResult {
  ServiceStatus status{ServiceStatus::STORE_FAILED};
  StoreStatus store_status{StoreStatus::EMPTY};
  uint16_t document_version{0};
  uint32_t generation{0};
  size_t document_size{0};

  bool ok() const { return status == ServiceStatus::OK; }
  bool durable() const {
    return status == ServiceStatus::OK ||
           status == ServiceStatus::LEGACY_MIRROR_FAILED;
  }
};

// Owns the transition between legacy preference fields and the versioned,
// checksummed document. The durable store is always committed before the
// compatibility mirror, so an interrupted legacy write cannot lose the new
// source of truth.
class ConfigurationService {
 public:
  ConfigurationService(ConfigurationStore &store,
                       LegacyConfigurationAdapter &legacy)
      : store_(store), legacy_(legacy) {}
  ConfigurationService(ConfigurationStore &store,
                       LegacyConfigurationAdapter &legacy, uint8_t *scratch,
                       size_t scratch_capacity)
      : store_(store),
        legacy_(legacy),
        scratch_(scratch),
        scratch_capacity_(scratch_capacity) {}

  ServiceLoadResult load(uint8_t *output, size_t output_capacity);
  ServiceSaveResult save(uint16_t document_version, const uint8_t *document,
                         size_t document_size);
  // Commits only when expected_revision still matches the durable snapshot.
  // Revision zero represents an empty store. On conflict, generation contains
  // the current durable revision so the caller can fetch a fresh snapshot.
  ServiceSaveResult save_if_revision(uint32_t expected_revision,
                                     uint16_t document_version,
                                     const uint8_t *document,
                                     size_t document_size);
  ServiceSaveResult save_current_if_revision(uint32_t expected_revision,
                                             const uint8_t *document,
                                             size_t document_size) {
    return save_if_revision(expected_revision,
                            CURRENT_CONFIGURATION_DOCUMENT_VERSION, document,
                            document_size);
  }
  ServiceSaveResult save_current(const uint8_t *document,
                                 size_t document_size) {
    return save(CURRENT_CONFIGURATION_DOCUMENT_VERSION, document,
                document_size);
  }

  size_t maximum_document_size() const;

  // Installs the fixed transaction workspace before the service is first
  // used. Firmware supplies this from PSRAM; host tools may keep the vector
  // fallback by leaving it unset.
  void use_scratch(uint8_t *scratch, size_t scratch_capacity) {
    scratch_ = scratch;
    scratch_capacity_ = scratch_capacity;
  }

 private:
  CommitResult commit_document(uint16_t document_version,
                               const uint8_t *document,
                               size_t document_size);
  ServiceLoadResult ensure_snapshot_exists();

  ConfigurationStore &store_;
  LegacyConfigurationAdapter &legacy_;
  uint8_t *scratch_{nullptr};
  size_t scratch_capacity_{0};
};

}  // namespace espcontrol::configuration
