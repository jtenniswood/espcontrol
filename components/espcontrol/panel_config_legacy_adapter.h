#pragma once

#include "configuration_service.h"
#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

// Imports preference-backed text fields once and mirrors native saves while
// the checked release policy still promises downgrade compatibility.
class PanelConfigLegacyAdapter final : public LegacyConfigurationAdapter {
 public:
  explicit PanelConfigLegacyAdapter(PanelConfigTextBindings &bindings)
      : bindings_(bindings) {}

  bool configured() const { return bindings_.configured(); }
  LegacyLoadResult load(uint8_t *output, size_t output_capacity) override;
  bool mirror(uint16_t document_version, const uint8_t *document,
              size_t document_size) override;

 private:
  PanelConfigTextBindings &bindings_;
};

}  // namespace espcontrol::configuration
