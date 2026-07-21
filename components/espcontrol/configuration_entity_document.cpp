#include "configuration_entity_document.h"

#include <cstring>
#include <limits>

namespace espcontrol::configuration {
namespace {

constexpr size_t MAGIC_OFFSET = 0;
constexpr size_t FORMAT_OFFSET = 4;
constexpr size_t RECORD_COUNT_OFFSET = 6;

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

bool valid_domain(ConfigurationEntityDomain domain) {
  switch (domain) {
    case ConfigurationEntityDomain::TEXT:
    case ConfigurationEntityDomain::SELECT:
    case ConfigurationEntityDomain::NUMBER:
    case ConfigurationEntityDomain::SWITCH:
      return true;
  }
  return false;
}

}  // namespace

ConfigurationEntityDocumentBuilder::ConfigurationEntityDocumentBuilder(
    uint8_t *output, size_t output_capacity)
    : output_(output), output_capacity_(output_capacity) {
  if (output == nullptr && output_capacity > 0) valid_ = false;
}

bool ConfigurationEntityDocumentBuilder::append(
    const ConfigurationEntityView &entity) {
  if (!valid_ || !valid_domain(entity.domain) || entity.object_id == nullptr ||
      entity.object_id_size == 0 ||
      entity.object_id_size > std::numeric_limits<uint8_t>::max() ||
      (entity.value == nullptr && entity.value_size > 0) ||
      entity.value_size > std::numeric_limits<uint16_t>::max() ||
      record_count_ == std::numeric_limits<uint16_t>::max()) {
    valid_ = false;
    return false;
  }

  const size_t record_size = CONFIGURATION_ENTITY_RECORD_HEADER_SIZE +
                             entity.object_id_size + entity.value_size;
  if (record_size > std::numeric_limits<size_t>::max() - required_size_) {
    valid_ = false;
    return false;
  }
  const size_t offset = required_size_;
  required_size_ += record_size;
  ++record_count_;

  if (output_ == nullptr || required_size_ > output_capacity_) return true;
  output_[offset] = static_cast<uint8_t>(entity.domain);
  output_[offset + 1] = static_cast<uint8_t>(entity.object_id_size);
  write_u16(output_ + offset + 2, static_cast<uint16_t>(entity.value_size));
  std::memcpy(output_ + offset + CONFIGURATION_ENTITY_RECORD_HEADER_SIZE,
              entity.object_id, entity.object_id_size);
  if (entity.value_size > 0) {
    std::memcpy(output_ + offset + CONFIGURATION_ENTITY_RECORD_HEADER_SIZE +
                    entity.object_id_size,
                entity.value, entity.value_size);
  }
  return true;
}

EntityDocumentResult ConfigurationEntityDocumentBuilder::finish() {
  if (!valid_) {
    return {EntityDocumentStatus::INVALID_ARGUMENT, record_count_,
            required_size_};
  }
  if (record_count_ == 0) {
    return {EntityDocumentStatus::EMPTY, 0, 0};
  }
  if (output_ == nullptr || required_size_ > output_capacity_) {
    return {EntityDocumentStatus::BUFFER_TOO_SMALL, record_count_,
            required_size_};
  }
  write_u32(output_ + MAGIC_OFFSET, CONFIGURATION_ENTITY_DOCUMENT_MAGIC);
  write_u16(output_ + FORMAT_OFFSET, CONFIGURATION_ENTITY_DOCUMENT_FORMAT);
  write_u16(output_ + RECORD_COUNT_OFFSET, record_count_);
  return {EntityDocumentStatus::OK, record_count_, required_size_};
}

ConfigurationEntityDocumentReader::ConfigurationEntityDocumentReader(
    const uint8_t *document, size_t document_size)
    : document_(document), document_size_(document_size) {
  if (document == nullptr ||
      document_size < CONFIGURATION_ENTITY_DOCUMENT_HEADER_SIZE) {
    status_ = document_size == 0 ? EntityDocumentStatus::EMPTY
                                : EntityDocumentStatus::INVALID_ARGUMENT;
    return;
  }
  if (read_u32(document + MAGIC_OFFSET) !=
      CONFIGURATION_ENTITY_DOCUMENT_MAGIC) {
    status_ = EntityDocumentStatus::INVALID_DOCUMENT;
    return;
  }
  if (read_u16(document + FORMAT_OFFSET) !=
      CONFIGURATION_ENTITY_DOCUMENT_FORMAT) {
    status_ = EntityDocumentStatus::UNSUPPORTED_FORMAT;
    return;
  }
  record_count_ = read_u16(document + RECORD_COUNT_OFFSET);
  status_ = record_count_ == 0 ? EntityDocumentStatus::EMPTY
                               : EntityDocumentStatus::OK;
}

EntityDocumentResult ConfigurationEntityDocumentReader::inspect() const {
  if (status_ != EntityDocumentStatus::OK) {
    return {status_, record_count_, document_size_};
  }
  ConfigurationEntityDocumentReader cursor(document_, document_size_);
  ConfigurationEntityView entity;
  while (cursor.next(&entity)) {
  }
  return {cursor.finished() ? EntityDocumentStatus::OK
                            : EntityDocumentStatus::INVALID_DOCUMENT,
          record_count_, document_size_};
}

bool ConfigurationEntityDocumentReader::next(ConfigurationEntityView *output) {
  if (status_ != EntityDocumentStatus::OK || output == nullptr ||
      record_index_ >= record_count_) {
    return false;
  }
  if (offset_ > document_size_ ||
      CONFIGURATION_ENTITY_RECORD_HEADER_SIZE > document_size_ - offset_) {
    status_ = EntityDocumentStatus::INVALID_DOCUMENT;
    return false;
  }
  const auto domain =
      static_cast<ConfigurationEntityDomain>(document_[offset_]);
  const size_t object_id_size = document_[offset_ + 1];
  const size_t value_size = read_u16(document_ + offset_ + 2);
  const size_t payload_size = object_id_size + value_size;
  if (!valid_domain(domain) || object_id_size == 0 ||
      payload_size > document_size_ - offset_ -
                         CONFIGURATION_ENTITY_RECORD_HEADER_SIZE) {
    status_ = EntityDocumentStatus::INVALID_DOCUMENT;
    return false;
  }
  const char *object_id = reinterpret_cast<const char *>(
      document_ + offset_ + CONFIGURATION_ENTITY_RECORD_HEADER_SIZE);
  *output = {domain, object_id, object_id_size, object_id + object_id_size,
             value_size};
  offset_ += CONFIGURATION_ENTITY_RECORD_HEADER_SIZE + payload_size;
  ++record_index_;
  if (record_index_ == record_count_ && offset_ != document_size_) {
    status_ = EntityDocumentStatus::INVALID_DOCUMENT;
    return false;
  }
  return true;
}

bool ConfigurationEntityDocumentReader::finished() const {
  return status_ == EntityDocumentStatus::OK &&
         record_index_ == record_count_ && offset_ == document_size_;
}

LegacyLoadResult EntityConfigurationAdapter::load(uint8_t *output,
                                                  size_t output_capacity) {
  ConfigurationEntityDocumentBuilder builder(output, output_capacity);
  const size_t count = registry_.size();
  for (size_t index = 0; index < count; ++index) {
    ConfigurationEntityView entity;
    if (!registry_.read(index, &entity) || !builder.append(entity)) {
      return {LegacyStatus::READ_FAILED,
              CURRENT_CONFIGURATION_DOCUMENT_VERSION, 0};
    }
  }
  const EntityDocumentResult result = builder.finish();
  switch (result.status) {
    case EntityDocumentStatus::OK:
      return {LegacyStatus::OK, CURRENT_CONFIGURATION_DOCUMENT_VERSION,
              result.document_size};
    case EntityDocumentStatus::EMPTY:
      return {LegacyStatus::EMPTY, CURRENT_CONFIGURATION_DOCUMENT_VERSION, 0};
    case EntityDocumentStatus::BUFFER_TOO_SMALL:
      return {LegacyStatus::BUFFER_TOO_SMALL,
              CURRENT_CONFIGURATION_DOCUMENT_VERSION, result.document_size};
    default:
      return {LegacyStatus::READ_FAILED,
              CURRENT_CONFIGURATION_DOCUMENT_VERSION, result.document_size};
  }
}

bool EntityConfigurationAdapter::mirror(uint16_t document_version,
                                        const uint8_t *document,
                                        size_t document_size) {
  if (document_version != CURRENT_CONFIGURATION_DOCUMENT_VERSION) return false;
  ConfigurationEntityDocumentReader validator(document, document_size);
  if (!validator.inspect().ok()) return false;
  ConfigurationEntityView entity;
  while (validator.next(&entity)) {
    if (!registry_.can_apply(entity)) return false;
  }
  if (!validator.finished()) return false;

  ConfigurationEntityDocumentReader applier(document, document_size);
  while (applier.next(&entity)) {
    if (!registry_.apply(entity)) return false;
  }
  return applier.finished();
}

}  // namespace espcontrol::configuration
