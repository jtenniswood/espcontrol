#pragma once

#include "configuration_service.h"
#include "configuration_release_policy.h"
#include "panel_config_representation.h"

namespace espcontrol::configuration {

// Ship both readers while the durable writer stays v1. Firmware supplies the
// same PSRAM-only temporary allocator as the service; no index is retained.
class PanelConfigDocumentValidator final
    : public ConfigurationDocumentValidator {
public:
  explicit PanelConfigDocumentValidator(
      ConfigurationBuffer::Allocate allocate = ConfigurationBuffer::allocate_host,
      ConfigurationBuffer::Release release = ConfigurationBuffer::release_host)
      : allocate_(allocate), release_(release) {}
  bool supports_version(uint16_t document_version) const override {
    return std::find(PANEL_CONFIG_STORAGE_READ_VERSIONS.begin(), PANEL_CONFIG_STORAGE_READ_VERSIONS.end(),
                     document_version) != PANEL_CONFIG_STORAGE_READ_VERSIONS.end();
  }
  bool supports_write_version(uint16_t version) const override {
    return version == PANEL_CONFIG_DURABLE_WRITE_VERSION;
  }
  uint16_t write_version() const override { return PANEL_CONFIG_DURABLE_WRITE_VERSION; }

  bool checks_replacements(uint16_t incoming) const override { return incoming == 1; }
  bool replacement_needs_document(uint16_t current, uint16_t incoming) const override {
    return current == 2 && incoming == 1;
  }
  ServiceStatus replacement_status(uint16_t current, const uint8_t *document,
      size_t size, uint16_t incoming) const override {
    return replacement_needs_document(current, incoming)
      ? inspect_v2(document, size, true) : ServiceStatus::OK;
  }
  bool validate(uint16_t document_version, const uint8_t *document,
                size_t document_size) const override {
    return validation_status(document_version, document, document_size) == ServiceStatus::OK;
  }
  ServiceStatus validation_status(uint16_t version, const uint8_t *document,
      size_t size) const override {
    if (version == PANEL_CONFIG_DOCUMENT_VERSION)
      return PanelConfigReader(document, size).validate() == PanelConfigStatus::OK
          ? ServiceStatus::OK : ServiceStatus::INVALID_DOCUMENT;
    if (version != PANEL_CONFIG_DOCUMENT_VERSION_2 || size > PANEL_CONFIG_V2_HTTP_MAX_BYTES)
      return ServiceStatus::INVALID_DOCUMENT;
    return inspect_v2(document, size, false);
  }
  ServiceStatus write_validation_status(uint16_t version, const uint8_t *document,
      size_t size) const override {
    return version == PANEL_CONFIG_DOCUMENT_VERSION_2
        ? inspect_v2(document, size, PANEL_CONFIG_STORAGE_WRITE_NAVIGATION_DEPTH < 2)
        : validation_status(version, document, size);
  }
 private:
  ServiceStatus inspect_v2(const uint8_t *document, size_t size, bool protect_nested) const {
    if (size > PANEL_CONFIG_V2_HTTP_MAX_BYTES) return ServiceStatus::INVALID_DOCUMENT;
    PanelConfigV2Reader reader(document, size);
    size_t needed = 0;
    if (reader.workspace_bytes(&needed) != PanelConfigStatus::OK ||
        needed > PANEL_CONFIG_CONVERSION_MAX_WORKSPACE) return ServiceStatus::INVALID_DOCUMENT;
    ConfigurationBuffer scratch(allocate_, release_);
    auto *workspace = scratch.allocate(needed);
    if (!workspace) return ServiceStatus::ALLOCATION_FAILED;
    if (reader.validate(workspace, needed) != PanelConfigStatus::OK) return ServiceStatus::INVALID_DOCUMENT;
    if (protect_nested) {
      PanelConfigV2Record row;
      for (size_t i=0; i<reader.record_count(); ++i) {
        reader.record(i, &row);
        if (row.kind == PanelConfigV2Kind::PAGE && row.id && row.first)
          return ServiceStatus::INCOMPATIBLE_REPLACEMENT;
      }
    }
    return ServiceStatus::OK;
  }
  ConfigurationBuffer::Allocate allocate_;
  ConfigurationBuffer::Release release_;
};

} // namespace espcontrol::configuration
