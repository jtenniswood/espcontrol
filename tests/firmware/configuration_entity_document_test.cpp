#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>

#include "configuration_entity_document.h"

namespace {

using namespace espcontrol::configuration;

struct OwnedEntity {
  ConfigurationEntityDomain domain;
  std::string object_id;
  std::string value;
};

class FakeRegistry final : public ConfigurationEntityRegistry {
 public:
  size_t size() const override { return entities.size(); }

  bool read(size_t index, ConfigurationEntityView *output) const override {
    if (fail_read || output == nullptr || index >= entities.size()) return false;
    const OwnedEntity &entity = entities[index];
    *output = {entity.domain, entity.object_id.data(), entity.object_id.size(),
               entity.value.data(), entity.value.size()};
    return true;
  }

  bool can_apply(const ConfigurationEntityView &entity) const override {
    if (reject_object_id.empty()) return true;
    return std::string(entity.object_id, entity.object_id_size) !=
           reject_object_id;
  }

  bool apply(const ConfigurationEntityView &entity) override {
    applied.push_back(
        {entity.domain,
         std::string(entity.object_id, entity.object_id_size),
         std::string(entity.value, entity.value_size)});
    return true;
  }

  std::vector<OwnedEntity> entities;
  std::vector<OwnedEntity> applied;
  std::string reject_object_id;
  bool fail_read{false};
};

bool legacy_round_trip_keeps_entity_identity_and_values() {
  FakeRegistry registry;
  registry.entities = {
      {ConfigurationEntityDomain::TEXT, "button_1_config",
       "light.kitchen;Kitchen;Auto;Auto;;light;;"},
      {ConfigurationEntityDomain::SWITCH, "screen__clock_bar", "1"},
      {ConfigurationEntityDomain::NUMBER, "screensaver_timeout", "300"},
      {ConfigurationEntityDomain::SELECT, "screen__language", "en"},
  };
  EntityConfigurationAdapter adapter(registry);
  std::array<uint8_t, 512> document{};
  const LegacyLoadResult loaded = adapter.load(document.data(), document.size());
  if (loaded.status != LegacyStatus::OK || loaded.document_size == 0) {
    return false;
  }
  if (!adapter.mirror(loaded.document_version, document.data(),
                      loaded.document_size)) {
    return false;
  }
  return registry.applied.size() == registry.entities.size() &&
         registry.applied[0].object_id == "button_1_config" &&
         registry.applied[0].value == registry.entities[0].value &&
         registry.applied[1].domain == ConfigurationEntityDomain::SWITCH;
}

bool required_capacity_is_reported_without_partial_output() {
  FakeRegistry registry;
  registry.entities = {
      {ConfigurationEntityDomain::TEXT, "button_order", "1,2,3"},
      {ConfigurationEntityDomain::TEXT, "button_1_config", "large-value"},
  };
  EntityConfigurationAdapter adapter(registry);
  std::array<uint8_t, 10> small{};
  const LegacyLoadResult result = adapter.load(small.data(), small.size());
  return result.status == LegacyStatus::BUFFER_TOO_SMALL &&
         result.document_size > small.size();
}

bool invalid_or_unknown_documents_never_apply_partially() {
  FakeRegistry registry;
  registry.entities = {
      {ConfigurationEntityDomain::TEXT, "button_order", "1"},
      {ConfigurationEntityDomain::TEXT, "button_1_config", "config"},
  };
  EntityConfigurationAdapter adapter(registry);
  std::array<uint8_t, 256> document{};
  const LegacyLoadResult loaded = adapter.load(document.data(), document.size());
  if (loaded.status != LegacyStatus::OK) return false;

  registry.reject_object_id = "button_1_config";
  if (adapter.mirror(loaded.document_version, document.data(),
                     loaded.document_size) ||
      !registry.applied.empty()) {
    return false;
  }

  registry.reject_object_id.clear();
  return !adapter.mirror(loaded.document_version, document.data(),
                         loaded.document_size - 1) &&
         registry.applied.empty();
}

bool malformed_records_are_rejected() {
  std::array<uint8_t, 16> document{};
  document[0] = 'E';
  document[1] = 'C';
  document[2] = 'F';
  document[3] = '1';
  document[4] = 1;
  document[6] = 1;
  document[8] = static_cast<uint8_t>(ConfigurationEntityDomain::TEXT);
  document[9] = 8;
  document[10] = 20;
  ConfigurationEntityDocumentReader reader(document.data(), document.size());
  return reader.inspect().status == EntityDocumentStatus::INVALID_DOCUMENT;
}

}  // namespace

int main() {
  return legacy_round_trip_keeps_entity_identity_and_values() &&
                 required_capacity_is_reported_without_partial_output() &&
                 invalid_or_unknown_documents_never_apply_partially() &&
                 malformed_records_are_rejected()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
