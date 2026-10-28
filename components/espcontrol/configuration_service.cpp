#include "configuration_service.h"

#include <cstring>

namespace espcontrol::configuration {
namespace {

// Little-endian bytes spell "ECDO" on storage.
constexpr uint32_t DOCUMENT_MAGIC = 0x4F444345;
constexpr size_t DOCUMENT_MAGIC_OFFSET = 0;
constexpr size_t DOCUMENT_VERSION_OFFSET = 4;
constexpr size_t DOCUMENT_HEADER_SIZE_OFFSET = 6;

uint16_t read_u16(const uint8_t *data) {
  return static_cast<uint16_t>(data[0]) |
         (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t read_u32(const uint8_t *data) {
  return static_cast<uint32_t>(data[0]) |
         (static_cast<uint32_t>(data[1]) << 8) |
         (static_cast<uint32_t>(data[2]) << 16) |
         (static_cast<uint32_t>(data[3]) << 24);
}

void write_u16(uint8_t *data, uint16_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void write_u32(uint8_t *data, uint32_t value) {
  data[0] = static_cast<uint8_t>(value & 0xFF);
  data[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  data[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  data[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

bool supported_version(uint16_t version) {
  return version == CURRENT_CONFIGURATION_DOCUMENT_VERSION;
}

ServiceStatus store_failure(StoreStatus status) {
  return status == StoreStatus::ALLOCATION_FAILED
             ? ServiceStatus::ALLOCATION_FAILED
             : ServiceStatus::STORE_FAILED;
}

// Every unsuccessful path, including prepare itself and a generation conflict,
// releases staged resources. Destruction after successful activation is safe:
// adapters transfer staged ownership during activate_prepared().
class RuntimePreparation {
 public:
  RuntimePreparation(ConfigurationRuntimeAdapter *runtime, uint16_t version,
                     const uint8_t *document, size_t size, bool write = false)
      : runtime_(runtime), version_(version), document_(document), size_(size),
        ready_(runtime == nullptr || (write ? runtime->prepare_write(version, document, size)
                                         : runtime->prepare(version, document, size))) {}
  ~RuntimePreparation() {
    if (runtime_ != nullptr) runtime_->discard_prepared();
  }
  bool ready() const { return ready_; }
  ServiceStatus failure_status() const {
    return runtime_ && runtime_->preparation_allocation_failed()
        ? ServiceStatus::ALLOCATION_FAILED
        : runtime_ && runtime_->preparation_capacity_exceeded()
            ? ServiceStatus::CAPACITY_EXCEEDED : ServiceStatus::RUNTIME_PREPARE_FAILED;
  }
  bool activate(uint32_t generation) {
    return ready_ && (runtime_ == nullptr ||
      runtime_->activate_prepared(generation, version_, document_, size_));
  }

 private:
  ConfigurationRuntimeAdapter *runtime_;
  uint16_t version_;
  const uint8_t *document_;
  size_t size_;
  bool ready_;
};

}  // namespace

bool ConfigurationService::supports_version(uint16_t document_version) const {
  return validator_ != nullptr ? validator_->supports_version(document_version)
                               : supported_version(document_version);
}

bool ConfigurationService::supports_write_version(uint16_t document_version) const {
  return validator_ ? validator_->supports_write_version(document_version)
                    : supported_version(document_version);
}

ServiceStatus ConfigurationService::document_validation_status(
    uint16_t document_version, const uint8_t *document, size_t document_size) const {
  return validator_ ? validator_->validation_status(document_version, document, document_size)
                    : ServiceStatus::OK;
}

uint8_t *
ConfigurationService::encoded_buffer(size_t required_size,
                                     ConfigurationBuffer *fallback) const {
  if (scratch_buffer_ != nullptr && scratch_capacity_ >= required_size)
    return scratch_buffer_;
  return fallback->allocate(required_size);
}

size_t ConfigurationService::maximum_document_size() const {
  const size_t maximum_payload = store_.maximum_payload_size();
  return maximum_payload > CONFIGURATION_DOCUMENT_HEADER_SIZE
             ? maximum_payload - CONFIGURATION_DOCUMENT_HEADER_SIZE
             : 0;
}

CommitResult ConfigurationService::commit_document(
    uint16_t document_version, const uint8_t *document,
    size_t document_size) {
  if (document_size > maximum_document_size()) {
    return {StoreStatus::PAYLOAD_TOO_LARGE, 0, document_size};
  }

  ConfigurationBuffer fallback(allocate_, release_);
  const size_t encoded_size = CONFIGURATION_DOCUMENT_HEADER_SIZE + document_size;
  uint8_t *encoded = encoded_buffer(encoded_size, &fallback);
  if (encoded == nullptr)
    return {StoreStatus::ALLOCATION_FAILED, 0, document_size};
  write_u32(encoded + DOCUMENT_MAGIC_OFFSET, DOCUMENT_MAGIC);
  write_u16(encoded + DOCUMENT_VERSION_OFFSET, document_version);
  write_u16(encoded + DOCUMENT_HEADER_SIZE_OFFSET,
            CONFIGURATION_DOCUMENT_HEADER_SIZE);
  if (document_size > 0) {
    std::memcpy(encoded + CONFIGURATION_DOCUMENT_HEADER_SIZE, document,
                document_size);
  }
  return store_.commit(encoded, encoded_size);
}

CommitResult ConfigurationService::commit_document_if_generation(
    uint32_t expected_generation, uint16_t document_version,
    const uint8_t *document, size_t document_size) {
  if (document_size > maximum_document_size()) {
    return {StoreStatus::PAYLOAD_TOO_LARGE, 0, document_size};
  }

  ConfigurationBuffer fallback(allocate_, release_);
  const size_t encoded_size = CONFIGURATION_DOCUMENT_HEADER_SIZE + document_size;
  uint8_t *encoded = encoded_buffer(encoded_size, &fallback);
  if (encoded == nullptr)
    return {StoreStatus::ALLOCATION_FAILED, 0, document_size};
  write_u32(encoded + DOCUMENT_MAGIC_OFFSET, DOCUMENT_MAGIC);
  write_u16(encoded + DOCUMENT_VERSION_OFFSET, document_version);
  write_u16(encoded + DOCUMENT_HEADER_SIZE_OFFSET,
            CONFIGURATION_DOCUMENT_HEADER_SIZE);
  if (document_size > 0) {
    std::memcpy(encoded + CONFIGURATION_DOCUMENT_HEADER_SIZE, document,
                document_size);
  }
  return store_.commit_if_generation(expected_generation, encoded, encoded_size);
}

ServiceLoadResult ConfigurationService::load(uint8_t *output,
                                             size_t output_capacity) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  return load_unlocked(output, output_capacity);
}

ServiceLoadResult ConfigurationService::load_and_apply_runtime(
    uint8_t *output, size_t output_capacity) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  const ServiceLoadResult loaded = load_unlocked(output, output_capacity);
  if (!loaded.ok() || runtime_ == nullptr) return loaded;
  RuntimePreparation prepared(runtime_, loaded.document_version, output,
                               loaded.document_size);
  if (!prepared.ready()) {
    ServiceLoadResult failed = loaded;
    failed.status = prepared.failure_status();
    return failed;
  }
  if (!prepared.activate(loaded.generation)) {
    ServiceLoadResult failed = loaded;
    failed.status = ServiceStatus::RUNTIME_APPLY_FAILED;
    return failed;
  }
  select_compatibility_generation(loaded.generation);
  compatibility_state_.publication =
      legacy_.publish_status(loaded.document_version, output, loaded.document_size);
  compatibility_publication_current_ =
      compatibility_state_.publication == CompatibilityStatus::OK;
  if (!compatibility_publication_current_) {
    ServiceLoadResult failed = loaded;
    failed.status = ServiceStatus::LEGACY_MIRROR_FAILED;
    return failed;
  }
  return loaded;
}

void ConfigurationService::select_compatibility_generation(uint32_t generation) {
  if (compatibility_state_.known && compatibility_state_.generation == generation) return;
  compatibility_state_ = {generation, true, CompatibilityStatus::RETRYABLE_FAILURE,
      legacy_writes_enabled() ? CompatibilityStatus::RETRYABLE_FAILURE : CompatibilityStatus::OK};
}

ConfigurationCompatibilityState ConfigurationService::compatibility_state() const {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  return compatibility_state_;
}

ServiceLoadResult ConfigurationService::retry_compatibility(uint8_t *output, size_t capacity) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  auto loaded = load_unlocked(output, capacity, false);
  if (!loaded.ok()) {
    if (loaded.status == ServiceStatus::EMPTY || loaded.status == ServiceStatus::INVALID_DOCUMENT ||
        loaded.status == ServiceStatus::UNSUPPORTED_VERSION) {
      compatibility_state_.publication = compatibility_state_.persistence = CompatibilityStatus::NOT_REPRESENTABLE;
      compatibility_publication_current_ = false;
    }
    return loaded;
  }
  select_compatibility_generation(loaded.generation);
  if (compatibility_state_.publication == CompatibilityStatus::RETRYABLE_FAILURE)
    compatibility_state_.publication = legacy_.publish_status(loaded.document_version, output, loaded.document_size);
  compatibility_publication_current_ = compatibility_state_.publication == CompatibilityStatus::OK;
  if (compatibility_state_.persistence == CompatibilityStatus::RETRYABLE_FAILURE)
    compatibility_state_.persistence = legacy_.mirror_status(loaded.document_version, output, loaded.document_size);
  if (!compatibility_publication_current_ || compatibility_state_.persistence != CompatibilityStatus::OK)
    loaded.status = ServiceStatus::LEGACY_MIRROR_FAILED;
  return loaded;
}

ServiceLoadResult ConfigurationService::load_unlocked(uint8_t *output,
                                                      size_t output_capacity,
                                                      bool import_legacy) {
  if (output == nullptr && output_capacity > 0) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  }
  ConfigurationBuffer fallback(allocate_, release_);
  const size_t encoded_capacity = store_.maximum_payload_size();
  uint8_t *encoded = encoded_buffer(encoded_capacity, &fallback);
  if (encoded == nullptr)
    return {ServiceStatus::ALLOCATION_FAILED, StoreStatus::ALLOCATION_FAILED};
  const LoadResult stored = store_.load(encoded, encoded_capacity);
  if (stored.ok()) {
    if (stored.payload_size < CONFIGURATION_DOCUMENT_HEADER_SIZE ||
        read_u32(encoded + DOCUMENT_MAGIC_OFFSET) != DOCUMENT_MAGIC ||
        read_u16(encoded + DOCUMENT_HEADER_SIZE_OFFSET) !=
            CONFIGURATION_DOCUMENT_HEADER_SIZE) {
      return {ServiceStatus::INVALID_DOCUMENT, stored.status, 0,
              stored.generation, stored.payload_size};
    }

    const uint16_t version =
        read_u16(encoded + DOCUMENT_VERSION_OFFSET);
    const size_t document_size =
        stored.payload_size - CONFIGURATION_DOCUMENT_HEADER_SIZE;
    if (!supports_version(version)) {
      return {ServiceStatus::UNSUPPORTED_VERSION, stored.status, version,
              stored.generation, document_size};
    }
    const uint8_t *document =
        encoded + CONFIGURATION_DOCUMENT_HEADER_SIZE;
    const auto validation = document_validation_status(version, document, document_size);
    if (validation != ServiceStatus::OK) {
      return {validation, stored.status, version,
              stored.generation, document_size};
    }
    if (document_size > output_capacity) {
      return {ServiceStatus::BUFFER_TOO_SMALL, stored.status, version,
              stored.generation, document_size};
    }
    if (document_size > 0 && output == nullptr) {
      return {ServiceStatus::INVALID_ARGUMENT, stored.status, version,
              stored.generation, document_size};
    }
    if (document_size > 0) {
      std::memmove(output, document, document_size);
    }
    return {ServiceStatus::OK, stored.status, version, stored.generation,
            document_size};
  }

  if (stored.status != StoreStatus::EMPTY) {
    return {store_failure(stored.status), stored.status};
  }

  if (!import_legacy) return {ServiceStatus::EMPTY, stored.status};
  fallback.reset();
  const LegacyLoadResult legacy = legacy_.load(output, output_capacity);
  if (legacy.status == LegacyStatus::EMPTY) {
    return {ServiceStatus::EMPTY, stored.status};
  }
  if (legacy.status == LegacyStatus::BUFFER_TOO_SMALL) {
    return {ServiceStatus::BUFFER_TOO_SMALL, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }
  if (legacy.status == LegacyStatus::ALLOCATION_FAILED)
    return {ServiceStatus::ALLOCATION_FAILED, stored.status,
            legacy.document_version, 0, legacy.document_size};
  if (legacy.status != LegacyStatus::OK) {
    return {ServiceStatus::LEGACY_READ_FAILED, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }
  if (!supports_write_version(legacy.document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }
  if (legacy.document_size > output_capacity ||
      (legacy.document_size > 0 && output == nullptr)) {
    return {legacy.document_size > output_capacity
                ? ServiceStatus::BUFFER_TOO_SMALL
                : ServiceStatus::INVALID_ARGUMENT,
            stored.status, legacy.document_version, 0,
            legacy.document_size};
  }
  const auto validation = document_validation_status(legacy.document_version, output, legacy.document_size);
  if (validation != ServiceStatus::OK) {
    return {validation, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }

  const CommitResult imported = commit_document(
      legacy.document_version, output, legacy.document_size);
  if (!imported.ok()) {
    return {store_failure(imported.status), imported.status,
            legacy.document_version, imported.generation, legacy.document_size};
  }
  return {ServiceStatus::IMPORTED_LEGACY, imported.status,
          legacy.document_version, imported.generation,
          legacy.document_size};
}

ServiceLoadResult ConfigurationService::refresh_legacy_shadow(
    uint8_t *output, size_t output_capacity) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  return refresh_legacy_shadow_unlocked(output, output_capacity);
}

ServiceLoadResult ConfigurationService::refresh_legacy_shadow_unlocked(
    uint8_t *output, size_t output_capacity) {
  if (output == nullptr && output_capacity > 0) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  }

  const LegacyLoadResult legacy = legacy_.load(output, output_capacity);
  if (legacy.status == LegacyStatus::EMPTY) {
    return {ServiceStatus::EMPTY, StoreStatus::EMPTY};
  }
  if (legacy.status == LegacyStatus::BUFFER_TOO_SMALL) {
    return {ServiceStatus::BUFFER_TOO_SMALL, StoreStatus::EMPTY,
            legacy.document_version, 0, legacy.document_size};
  }
  if (legacy.status == LegacyStatus::ALLOCATION_FAILED)
    return {ServiceStatus::ALLOCATION_FAILED, StoreStatus::EMPTY,
            legacy.document_version, 0, legacy.document_size};
  if (legacy.status != LegacyStatus::OK) {
    return {ServiceStatus::LEGACY_READ_FAILED, StoreStatus::EMPTY,
            legacy.document_version, 0, legacy.document_size};
  }
  const auto validation = supports_write_version(legacy.document_version)
      ? document_validation_status(legacy.document_version, output, legacy.document_size)
      : ServiceStatus::UNSUPPORTED_VERSION;
  if (validation != ServiceStatus::OK) {
    return {validation,
            StoreStatus::EMPTY, legacy.document_version, 0,
            legacy.document_size};
  }

  ConfigurationBuffer fallback(allocate_, release_);
  const size_t encoded_capacity = store_.maximum_payload_size();
  uint8_t *encoded = encoded_buffer(encoded_capacity, &fallback);
  if (encoded == nullptr) {
    return {ServiceStatus::ALLOCATION_FAILED, StoreStatus::ALLOCATION_FAILED,
            legacy.document_version, 0, legacy.document_size};
  }
  const LoadResult stored = store_.load(encoded, encoded_capacity);
  // This legacy-only utility must not roll a newer native document back to
  // stale preference fields. Live edits use the lossless merge path instead.
  if (stored.ok() && stored.payload_size >= CONFIGURATION_DOCUMENT_HEADER_SIZE &&
      read_u32(encoded + DOCUMENT_MAGIC_OFFSET) == DOCUMENT_MAGIC &&
      read_u16(encoded + DOCUMENT_HEADER_SIZE_OFFSET) == CONFIGURATION_DOCUMENT_HEADER_SIZE &&
      read_u16(encoded + DOCUMENT_VERSION_OFFSET) != legacy.document_version) {
    return {ServiceStatus::UNSUPPORTED_VERSION, stored.status,
            read_u16(encoded + DOCUMENT_VERSION_OFFSET), stored.generation,
            stored.payload_size - CONFIGURATION_DOCUMENT_HEADER_SIZE};
  }
  const bool matches_native =
      stored.ok() &&
      stored.payload_size ==
          CONFIGURATION_DOCUMENT_HEADER_SIZE + legacy.document_size &&
      read_u32(encoded + DOCUMENT_MAGIC_OFFSET) == DOCUMENT_MAGIC &&
      read_u16(encoded + DOCUMENT_VERSION_OFFSET) == legacy.document_version &&
      read_u16(encoded + DOCUMENT_HEADER_SIZE_OFFSET) ==
          CONFIGURATION_DOCUMENT_HEADER_SIZE &&
      std::memcmp(encoded + CONFIGURATION_DOCUMENT_HEADER_SIZE, output,
                  legacy.document_size) == 0;
  if (matches_native) {
    return {ServiceStatus::OK, stored.status, legacy.document_version,
            stored.generation, legacy.document_size};
  }
  if (stored.status != StoreStatus::OK && stored.status != StoreStatus::EMPTY) {
    return {store_failure(stored.status), stored.status,
            legacy.document_version, stored.generation, legacy.document_size};
  }

  fallback.reset();
  const CommitResult committed = commit_document(
      legacy.document_version, output, legacy.document_size);
  if (!committed.ok()) {
    return {store_failure(committed.status), committed.status,
            legacy.document_version, committed.generation,
            legacy.document_size};
  }
  return {ServiceStatus::SYNCED_LEGACY, committed.status,
          legacy.document_version, committed.generation,
          legacy.document_size};
}

ServiceLoadResult
ConfigurationService::synchronize_legacy_runtime(uint8_t *output,
                                                 size_t output_capacity) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  const auto loaded = load_unlocked(output, output_capacity);
  if (!loaded.ok())
    return loaded;
  if (!compatibility_publication_current_) {
    auto failed = loaded;
    failed.status = ServiceStatus::LEGACY_MIRROR_FAILED;
    return failed;
  }
  // Load scratch has been released. Borrow a separate merge buffer until the
  // output is ready, then release it before save prepares its encoded payload.
  ConfigurationBuffer fallback(allocate_, release_);
  const auto capacity = store_.maximum_payload_size();
  auto *merged = encoded_buffer(capacity, &fallback);
  if (!merged)
    return {ServiceStatus::ALLOCATION_FAILED, StoreStatus::ALLOCATION_FAILED};
  const auto output_begin = reinterpret_cast<uintptr_t>(output);
  const auto merged_begin = reinterpret_cast<uintptr_t>(merged);
  if (output_capacity &&
      ((output_begin <= merged_begin &&
        merged_begin - output_begin < output_capacity) ||
       (merged_begin < output_begin && output_begin - merged_begin < capacity)))
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  bool changed = false;
  const auto legacy =
      legacy_.merge(loaded.document_version, output, loaded.document_size,
                    merged, capacity, &changed);
  if (legacy.status == LegacyStatus::ALLOCATION_FAILED)
    return {ServiceStatus::ALLOCATION_FAILED, loaded.store_status,
            loaded.document_version, loaded.generation, loaded.document_size};
  if (legacy.status != LegacyStatus::OK || legacy.document_size > capacity)
    return {ServiceStatus::LEGACY_READ_FAILED, loaded.store_status,
            loaded.document_version, loaded.generation, loaded.document_size};
  if (!changed)
    return loaded;
  if (legacy.document_size > output_capacity)
    return {ServiceStatus::BUFFER_TOO_SMALL, loaded.store_status,
            legacy.document_version, loaded.generation, legacy.document_size};
  std::memcpy(output, merged, legacy.document_size);
  fallback.reset();
  const auto saved =
      save_unlocked(legacy.document_version, output, legacy.document_size);
  if (saved.status == ServiceStatus::CAPACITY_EXCEEDED) {
    // A legacy client may already have persisted individual text fields. A
    // rejected native edit must not leave those fields as a false downgrade
    // copy. Fence the partial view and repair it from the durable generation.
    select_compatibility_generation(loaded.generation);
    compatibility_state_.publication = CompatibilityStatus::RETRYABLE_FAILURE;
    compatibility_state_.persistence = legacy_writes_enabled()
        ? CompatibilityStatus::RETRYABLE_FAILURE : CompatibilityStatus::OK;
    compatibility_publication_current_ = false;
  }
  return {saved.status, saved.store_status, saved.document_version,
          saved.generation, saved.document_size};
}

ServiceSaveResult ConfigurationService::save(uint16_t document_version,
                                             const uint8_t *document,
                                             size_t document_size) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  return save_unlocked(document_version, document, document_size);
}

ServiceSaveResult ConfigurationService::save_unlocked(
    uint16_t document_version, const uint8_t *document, size_t document_size) {
  if (document_size > 0 && document == nullptr) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!supports_write_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  const auto validation = document_validation_status(document_version, document, document_size);
  if (validation != ServiceStatus::OK) {
    return {validation, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  if (document_size > maximum_document_size()) {
    return {ServiceStatus::STORE_FAILED, StoreStatus::PAYLOAD_TOO_LARGE,
            document_version, 0, document_size};
  }
  if (legacy_writes_enabled()) {
    const auto admission = legacy_.admit_write(document_version, document, document_size);
    if (admission != CompatibilityStatus::OK)
      return {admission == CompatibilityStatus::RETRYABLE_FAILURE ? ServiceStatus::ALLOCATION_FAILED
                                                                 : ServiceStatus::CAPACITY_EXCEEDED,
              StoreStatus::EMPTY, document_version, 0, document_size};
  }
  RuntimePreparation prepared(runtime_, document_version, document, document_size, true);
  if (!prepared.ready()) {
    return {prepared.failure_status(), StoreStatus::EMPTY,
            document_version, 0, document_size};
  }

  const CommitResult committed =
      commit_document(document_version, document, document_size);
  if (!committed.ok()) {
    return {store_failure(committed.status), committed.status, document_version,
            committed.generation, document_size};
  }
  const bool applied = prepared.activate(committed.generation);
  select_compatibility_generation(committed.generation);
  compatibility_state_.publication = legacy_.publish_status(document_version, document, document_size);
  compatibility_publication_current_ = compatibility_state_.publication == CompatibilityStatus::OK;
  compatibility_state_.persistence = legacy_writes_enabled()
      ? legacy_.mirror_status(document_version, document, document_size) : CompatibilityStatus::OK;
  const bool published = compatibility_publication_current_;
  const bool mirrored = compatibility_state_.persistence == CompatibilityStatus::OK;
  if (!applied) {
    return {ServiceStatus::RUNTIME_APPLY_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!published || !mirrored) {
    return {ServiceStatus::LEGACY_MIRROR_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  return {ServiceStatus::OK, committed.status, document_version,
          committed.generation, document_size};
}

ServiceSaveResult ConfigurationService::save_if_generation(
    uint32_t expected_generation, uint16_t document_version,
    const uint8_t *document, size_t document_size) {
  std::lock_guard<std::mutex> lock(operation_mutex_);
  return save_if_generation_unlocked(expected_generation, document_version,
                                     document, document_size);
}

ServiceSaveResult ConfigurationService::save_if_generation_unlocked(
    uint32_t expected_generation, uint16_t document_version,
    const uint8_t *document, size_t document_size) {
  if (document_size > 0 && document == nullptr) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!supports_write_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  const auto validation = document_validation_status(document_version, document, document_size);
  if (validation != ServiceStatus::OK) {
    return {validation, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  if (document_size > maximum_document_size()) {
    return {ServiceStatus::STORE_FAILED, StoreStatus::PAYLOAD_TOO_LARGE,
            document_version, 0, document_size};
  }
  if (legacy_writes_enabled()) {
    const auto admission = legacy_.admit_write(document_version, document, document_size);
    if (admission != CompatibilityStatus::OK)
      return {admission == CompatibilityStatus::RETRYABLE_FAILURE ? ServiceStatus::ALLOCATION_FAILED
                                                                 : ServiceStatus::CAPACITY_EXCEEDED,
              StoreStatus::EMPTY, document_version, 0, document_size};
  }
  RuntimePreparation prepared(runtime_, document_version, document, document_size, true);
  if (!prepared.ready()) {
    return {prepared.failure_status(), StoreStatus::EMPTY,
            document_version, 0, document_size};
  }

  const CommitResult committed = commit_document_if_generation(
      expected_generation, document_version, document, document_size);
  if (committed.status == StoreStatus::GENERATION_CONFLICT) {
    return {ServiceStatus::GENERATION_CONFLICT, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!committed.ok()) {
    return {store_failure(committed.status), committed.status, document_version,
            committed.generation, document_size};
  }
  const bool applied = prepared.activate(committed.generation);
  select_compatibility_generation(committed.generation);
  compatibility_state_.publication = legacy_.publish_status(document_version, document, document_size);
  compatibility_publication_current_ = compatibility_state_.publication == CompatibilityStatus::OK;
  compatibility_state_.persistence = legacy_writes_enabled()
      ? legacy_.mirror_status(document_version, document, document_size) : CompatibilityStatus::OK;
  const bool published = compatibility_publication_current_;
  const bool mirrored = compatibility_state_.persistence == CompatibilityStatus::OK;
  if (!applied) {
    return {ServiceStatus::RUNTIME_APPLY_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!published || !mirrored) {
    return {ServiceStatus::LEGACY_MIRROR_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  return {ServiceStatus::OK, committed.status, document_version,
          committed.generation, document_size};
}

}  // namespace espcontrol::configuration
