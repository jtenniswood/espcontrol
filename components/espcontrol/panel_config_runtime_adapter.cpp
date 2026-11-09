#include "panel_config_runtime_adapter.h"
#include "panel_config_compatibility_view.h"

namespace espcontrol::configuration {

bool PanelConfigRuntimeAdapter::apply(uint16_t document_version,
                                      const uint8_t *document,
                                      size_t document_size) {
  if (!bindings_.configured()) return false;
  PanelConfigCompatibilityView view(allocate_, release_);
  return view.load(document_version, document, document_size) == PanelConfigRepresentationStatus::OK &&
         bindings_.publish_document(view.data(), view.size());
}

}  // namespace espcontrol::configuration
