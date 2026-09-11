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

}  // namespace espcontrol
