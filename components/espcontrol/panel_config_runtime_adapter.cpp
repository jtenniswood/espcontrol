#include "panel_config_runtime_adapter.h"

namespace espcontrol::configuration {

bool PanelConfigRuntimeAdapter::apply(uint16_t document_version,
                                      const uint8_t *document,
                                      size_t document_size) {
  return document_version == PANEL_CONFIG_DOCUMENT_VERSION &&
         bindings_.configured() &&
         bindings_.publish_document(document, document_size);
}

}  // namespace espcontrol::configuration
