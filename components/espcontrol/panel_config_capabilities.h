#pragma once

#include <cstdio>

#include "panel_config_document.h"

namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_API_VERSION = 1;
constexpr uint16_t PANEL_CONFIG_WEB_ASSET_VERSION = 1;
constexpr size_t PANEL_CONFIG_CAPABILITIES_MAX_JSON_BYTES = 160;
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

// The delivery marker lets the hosted bridge distinguish firmware that
// understands the versioned web-asset manifest from earlier installations.
inline bool write_panel_config_capabilities_json(char *output,
                                                 size_t output_capacity,
                                                 size_t *output_size) {
  if (output == nullptr || output_size == nullptr || output_capacity == 0)
    return false;
  const int written = std::snprintf(
      output, output_capacity,
      "{\"api\":{\"version\":%u},\"configuration\":{\"document_versions\":[%u],"
      "\"read\":%s,\"write\":%s},\"web_assets\":{\"versions\":[%u],"
      "\"delivery\":\"%s\"}}",
      static_cast<unsigned>(PANEL_CONFIG_API_VERSION),
      static_cast<unsigned>(PANEL_CONFIG_DOCUMENT_VERSION),
      panel_config_read_supported() ? "true" : "false",
      panel_config_write_supported() ? "true" : "false",
      static_cast<unsigned>(PANEL_CONFIG_WEB_ASSET_VERSION),
      PANEL_CONFIG_WEB_ASSET_DELIVERY);
  if (written < 0 || static_cast<size_t>(written) >= output_capacity)
    return false;
  *output_size = static_cast<size_t>(written);
  return true;
}

}  // namespace espcontrol::configuration
