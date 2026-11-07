#pragma once

#include <array>
#include <string>

#include "config_field_decode.h"
#include "panel_config_document.h"
#include "panel_backup_stream.h"

namespace espcontrol::backup {

inline std::string json_string(const std::string &value) {
  std::string out = "\"";
  for (unsigned char ch : value) {
    if (ch == '"' || ch == '\\') {
      out += '\\';
      out += static_cast<char>(ch);
    } else if (ch < 0x20) {
      constexpr char HEX[] = "0123456789abcdef";
      out += "\\u00";
      out += HEX[ch >> 4];
      out += HEX[ch & 15];
    } else {
      out += static_cast<char>(ch);
    }
  }
  return out + '"';
}

// Use raw saved fields: display-time normalization may translate labels or
// replace defaults. Import applies the browser's normal compatibility rules.
template<typename Sink>
bool write_card(std::string_view value, Sink &sink) {
  const bool compact = !value.empty() && value[0] == '~';
  size_t start = compact ? 1 : 0;
  if (!sink("{")) return false;
  for (size_t index = 0; index < 9; ++index) {
    size_t end = value.find(compact ? ',' : ';', start);
    if (end == std::string::npos) end = value.size();
    std::string field(value.substr(start, end - start));
    if (compact) field = decode_compact_field(field);
    if (field.empty() && (index == 2 || index == 3)) field = "Auto";
    if (!sink(std::string(index ? ",\"" : "\"") + BACKUP_CARD_FIELDS[index] + "\":") ||
        !write_backup_json_string(field, sink))
      return false;
    start = end < value.size() ? end + 1 : value.size();
  }
  return sink("}");
}

inline bool valid_backup_document(const uint8_t *document, size_t size, size_t slots) {
  using namespace configuration;
  if (slots == 0 || slots > PANEL_CONFIG_MAX_SLOT_COUNT) return false;
  // Inactive slots belong to the saved document even when the visible profile
  // is smaller. The readable projection uses slots; native bytes retain all.
  return PanelConfigReader(document, size).validate() == PanelConfigStatus::OK;
}

// The caller validates the document before sending HTTP headers. Keep references
// into that immutable document and serialize one card at a time, without a
// second full JSON document or a second full base64 payload in RAM.
template<typename Sink>
bool write_backup(const uint8_t *document, size_t size, size_t slots,
                  const std::string &settings, const std::string &screen,
                  const std::string &identity, const std::string &exported_at,
                  Sink &sink) {
  using namespace configuration;
  PanelConfigReader reader(document, size);
  if (!valid_backup_document(document, size, slots)) return false;
  std::array<PanelConfigRecord, PANEL_CONFIG_MAX_SLOT_COUNT> buttons{}, subpages{};
  std::string_view profile, order, color = "0073FF";
  reader.begin();
  PanelConfigRecord record;
  while (reader.next(&record) == PanelConfigStatus::OK) {
    const std::string_view value(reinterpret_cast<const char *>(record.value), record.value_size);
    switch (record.type) {
      case PanelConfigRecordType::DEVICE_PROFILE: profile = value; break;
      case PanelConfigRecordType::BUTTON:
        buttons[record.slot - 1] = record;
        break;
      case PanelConfigRecordType::SUBPAGE:
        subpages[record.slot - 1] = record;
        break;
      case PanelConfigRecordType::SETTING: {
        const std::string_view key(reinterpret_cast<const char *>(record.key), record.key_size);
        if (key == "button_order") order = value;
        if (key == "button_on_color" && !value.empty()) color = value;
        break;
      }
    }
  }
  if (!sink("{\"version\":2,\"format\":\"espcontrol.backup\",\"device\":" + json_string(std::string(profile)) +
            ",\"source\":{\"device\":" + json_string(std::string(profile)) + ",\"slots\":" + std::to_string(slots) + "}")) return false;
  if (!exported_at.empty() && !sink(",\"exported_at\":" + json_string(exported_at))) return false;
  if (!identity.empty() && !sink(",\"identity\":" + identity)) return false;
  if (!sink(",\"button_order\":") || !write_backup_json_string(order, sink) ||
      !sink(",\"button_on_color\":") || !write_backup_json_string(color, sink) || !sink(",\"buttons\":[")) return false;
  for (size_t i = 0; i < slots; ++i) {
    if (i && !sink(",")) return false;
    const auto &button = buttons[i];
    const std::string_view value = button.value == nullptr ? "" :
        std::string_view(reinterpret_cast<const char *>(button.value), button.value_size);
    if (!write_card(value, sink)) return false;
  }
  if (!sink("],\"subpages\":{")) return false;
  bool first = true;
  for (size_t i = 0; i < slots; ++i) {
    const auto &subpage = subpages[i];
    if (subpage.value == nullptr) continue;
    const std::string_view value(reinterpret_cast<const char *>(subpage.value), subpage.value_size);
    if (!sink(std::string(first ? "" : ",") + json_string(std::to_string(i + 1)) + ":") ||
        !write_backup_json_string(value, sink)) return false;
    first = false;
  }
  if (!sink("},\"settings\":" + settings + ",\"screen\":" + screen +
            ",\"native_config\":{\"document_version\":" + std::to_string(PANEL_CONFIG_DOCUMENT_VERSION) + ",\"device_profile\":" + json_string(std::string(profile)) + ",\"payload\":\"")) return false;
  return write_backup_base64(document, size, sink) && sink("\"}}");
}
}  // namespace espcontrol::backup
