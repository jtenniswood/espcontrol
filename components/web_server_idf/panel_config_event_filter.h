#pragma once

#include <cstddef>
#include <cstring>

namespace esphome::web_server_idf {

inline bool event_payload_contains(const char *payload, size_t payload_size,
                                   const char *needle) {
  if (payload == nullptr || needle == nullptr) return false;
  const size_t needle_size = std::strlen(needle);
  if (needle_size == 0 || needle_size > payload_size) return false;
  for (size_t offset = 0; offset <= payload_size - needle_size; ++offset) {
    if (std::memcmp(payload + offset, needle, needle_size) == 0) return true;
  }
  return false;
}

// Native configuration is authoritative on current firmware. Without web
// authentication, do not also expose its legacy text-entity mirror through
// Server-Sent Events: those values can contain reversibly encoded Wifi keys.
inline bool event_payload_is_legacy_panel_config(const char *payload,
                                                 size_t payload_size) {
  const bool config_entity =
      event_payload_contains(payload, payload_size,
                             "\"id\":\"text-button_") ||
      event_payload_contains(payload, payload_size,
                             "\"id\":\"text-subpage_");
  return config_entity &&
         event_payload_contains(payload, payload_size, "_config");
}

}  // namespace esphome::web_server_idf
