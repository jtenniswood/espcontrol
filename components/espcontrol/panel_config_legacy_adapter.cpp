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

LegacyLoadResult
PanelConfigLegacyAdapter::merge(uint16_t version, const uint8_t *native,
                                size_t native_size, uint8_t *output,
                                size_t output_capacity, bool *changed) {
  if (changed)
    *changed = false;
  if (version != PANEL_CONFIG_DOCUMENT_VERSION || !bindings_.configured())
    return {LegacyStatus::READ_FAILED, version, 0};
  size_t size = 0;
  if (!bindings_.merge_document(native, native_size, output, output_capacity,
                                &size))
    return {LegacyStatus::READ_FAILED, version, size};
  // Compare records by identity rather than byte ordering. Browser saves may
  // use a different record order from the legacy projection.
  PanelConfigReader original(native, native_size), merged(output, size);
  size_t originals = 0, matches = 0, replacements = 0;
  PanelConfigRecord record, other;
  original.begin();
  while (original.next(&record) == PanelConfigStatus::OK) {
    ++originals;
    merged.begin();
    while (merged.next(&other) == PanelConfigStatus::OK) {
      if (record.type != other.type || record.slot != other.slot ||
          record.key_size != other.key_size ||
          (record.key_size &&
           std::memcmp(record.key, other.key, record.key_size) != 0))
        continue;
      if (record.value_size == other.value_size &&
          (!record.value_size ||
           std::memcmp(record.value, other.value, record.value_size) == 0))
        ++matches;
      break;
    }
  }
  merged.begin();
  while (merged.next(&other) == PanelConfigStatus::OK)
    ++replacements;
  if (changed)
    *changed = matches != originals || matches != replacements;
  return {LegacyStatus::OK, version, size};
}

bool PanelConfigLegacyAdapter::mirror(uint16_t document_version,
                                      const uint8_t *document,
                                      size_t document_size) {
  return document_version == PANEL_CONFIG_DOCUMENT_VERSION &&
         bindings_.configured() &&
         bindings_.persist_document(document, document_size);
}

}  // namespace espcontrol::configuration
