#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>

#include "custom_icon_glyphs.h"
#include "panel_config_document.h"

namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_API_VERSION = 1;
// Version 2 editors send the reset epoch on configuration writes.
constexpr uint16_t PANEL_CONFIG_WEB_ASSET_VERSION = 2;
// Size of the response without custom icons; each custom icon adds at most
// PANEL_CONFIG_CAPABILITIES_BYTES_PER_CUSTOM_ICON ("10FFFD" in quotes, comma).
constexpr size_t PANEL_CONFIG_CAPABILITIES_BASE_JSON_BYTES = 384;
constexpr size_t PANEL_CONFIG_CAPABILITIES_BYTES_PER_CUSTOM_ICON = 9;
constexpr const char *PANEL_CONFIG_WEB_ASSET_DELIVERY = "manifest";

inline bool &panel_config_read_supported() {
  static bool supported = false;
  return supported;
}

inline void set_panel_config_read_supported(bool supported) {
  panel_config_read_supported() = supported;
}

inline bool &panel_config_write_supported() {
  static bool supported = false;
  return supported;
}

inline void set_panel_config_write_supported(bool supported) {
  panel_config_write_supported() = supported;
}

// The icons compiled into font_icon_main beyond the built-in set come from
// custom_icon_glyphs.h, the same list find_icon() draws from. The web icon
// picker only offers these, so it never suggests a glyph the display lacks.
inline size_t panel_config_capabilities_json_capacity() {
  return PANEL_CONFIG_CAPABILITIES_BASE_JSON_BYTES +
         custom_icon_glyphs().size() * PANEL_CONFIG_CAPABILITIES_BYTES_PER_CUSTOM_ICON;
}

// The delivery marker lets the hosted bridge distinguish firmware that
// understands the versioned web-asset manifest from earlier installations.
inline bool write_panel_config_capabilities_json(char *output,
                                                 size_t output_capacity,
                                                 size_t *output_size) {
  if (output == nullptr || output_size == nullptr || output_capacity == 0)
    return false;
  int written = std::snprintf(
      output, output_capacity,
      "{\"screensaver_pin\":{\"version\":1},\"identity\":{\"version\":1},\"api\":{\"version\":%u},\"configuration\":{\"document_versions\":[%u],"
      "\"read\":%s,\"write\":%s,\"max_record_body_bytes\":%u},\"web_assets\":{\"versions\":[%u],"
      "\"delivery\":\"%s\"},\"reset\":{\"modes\":[\"customization\",\"factory\"],\"status\":\"/api/v1/reset\"},"
      "\"icons\":{\"custom\":[",
      static_cast<unsigned>(PANEL_CONFIG_API_VERSION),
      static_cast<unsigned>(PANEL_CONFIG_DOCUMENT_VERSION),
      panel_config_read_supported() ? "true" : "false",
      panel_config_write_supported() ? "true" : "false",
      static_cast<unsigned>(PANEL_CONFIG_MAX_RECORD_BODY_BYTES),
      static_cast<unsigned>(PANEL_CONFIG_WEB_ASSET_VERSION),
      PANEL_CONFIG_WEB_ASSET_DELIVERY);
  if (written < 0 || static_cast<size_t>(written) >= output_capacity)
    return false;
  size_t length = static_cast<size_t>(written);
  const std::vector<CustomIconGlyph> &icons = custom_icon_glyphs();
  for (size_t i = 0; i < icons.size(); i++) {
    written = std::snprintf(output + length, output_capacity - length, "%s\"%X\"",
                            i == 0 ? "" : ",", static_cast<unsigned>(icons[i].codepoint));
    if (written < 0 || static_cast<size_t>(written) >= output_capacity - length)
      return false;
    length += static_cast<size_t>(written);
  }
  written = std::snprintf(output + length, output_capacity - length, "]}}");
  if (written < 0 || static_cast<size_t>(written) >= output_capacity - length)
    return false;
  *output_size = length + static_cast<size_t>(written);
  return true;
}

}  // namespace espcontrol::configuration
