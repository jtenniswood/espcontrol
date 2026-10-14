#pragma once

#include <cstdio>

#include "panel_config_document.h"
#include "wallpaper_format.h"

namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_API_VERSION = 1;
// Version 2 editors send the reset epoch on configuration writes.
constexpr uint16_t PANEL_CONFIG_WEB_ASSET_VERSION = 2;
constexpr size_t PANEL_CONFIG_CAPABILITIES_MAX_JSON_BYTES = 512;
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
                                                 size_t *output_size,
                                                 bool wallpaper_available = false) {
  if (output == nullptr || output_size == nullptr || output_capacity == 0)
    return false;
  const int written = std::snprintf(
      output, output_capacity,
      "{\"background\":{\"version\":1},\"wallpaper\":{\"version\":1,\"available\":%s,\"opacity\":true,\"state_opacity\":true,\"jpeg\":%s,\"max_side\":%u,\"max_pixels\":%u,\"max_bytes\":%u},\"screensaver_pin\":{\"version\":1},\"identity\":{\"version\":1},\"api\":{\"version\":%u},\"configuration\":{\"document_versions\":[%u],"
      "\"read\":%s,\"write\":%s},\"web_assets\":{\"versions\":[%u],"
      "\"delivery\":\"%s\"},\"reset\":{\"modes\":[\"customization\",\"factory\"],\"status\":\"/api/v1/reset\"}}",
      wallpaper_available ? "true" : "false",
#ifdef USE_ARTWORK_IMAGE_JPEG_SUPPORT
      "true",
#else
      "false",
#endif
      static_cast<unsigned>(WALLPAPER_JPEG_MAX_SIDE),
      static_cast<unsigned>(WALLPAPER_JPEG_MAX_PIXELS),
      static_cast<unsigned>(WALLPAPER_MAX_BYTES),
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
