#pragma once

#include "legacy_card_config.h"
#include "panel_config_document.h"

namespace espcontrol {

// A malformed/unreadable document must never authorize image deletion.
inline bool panel_config_references_asset(const uint8_t *document, size_t size,
                                         const std::string &id, bool &referenced) {
  using namespace configuration;
  PanelConfigReader reader(document, size);
  if (reader.begin() != PanelConfigStatus::OK) return false;
  referenced = false;
  PanelConfigRecord record;
  PanelConfigStatus status;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
    if (record.type != PanelConfigRecordType::BUTTON &&
        record.type != PanelConfigRecordType::SUBPAGE) continue;
    const std::string value(reinterpret_cast<const char *>(record.value), record.value_size);
    referenced |= record.type == PanelConfigRecordType::BUTTON
        ? card_config_references_asset(value, id)
        : subpage_config_references_asset(value, id);
  }
  return status == PanelConfigStatus::END;
}

// Removing an option only shrinks records. Copy each record before writing it
// so the reader can safely advance through the original document in place.
inline bool clear_panel_config_asset_references(uint8_t *document, size_t &size,
                                               const std::string &id) {
  using namespace configuration;
  PanelConfigReader reader(document, size);
  if (reader.begin() != PanelConfigStatus::OK) return false;
  PanelConfigWriter writer(document, size);
  if (writer.begin() != PanelConfigStatus::OK) return false;
  PanelConfigRecord record;
  PanelConfigStatus status;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
    std::string value(reinterpret_cast<const char *>(record.value), record.value_size);
    const auto *bytes = reinterpret_cast<const uint8_t *>(value.data());
    switch (record.type) {
      case PanelConfigRecordType::DEVICE_PROFILE:
        status = writer.append_device_profile(bytes, value.size());
        break;
      case PanelConfigRecordType::BUTTON:
        clear_card_background_reference(value, id);
        status = writer.append_button(record.slot, reinterpret_cast<const uint8_t *>(value.data()), value.size());
        break;
      case PanelConfigRecordType::SUBPAGE:
        clear_subpage_background_references(value, id);
        status = writer.append_subpage(record.slot, reinterpret_cast<const uint8_t *>(value.data()), value.size());
        break;
      case PanelConfigRecordType::SETTING: {
        const std::string key(reinterpret_cast<const char *>(record.key), record.key_size);
        status = writer.append_setting(reinterpret_cast<const uint8_t *>(key.data()), key.size(), bytes, value.size());
        break;
      }
    }
    if (status != PanelConfigStatus::OK) return false;
  }
  return status == PanelConfigStatus::END && writer.finish(&size) == PanelConfigStatus::OK;
}

}  // namespace espcontrol
