#pragma once

#include "configuration_service.h"
#include "panel_config_text_bindings.h"

namespace espcontrol::configuration {

// Imports preference-backed fields once, publishes live compatibility text
// after activation, and mirrors preferences while the checked release policy
// still promises downgrade compatibility.
class PanelConfigLegacyAdapter final : public LegacyConfigurationAdapter {
 public:
  explicit PanelConfigLegacyAdapter(PanelConfigTextBindings &bindings,
      ConfigurationBuffer::Allocate allocate = ConfigurationBuffer::allocate_host,
      ConfigurationBuffer::Release release = ConfigurationBuffer::release_host)
      : bindings_(bindings), allocate_(allocate), release_(release) {}

  bool configured() const { return bindings_.configured(); }
  LegacyLoadResult load(uint8_t *output, size_t output_capacity) override;
  LegacyLoadResult merge(uint16_t version, const uint8_t *native,
                         size_t native_size, uint8_t *output,
                         size_t output_capacity, bool *changed) override;
  bool publish(uint16_t document_version, const uint8_t *document,
               size_t document_size) override;
  bool mirror(uint16_t document_version, const uint8_t *document,
              size_t document_size) override;

  CompatibilityStatus publish_status(uint16_t version, const uint8_t *document, size_t size) override;
  CompatibilityStatus mirror_status(uint16_t version, const uint8_t *document, size_t size) override;

 private:
  PanelConfigTextBindings &bindings_;
  ConfigurationBuffer::Allocate allocate_;
  ConfigurationBuffer::Release release_;
};

}  // namespace espcontrol::configuration
