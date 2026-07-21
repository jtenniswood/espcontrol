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

class ServiceBuffer {
 public:
  ServiceBuffer(uint8_t *scratch, size_t scratch_capacity, size_t required)
      : data_(scratch), capacity_(scratch_capacity) {
    if (data_ == nullptr) {
      owned_.resize(required);
      data_ = owned_.empty() ? nullptr : owned_.data();
      capacity_ = owned_.size();
    }
  }

  bool available(size_t required) const {
    return required == 0 || (data_ != nullptr && capacity_ >= required);
  }
  uint8_t *data() { return data_; }
  size_t capacity() const { return capacity_; }

 private:
  std::vector<uint8_t> owned_;
  uint8_t *data_{nullptr};
  size_t capacity_{0};
};

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

  const size_t encoded_size =
      CONFIGURATION_DOCUMENT_HEADER_SIZE + document_size;
  ServiceBuffer encoded(scratch_, scratch_capacity_, encoded_size);
  if (!encoded.available(encoded_size)) {
    return {StoreStatus::BUFFER_TOO_SMALL, 0, document_size};
  }
  // Legacy import may use the fixed service scratch as both the source
  // document and the encoded destination. Move the document above its header
  // before writing metadata so no source bytes are overwritten.
  if (document_size > 0 && document == encoded.data()) {
    std::memmove(encoded.data() + CONFIGURATION_DOCUMENT_HEADER_SIZE,
                 encoded.data(), document_size);
  } else if (document_size > 0) {
    std::memcpy(encoded.data() + CONFIGURATION_DOCUMENT_HEADER_SIZE, document,
                document_size);
  }
  write_u32(encoded.data() + DOCUMENT_MAGIC_OFFSET, DOCUMENT_MAGIC);
  write_u16(encoded.data() + DOCUMENT_VERSION_OFFSET, document_version);
  write_u16(encoded.data() + DOCUMENT_HEADER_SIZE_OFFSET,
            CONFIGURATION_DOCUMENT_HEADER_SIZE);
  return store_.commit(encoded.data(), encoded_size);
}

ServiceLoadResult ConfigurationService::load(uint8_t *output,
                                             size_t output_capacity) {
  if (output == nullptr && output_capacity > 0) {
    return {ServiceStatus::INVALID_ARGUMENT, StoreStatus::INVALID_ARGUMENT};
  }
  ServiceBuffer encoded(scratch_, scratch_capacity_,
                        store_.maximum_payload_size());
  if (!encoded.available(store_.maximum_payload_size())) {
    const LoadResult inspected = store_.inspect();
    return {ServiceStatus::BUFFER_TOO_SMALL, inspected.status, 0,
            inspected.generation, inspected.payload_size};
  }
  const LoadResult stored = store_.load(
      encoded.data(), encoded.capacity());
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
      // output is allowed to be the caller-supplied service scratch. In that
      // case the document slides down over its envelope header, so the ranges
      // overlap and require memmove rather than memcpy.
      std::memmove(output,
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
  if (legacy.document_size > 0 && output == scratch_) {
    std::memmove(output,
                 output + CONFIGURATION_DOCUMENT_HEADER_SIZE,
                 legacy.document_size);
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
  if (scratch_ != nullptr) {
    // The output may alias the service scratch: load() first reconstructs the
    // encoded envelope there, then copies the document down over its header.
    return load(scratch_, maximum_document_size());
  }
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
