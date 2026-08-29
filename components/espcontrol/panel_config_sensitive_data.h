#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "panel_config_document.h"

namespace espcontrol::configuration {

inline bool panel_config_value_contains(const uint8_t *value, size_t value_size,
                                        const char *needle) {
  if (value == nullptr || needle == nullptr) return false;
  const size_t needle_size = std::strlen(needle);
  if (needle_size == 0 || needle_size > value_size) return false;
  for (size_t offset = 0; offset <= value_size - needle_size; ++offset) {
    if (std::memcmp(value + offset, needle, needle_size) == 0) return true;
  }
  return false;
}

// Wifi card fields are compact text inside button and subpage records. Require
// both the card type and password option so unrelated text containing either
// token does not make the whole panel document sensitive.
inline bool panel_config_contains_wifi_password(const uint8_t *document,
                                                size_t document_size) {
  PanelConfigReader reader(document, document_size);
  if (reader.begin() != PanelConfigStatus::OK) return false;
  bool sensitive = false;
  PanelConfigRecord record;
  PanelConfigStatus status = PanelConfigStatus::OK;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
    if ((record.type == PanelConfigRecordType::BUTTON ||
         record.type == PanelConfigRecordType::SUBPAGE) &&
        panel_config_value_contains(record.value, record.value_size, "wifi_qr") &&
        panel_config_value_contains(record.value, record.value_size, "pass64=")) {
      sensitive = true;
    }
  }
  return status == PanelConfigStatus::END && sensitive;
}

}  // namespace espcontrol::configuration
