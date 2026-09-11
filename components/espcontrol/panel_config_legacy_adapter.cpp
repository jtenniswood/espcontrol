#include "panel_config_legacy_adapter.h"

namespace espcontrol::configuration {

LegacyLoadResult PanelConfigLegacyAdapter::load(uint8_t *output,
                                                size_t output_capacity) {
  if (!bindings_.configured())
    return {LegacyStatus::EMPTY, PANEL_CONFIG_DOCUMENT_VERSION, 0};
  size_t document_size = 0;
  if (!bindings_.write_document(output, output_capacity, &document_size)) {
    return {LegacyStatus::BUFFER_TOO_SMALL, PANEL_CONFIG_DOCUMENT_VERSION,
            document_size};
  }
  return {LegacyStatus::OK, PANEL_CONFIG_DOCUMENT_VERSION, document_size};
}

bool PanelConfigLegacyAdapter::mirror(uint16_t document_version,
                                      const uint8_t *document,
                                      size_t document_size) {
  return document_version == PANEL_CONFIG_DOCUMENT_VERSION &&
         bindings_.configured() &&
         bindings_.persist_document(document, document_size);
}

}  // namespace espcontrol::configuration
