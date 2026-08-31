#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "panel_config_document.h"

namespace espcontrol::configuration {

inline std::string_view panel_config_serialized_field(std::string_view value,
                                                      char delimiter,
                                                      size_t field_index) {
  size_t start = 0;
  for (size_t index = 0; index < field_index; ++index) {
    const size_t end = value.find(delimiter, start);
    if (end == std::string_view::npos) return {};
    start = end + 1;
  }
  const size_t end = value.find(delimiter, start);
  return value.substr(start, end == std::string_view::npos
                                 ? std::string_view::npos
                                 : end - start);
}

inline bool panel_config_wifi_type(std::string_view type) {
  return type == "wifi_qr" || type == "wifi_qr_card";
}

inline bool panel_config_serialized_button_contains_wifi_password(
    std::string_view value) {
  const bool compact = !value.empty() && value.front() == '~';
  if (compact) value.remove_prefix(1);
  const char delimiter = compact ? ',' : ';';
  return panel_config_wifi_type(
             panel_config_serialized_field(value, delimiter, 6)) &&
         panel_config_serialized_field(value, delimiter, 8)
                 .find("pass64=") != std::string_view::npos;
}

inline bool panel_config_serialized_subpage_contains_wifi_password(
    std::string_view value) {
  const bool compact = !value.empty() && value.front() == '~';
  if (compact) value.remove_prefix(1);
  size_t start = value.find('|');  // Skip the subpage order field.
  while (start != std::string_view::npos) {
    ++start;
    const size_t end = value.find('|', start);
    const std::string_view button = value.substr(
        start, end == std::string_view::npos ? std::string_view::npos
                                             : end - start);
    const char delimiter = compact ? ',' : ':';
    const size_t type_index = compact ? 0 : 6;
    if (panel_config_wifi_type(
            panel_config_serialized_field(button, delimiter, type_index)) &&
        panel_config_serialized_field(button, delimiter, 8)
                .find("pass64=") != std::string_view::npos) {
      return true;
    }
    start = end;
  }
  return false;
}

// Wifi card fields are compact text inside button and subpage records. Parse
// the actual type and options fields so labels and unrelated values containing
// the same tokens do not make the whole panel document sensitive.
inline bool panel_config_contains_wifi_password(const uint8_t *document,
                                                size_t document_size) {
  PanelConfigReader reader(document, document_size);
  if (reader.begin() != PanelConfigStatus::OK) return false;
  bool sensitive = false;
  PanelConfigRecord record;
  PanelConfigStatus status = PanelConfigStatus::OK;
  while ((status = reader.next(&record)) == PanelConfigStatus::OK) {
    const std::string_view value(
        reinterpret_cast<const char *>(record.value), record.value_size);
    if (record.type == PanelConfigRecordType::BUTTON)
      sensitive |= panel_config_serialized_button_contains_wifi_password(value);
    else if (record.type == PanelConfigRecordType::SUBPAGE)
      sensitive |= panel_config_serialized_subpage_contains_wifi_password(value);
  }
  return status == PanelConfigStatus::END && sensitive;
}

}  // namespace espcontrol::configuration
