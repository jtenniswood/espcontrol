#pragma once

#include "configuration_service.h"
#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

// Applies native documents to the running panel without participating in
// legacy import or downgrade mirroring.
class PanelConfigRuntimeAdapter final : public ConfigurationRuntimeAdapter {
 public:
  explicit PanelConfigRuntimeAdapter(PanelConfigTextBindings &bindings)
      : bindings_(bindings) {}

  bool apply(uint16_t document_version, const uint8_t *document,
             size_t document_size) override;

 private:
  PanelConfigTextBindings &bindings_;
};

}  // namespace espcontrol::configuration
