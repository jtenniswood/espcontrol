#pragma once

#include <cstddef>

#include "configuration_entity_document.h"

namespace espcontrol::configuration {

// ESPHome adapter for every entity marked entity_category: config. Values are
// addressed by the existing object IDs, so documents remain compatible with
// the legacy web/entity surface while saves become atomic revisions.
class EspHomeConfigurationRegistry final : public ConfigurationEntityRegistry {
 public:
  size_t size() const override;
  bool read(size_t index, ConfigurationEntityView *output) const override;
  bool can_apply(const ConfigurationEntityView &entity) const override;
  bool apply(const ConfigurationEntityView &entity) override;

 private:
  mutable char object_id_buffer_[128]{};
  mutable char value_buffer_[32]{};
};

}  // namespace espcontrol::configuration
