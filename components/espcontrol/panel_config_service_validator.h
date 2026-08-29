#pragma once

#include "configuration_service.h"
#include "panel_config_document.h"

namespace espcontrol::configuration {

// Bridges the generic atomic service to the native PanelConfig v1 codec. It
// can be supplied by the later live firmware integration without changing the
// store or legacy compatibility adapter.
class PanelConfigDocumentValidator final
    : public ConfigurationDocumentValidator {
public:
  bool supports_version(uint16_t document_version) const override {
    return document_version == PANEL_CONFIG_DOCUMENT_VERSION;
  }

  bool validate(uint16_t document_version, const uint8_t *document,
                size_t document_size) const override {
    if (!supports_version(document_version))
      return false;
    PanelConfigReader reader(document, document_size);
    return reader.validate() == PanelConfigStatus::OK;
  }
};

} // namespace espcontrol::configuration
