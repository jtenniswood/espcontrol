#pragma once

#include <cstdio>

#include "panel_config_document.h"

namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_API_VERSION = 1;
// Version 2 editors send the reset epoch on configuration writes.
constexpr uint16_t PANEL_CONFIG_WEB_ASSET_VERSION = 2;
constexpr size_t PANEL_CONFIG_CAPABILITIES_MAX_JSON_BYTES = 384;
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

// True when the firmware was compiled with icons added to font_icon_main
// beyond the built-in set. The web icon picker only offers custom icons then,
// so official prebuilt firmware never shows them.
inline bool &panel_config_custom_icons_available() {
  static bool available = false;
  return available;
}

inline void set_panel_config_custom_icons_available(bool available) {
  panel_config_custom_icons_available() = available;
}

// The delivery marker lets the hosted bridge distinguish firmware that
// understands the versioned web-asset manifest from earlier installations.
inline bool write_panel_config_capabilities_json(char *output,
                                                 size_t output_capacity,
                                                 size_t *output_size) {
  if (output == nullptr || output_size == nullptr || output_capacity == 0)
    return false;
  const int written = std::snprintf(
      output, output_capacity,
      "{\"screensaver_pin\":{\"version\":1},\"identity\":{\"version\":1},\"api\":{\"version\":%u},\"configuration\":{\"document_versions\":[%u],"
      "\"read\":%s,\"write\":%s,\"max_record_body_bytes\":%u},\"web_assets\":{\"versions\":[%u],"
      "\"delivery\":\"%s\"},\"reset\":{\"modes\":[\"customization\",\"factory\"],\"status\":\"/api/v1/reset\"},"
      "\"icons\":{\"custom\":%s}}",
      static_cast<unsigned>(PANEL_CONFIG_API_VERSION),
      static_cast<unsigned>(PANEL_CONFIG_DOCUMENT_VERSION),
      panel_config_read_supported() ? "true" : "false",
      panel_config_write_supported() ? "true" : "false",
      static_cast<unsigned>(PANEL_CONFIG_MAX_RECORD_BODY_BYTES),
      static_cast<unsigned>(PANEL_CONFIG_WEB_ASSET_VERSION),
      PANEL_CONFIG_WEB_ASSET_DELIVERY,
      panel_config_custom_icons_available() ? "true" : "false");
  if (written < 0 || static_cast<size_t>(written) >= output_capacity)
    return false;
  *output_size = static_cast<size_t>(written);
  return true;
}

}  // namespace espcontrol::configuration
