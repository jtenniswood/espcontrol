#pragma once

#include "panel_config_representation.h"

namespace espcontrol::configuration {

// One scoped compatibility view, borrowed for v1 and privately projected for
// v2. Never cache it across operations or keep a second decoded runtime model.
class PanelConfigCompatibilityView {
 public:
  PanelConfigCompatibilityView(ConfigurationBuffer::Allocate allocate,
                             ConfigurationBuffer::Release release)
      : converted_(allocate, release) {}
  PanelConfigRepresentationStatus load(uint16_t version, const uint8_t *source,
      size_t size, size_t ceiling = PANEL_CONFIG_V2_HTTP_MAX_BYTES) {
    if (version == PANEL_CONFIG_DOCUMENT_VERSION) {
      data_ = source; size_ = size;
      return PanelConfigRepresentationStatus::OK;
    }
    const auto result = converted_.convert(version, PANEL_CONFIG_DOCUMENT_VERSION, source, size, ceiling);
    if (result == PanelConfigRepresentationStatus::OK) {
      data_ = converted_.data(); size_ = converted_.size();
    }
    return result;
  }
  const uint8_t *data() const { return data_; }
  size_t size() const { return size_; }
 private:
  PanelConfigRepresentation converted_;
  const uint8_t *data_{nullptr};
  size_t size_{0};
};

} // namespace espcontrol::configuration
