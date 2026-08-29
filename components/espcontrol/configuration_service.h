#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

#include "configuration_store.h"

namespace espcontrol::configuration {

constexpr uint16_t CURRENT_CONFIGURATION_DOCUMENT_VERSION = 1;
constexpr size_t CONFIGURATION_DOCUMENT_HEADER_SIZE = 8;

// Release-controlled compatibility phases. The read/import-only phase is used
// after the two dual-write releases: older panels can still be upgraded, but
// native saves no longer overwrite their legacy text entities.
enum class LegacyConfigurationMode : uint8_t {
  DUAL_WRITE,
  READ_IMPORT_ONLY,
};

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

// Applies a validated native document to the currently running panel. This is
// deliberately separate from the legacy mirror: a panel can update its live
// grid without writing the legacy preference-backed entities.
class ConfigurationRuntimeAdapter {
 public:
  virtual ~ConfigurationRuntimeAdapter() = default;

  virtual bool apply(uint16_t document_version, const uint8_t *document,
                     size_t document_size) = 0;
};

enum class ServiceStatus : uint8_t {
  OK,
  IMPORTED_LEGACY,
  SYNCED_LEGACY,
  EMPTY,
  INVALID_ARGUMENT,
  BUFFER_TOO_SMALL,
  UNSUPPORTED_VERSION,
  INVALID_DOCUMENT,
  GENERATION_CONFLICT,
  STORE_FAILED,
  LEGACY_READ_FAILED,
  LEGACY_MIRROR_FAILED,
  RUNTIME_APPLY_FAILED,
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
           status == ServiceStatus::LEGACY_MIRROR_FAILED ||
           status == ServiceStatus::RUNTIME_APPLY_FAILED;
  }
};

// A service integration can select the native document it understands without
// changing the durable two-slot store. Leaving this unset preserves the
// compatibility service's existing v1 envelope behaviour during migration.
class ConfigurationDocumentValidator {
 public:
  virtual ~ConfigurationDocumentValidator() = default;

  virtual bool supports_version(uint16_t document_version) const = 0;
  virtual bool validate(uint16_t document_version, const uint8_t *document,
                        size_t document_size) const = 0;
};

// Owns the transition between legacy preference fields and the versioned,
// checksummed document. The durable store is always committed before the
// compatibility mirror, so an interrupted legacy write cannot lose the new
// source of truth.
class ConfigurationService {
 public:
  ConfigurationService(ConfigurationStore &store,
                       LegacyConfigurationAdapter &legacy,
      const ConfigurationDocumentValidator *validator = nullptr,
      uint8_t *scratch_buffer = nullptr, size_t scratch_capacity = 0,
      LegacyConfigurationMode legacy_mode = LegacyConfigurationMode::DUAL_WRITE)
      : store_(store), legacy_(legacy), validator_(validator),
        scratch_buffer_(scratch_buffer), scratch_capacity_(scratch_capacity),
        legacy_mode_(legacy_mode) {}

  ServiceLoadResult load(uint8_t *output, size_t output_capacity);
  // Restores the durable document and publishes it to the live panel as one
  // operation. This prevents an HTTP save from landing between a boot read
  // and its corresponding live apply.
  ServiceLoadResult load_and_apply_runtime(uint8_t *output,
                                           size_t output_capacity);
  // During compatibility releases the text entities remain authoritative.
  // Refresh the native shadow on boot so editor changes made through the
  // legacy API survive a later native-only firmware upgrade.
  ServiceLoadResult refresh_legacy_shadow(uint8_t *output,
                                          size_t output_capacity);
  ServiceSaveResult save(uint16_t document_version, const uint8_t *document,
                         size_t document_size);
  ServiceSaveResult save_if_generation(uint32_t expected_generation,
                                       uint16_t document_version,
                                       const uint8_t *document,
                                       size_t document_size);
  ServiceSaveResult save_current(const uint8_t *document,
                                 size_t document_size) {
    return save(CURRENT_CONFIGURATION_DOCUMENT_VERSION, document,
                document_size);
  }

  size_t maximum_document_size() const;
  bool legacy_writes_enabled() const {
    return legacy_mode_ == LegacyConfigurationMode::DUAL_WRITE;
  }
  void set_scratch_buffer(uint8_t *scratch_buffer, size_t scratch_capacity) {
    scratch_buffer_ = scratch_buffer;
    scratch_capacity_ = scratch_capacity;
  }
  void set_runtime_adapter(ConfigurationRuntimeAdapter *runtime) {
    runtime_ = runtime;
  }

 private:
  ServiceLoadResult load_unlocked(uint8_t *output, size_t output_capacity);
  ServiceLoadResult refresh_legacy_shadow_unlocked(uint8_t *output,
                                                   size_t output_capacity);
  ServiceSaveResult save_unlocked(uint16_t document_version,
                                  const uint8_t *document,
                                  size_t document_size);
  ServiceSaveResult save_if_generation_unlocked(
      uint32_t expected_generation, uint16_t document_version,
      const uint8_t *document, size_t document_size);
  CommitResult commit_document(uint16_t document_version,
                               const uint8_t *document,
                               size_t document_size);
  CommitResult commit_document_if_generation(uint32_t expected_generation,
                                             uint16_t document_version,
                                             const uint8_t *document,
                                             size_t document_size);
  bool supports_version(uint16_t document_version) const;
  bool document_is_valid(uint16_t document_version, const uint8_t *document,
                         size_t document_size) const;
  uint8_t *encoded_buffer(size_t required_size,
                          std::vector<uint8_t> *fallback) const;

  ConfigurationStore &store_;
  LegacyConfigurationAdapter &legacy_;
  const ConfigurationDocumentValidator *validator_{nullptr};
  uint8_t *scratch_buffer_{nullptr};
  size_t scratch_capacity_{0};
  ConfigurationRuntimeAdapter *runtime_{nullptr};
  LegacyConfigurationMode legacy_mode_{LegacyConfigurationMode::DUAL_WRITE};
  // The service scratch buffer is shared by load and commit paths. Keep the
  // complete operations mutually exclusive across ESPHome and HTTP tasks.
  std::mutex operation_mutex_{};
};

}  // namespace espcontrol::configuration
