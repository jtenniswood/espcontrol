#pragma once

#include <cstddef>
#include <cstdint>

#include "configuration_service.h"

namespace espcontrol::configuration {

constexpr uint32_t CONFIGURATION_ENTITY_DOCUMENT_MAGIC = 0x31464345;
constexpr uint16_t CONFIGURATION_ENTITY_DOCUMENT_FORMAT = 1;
constexpr size_t CONFIGURATION_ENTITY_DOCUMENT_HEADER_SIZE = 8;
constexpr size_t CONFIGURATION_ENTITY_RECORD_HEADER_SIZE = 4;

enum class ConfigurationEntityDomain : uint8_t {
  TEXT = 1,
  SELECT = 2,
  NUMBER = 3,
  SWITCH = 4,
};

enum class EntityDocumentStatus : uint8_t {
  OK,
  EMPTY,
  INVALID_ARGUMENT,
  BUFFER_TOO_SMALL,
  INVALID_DOCUMENT,
  UNSUPPORTED_FORMAT,
  REGISTRY_FAILED,
};

struct ConfigurationEntityView {
  ConfigurationEntityDomain domain{ConfigurationEntityDomain::TEXT};
  const char *object_id{nullptr};
  size_t object_id_size{0};
  const char *value{nullptr};
  size_t value_size{0};
};

struct EntityDocumentResult {
  EntityDocumentStatus status{EntityDocumentStatus::INVALID_DOCUMENT};
  uint16_t record_count{0};
  size_t document_size{0};

  bool ok() const { return status == EntityDocumentStatus::OK; }
};

// Platform adapters expose config-category entities through this narrow
// interface. The binary document keeps the existing entity object IDs and
// values intact; it only changes the boundary from many writes to one checked
// revision.
class ConfigurationEntityRegistry {
 public:
  virtual ~ConfigurationEntityRegistry() = default;

  virtual size_t size() const = 0;
  virtual bool read(size_t index, ConfigurationEntityView *output) const = 0;
  virtual bool can_apply(const ConfigurationEntityView &entity) const = 0;
  virtual bool apply(const ConfigurationEntityView &entity) = 0;
};

class ConfigurationEntityDocumentBuilder {
 public:
  ConfigurationEntityDocumentBuilder(uint8_t *output, size_t output_capacity);

  bool append(const ConfigurationEntityView &entity);
  EntityDocumentResult finish();

 private:
  uint8_t *output_{nullptr};
  size_t output_capacity_{0};
  size_t required_size_{CONFIGURATION_ENTITY_DOCUMENT_HEADER_SIZE};
  uint16_t record_count_{0};
  bool valid_{true};
};

class ConfigurationEntityDocumentReader {
 public:
  ConfigurationEntityDocumentReader(const uint8_t *document,
                                    size_t document_size);

  EntityDocumentResult inspect() const;
  bool next(ConfigurationEntityView *output);
  bool finished() const;

 private:
  const uint8_t *document_{nullptr};
  size_t document_size_{0};
  size_t offset_{CONFIGURATION_ENTITY_DOCUMENT_HEADER_SIZE};
  uint16_t record_count_{0};
  uint16_t record_index_{0};
  EntityDocumentStatus status_{EntityDocumentStatus::INVALID_DOCUMENT};
};

// Compatibility bridge used while current firmware and hosted web bundles
// still read and write the individual ESPHome config entities.
class EntityConfigurationAdapter final : public LegacyConfigurationAdapter {
 public:
  explicit EntityConfigurationAdapter(ConfigurationEntityRegistry &registry)
      : registry_(registry) {}

  LegacyLoadResult load(uint8_t *output, size_t output_capacity) override;
  bool validate(uint16_t document_version, const uint8_t *document,
                size_t document_size) const;
  bool mirror(uint16_t document_version, const uint8_t *document,
              size_t document_size) override;

 private:
  ConfigurationEntityRegistry &registry_;
};

}  // namespace espcontrol::configuration
