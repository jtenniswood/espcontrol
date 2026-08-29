#include "configuration_service.h"

#include <cstring>
#include <vector>

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

}  // namespace

bool ConfigurationService::supports_version(uint16_t document_version) const {
  return validator_ != nullptr ? validator_->supports_version(document_version)
                               : supported_version(document_version);
}

bool ConfigurationService::document_is_valid(uint16_t document_version,
                                             const uint8_t *document,
                                             size_t document_size) const {
  return validator_ == nullptr ||
         validator_->validate(document_version, document, document_size);
}

uint8_t *ConfigurationService::encoded_buffer(
    size_t required_size, std::vector<uint8_t> *fallback) const {
  if (scratch_buffer_ != nullptr && scratch_capacity_ >= required_size)
    return scratch_buffer_;
  fallback->resize(required_size);
  return fallback->empty() ? nullptr : fallback->data();
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

  std::vector<uint8_t> fallback;
  const size_t encoded_size = CONFIGURATION_DOCUMENT_HEADER_SIZE + document_size;
  uint8_t *encoded = encoded_buffer(encoded_size, &fallback);
  if (encoded == nullptr) return {StoreStatus::WRITE_FAILED, 0, document_size};
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

  std::vector<uint8_t> fallback;
  const size_t encoded_size = CONFIGURATION_DOCUMENT_HEADER_SIZE + document_size;
  uint8_t *encoded = encoded_buffer(encoded_size, &fallback);
  if (encoded == nullptr) return {StoreStatus::WRITE_FAILED, 0, document_size};
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
  if (runtime_->apply(loaded.document_version, output, loaded.document_size)) {
    return loaded;
  }
  ServiceLoadResult failed = loaded;
  failed.status = ServiceStatus::RUNTIME_APPLY_FAILED;
  return failed;
}

ServiceLoadResult ConfigurationService::load_unlocked(uint8_t *output,
                                                      size_t output_capacity) {
  if (output == nullptr && output_capacity > 0) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  }
  std::vector<uint8_t> fallback;
  const size_t encoded_capacity = store_.maximum_payload_size();
  uint8_t *encoded = encoded_buffer(encoded_capacity, &fallback);
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
    if (!document_is_valid(version, document, document_size)) {
      return {ServiceStatus::INVALID_DOCUMENT, stored.status, version,
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
    return {ServiceStatus::STORE_FAILED, stored.status};
  }

  const LegacyLoadResult legacy = legacy_.load(output, output_capacity);
  if (legacy.status == LegacyStatus::EMPTY) {
    return {ServiceStatus::EMPTY, stored.status};
  }
  if (legacy.status == LegacyStatus::BUFFER_TOO_SMALL) {
    return {ServiceStatus::BUFFER_TOO_SMALL, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }
  if (legacy.status != LegacyStatus::OK) {
    return {ServiceStatus::LEGACY_READ_FAILED, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }
  if (!supports_version(legacy.document_version)) {
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
  if (!document_is_valid(legacy.document_version, output,
                         legacy.document_size)) {
    return {ServiceStatus::INVALID_DOCUMENT, stored.status,
            legacy.document_version, 0, legacy.document_size};
  }

  const CommitResult imported = commit_document(
      legacy.document_version, output, legacy.document_size);
  if (!imported.ok()) {
    return {ServiceStatus::STORE_FAILED, imported.status,
            legacy.document_version, imported.generation,
            legacy.document_size};
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
  if (legacy.status != LegacyStatus::OK) {
    return {ServiceStatus::LEGACY_READ_FAILED, StoreStatus::EMPTY,
            legacy.document_version, 0, legacy.document_size};
  }
  if (!supports_version(legacy.document_version) ||
      !document_is_valid(legacy.document_version, output,
                         legacy.document_size)) {
    return {supports_version(legacy.document_version)
                ? ServiceStatus::INVALID_DOCUMENT
                : ServiceStatus::UNSUPPORTED_VERSION,
            StoreStatus::EMPTY, legacy.document_version, 0,
            legacy.document_size};
  }

  std::vector<uint8_t> fallback;
  const size_t encoded_capacity = store_.maximum_payload_size();
  uint8_t *encoded = encoded_buffer(encoded_capacity, &fallback);
  if (encoded == nullptr) {
    return {ServiceStatus::STORE_FAILED, StoreStatus::WRITE_FAILED,
            legacy.document_version, 0, legacy.document_size};
  }
  const LoadResult stored = store_.load(encoded, encoded_capacity);
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
    return {ServiceStatus::STORE_FAILED, stored.status,
            legacy.document_version, stored.generation, legacy.document_size};
  }

  const CommitResult committed = commit_document(
      legacy.document_version, output, legacy.document_size);
  if (!committed.ok()) {
    return {ServiceStatus::STORE_FAILED, committed.status,
            legacy.document_version, committed.generation,
            legacy.document_size};
  }
  return {ServiceStatus::SYNCED_LEGACY, committed.status,
          legacy.document_version, committed.generation,
          legacy.document_size};
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
  if (!supports_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!document_is_valid(document_version, document, document_size)) {
    return {ServiceStatus::INVALID_DOCUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  const CommitResult committed =
      commit_document(document_version, document, document_size);
  if (!committed.ok()) {
    return {ServiceStatus::STORE_FAILED, committed.status, document_version,
            committed.generation, document_size};
  }
  const bool mirrored = !legacy_writes_enabled() ||
                        legacy_.mirror(document_version, document, document_size);
  const bool applied = runtime_ == nullptr ||
                       runtime_->apply(document_version, document, document_size);
  if (!applied) {
    return {ServiceStatus::RUNTIME_APPLY_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!mirrored) {
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
  if (!supports_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!document_is_valid(document_version, document, document_size)) {
    return {ServiceStatus::INVALID_DOCUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  const CommitResult committed = commit_document_if_generation(
      expected_generation, document_version, document, document_size);
  if (committed.status == StoreStatus::GENERATION_CONFLICT) {
    return {ServiceStatus::GENERATION_CONFLICT, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!committed.ok()) {
    return {ServiceStatus::STORE_FAILED, committed.status, document_version,
            committed.generation, document_size};
  }
  const bool mirrored = !legacy_writes_enabled() ||
                        legacy_.mirror(document_version, document, document_size);
  const bool applied = runtime_ == nullptr ||
                       runtime_->apply(document_version, document, document_size);
  if (!applied) {
    return {ServiceStatus::RUNTIME_APPLY_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  if (!mirrored) {
    return {ServiceStatus::LEGACY_MIRROR_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  return {ServiceStatus::OK, committed.status, document_version,
          committed.generation, document_size};
}

}  // namespace espcontrol::configuration
