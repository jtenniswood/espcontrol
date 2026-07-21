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

  std::vector<uint8_t> encoded(CONFIGURATION_DOCUMENT_HEADER_SIZE +
                               document_size);
  write_u32(encoded.data() + DOCUMENT_MAGIC_OFFSET, DOCUMENT_MAGIC);
  write_u16(encoded.data() + DOCUMENT_VERSION_OFFSET, document_version);
  write_u16(encoded.data() + DOCUMENT_HEADER_SIZE_OFFSET,
            CONFIGURATION_DOCUMENT_HEADER_SIZE);
  if (document_size > 0) {
    std::memcpy(encoded.data() + CONFIGURATION_DOCUMENT_HEADER_SIZE, document,
                document_size);
  }
  return store_.commit(encoded.data(), encoded.size());
}

ServiceLoadResult ConfigurationService::load(uint8_t *output,
                                             size_t output_capacity) {
  if (output == nullptr && output_capacity > 0) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  }
  std::vector<uint8_t> encoded(store_.maximum_payload_size());
  const LoadResult stored = store_.load(
      encoded.empty() ? nullptr : encoded.data(), encoded.size());
  if (stored.ok()) {
    if (stored.payload_size < CONFIGURATION_DOCUMENT_HEADER_SIZE ||
        read_u32(encoded.data() + DOCUMENT_MAGIC_OFFSET) != DOCUMENT_MAGIC ||
        read_u16(encoded.data() + DOCUMENT_HEADER_SIZE_OFFSET) !=
            CONFIGURATION_DOCUMENT_HEADER_SIZE) {
      return {ServiceStatus::INVALID_DOCUMENT, stored.status, 0,
              stored.generation, stored.payload_size};
    }

    const uint16_t version =
        read_u16(encoded.data() + DOCUMENT_VERSION_OFFSET);
    const size_t document_size =
        stored.payload_size - CONFIGURATION_DOCUMENT_HEADER_SIZE;
    if (!supported_version(version)) {
      return {ServiceStatus::UNSUPPORTED_VERSION, stored.status, version,
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
      std::memcpy(output,
                  encoded.data() + CONFIGURATION_DOCUMENT_HEADER_SIZE,
                  document_size);
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
  if (!supported_version(legacy.document_version)) {
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

ServiceSaveResult ConfigurationService::save(uint16_t document_version,
                                             const uint8_t *document,
                                             size_t document_size) {
  if (document_size > 0 && document == nullptr) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!supported_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  const CommitResult committed =
      commit_document(document_version, document, document_size);
  if (!committed.ok()) {
    return {ServiceStatus::STORE_FAILED, committed.status, document_version,
            committed.generation, document_size};
  }
  if (!legacy_.mirror(document_version, document, document_size)) {
    return {ServiceStatus::LEGACY_MIRROR_FAILED, committed.status,
            document_version, committed.generation, document_size};
  }
  return {ServiceStatus::OK, committed.status, document_version,
          committed.generation, document_size};
}

ServiceLoadResult ConfigurationService::ensure_snapshot_exists() {
  const LoadResult stored = store_.inspect();
  if (stored.ok()) {
    return {ServiceStatus::OK, stored.status, 0, stored.generation,
            stored.payload_size};
  }
  if (stored.status != StoreStatus::EMPTY) {
    return {ServiceStatus::STORE_FAILED, stored.status};
  }

  // An empty document store can still have legacy preference fields. Loading
  // once imports those fields before the first revision-aware write is allowed.
  std::vector<uint8_t> scratch(maximum_document_size());
  return load(scratch.empty() ? nullptr : scratch.data(), scratch.size());
}

ServiceSaveResult ConfigurationService::save_if_revision(
    uint32_t expected_revision, uint16_t document_version,
    const uint8_t *document, size_t document_size) {
  if (document_size > 0 && document == nullptr) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }
  if (!supported_version(document_version)) {
    return {ServiceStatus::UNSUPPORTED_VERSION, StoreStatus::INVALID_ARGUMENT,
            document_version, 0, document_size};
  }

  const ServiceLoadResult current = ensure_snapshot_exists();
  const uint32_t current_revision =
      current.status == ServiceStatus::EMPTY ? 0 : current.generation;
  if (current.status != ServiceStatus::EMPTY && !current.ok()) {
    return {current.status, current.store_status, document_version,
            current_revision, document_size};
  }
  if (current_revision != expected_revision) {
    return {ServiceStatus::REVISION_CONFLICT, current.store_status,
            document_version, current_revision, document_size};
  }
  return save(document_version, document, document_size);
}

}  // namespace espcontrol::configuration
