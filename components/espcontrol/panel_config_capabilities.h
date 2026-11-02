#pragma once

#include <cstdio>

#include "panel_config_document.h"
#include "panel_config_representation.h"
#include "configuration_release_policy.h"
#include "device_ui_limits.h"

namespace espcontrol::configuration {

constexpr uint16_t PANEL_CONFIG_API_VERSION = 1;
// Version 2 editors send the reset epoch on configuration writes.
constexpr uint16_t PANEL_CONFIG_WEB_ASSET_VERSION = 2;
constexpr size_t PANEL_CONFIG_CAPABILITIES_MAX_JSON_BYTES = 896;
// Reader depth and writable depth are separate rollout capabilities. A
// depth-two reader must ship before a release can enable durable graph writes.
struct PanelConfigResourceLimits {
  size_t cards{0}, pages{0}, page_store_bytes{0}, state_metadata_bytes{0}, ha_state_bytes{0};
};
#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
constexpr unsigned PANEL_CONFIG_READ_NAVIGATION_DEPTH = DEVICE_UI_LIMITS.max_navigation_depth;
constexpr PanelConfigResourceLimits PANEL_CONFIG_RESOURCE_LIMITS{
    DEVICE_UI_LIMITS.max_cards(), DEVICE_UI_LIMITS.max_pages(), DEVICE_UI_LIMITS.page_store_bytes,
    DEVICE_UI_LIMITS.state_metadata_bytes, DEVICE_UI_LIMITS.ha_state_bytes};
#else
constexpr unsigned PANEL_CONFIG_READ_NAVIGATION_DEPTH = 2;
constexpr PanelConfigResourceLimits PANEL_CONFIG_RESOURCE_LIMITS{};
#endif
constexpr unsigned PANEL_CONFIG_WRITE_NAVIGATION_DEPTH =
    PANEL_CONFIG_DURABLE_WRITE_VERSION == 2 ? PANEL_CONFIG_READ_NAVIGATION_DEPTH : 1;
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
      "{\"screensaver_pin\":{\"version\":1},\"identity\":{\"version\":1},\"api\":{\"version\":%u},\"configuration\":{\"document_versions\":[%s],\"default_document_version\":1,\"durable_document_version\":%u,\"max_v2_document_bytes\":%u,\"max_navigation_depth\":%u,\"writable_navigation_depth\":%u,"
      "\"max_runtime_cards\":%u,\"max_runtime_pages\":%u,\"max_page_store_bytes\":%u,\"max_state_metadata_bytes\":%u,\"max_ha_state_bytes\":%u,"
      "\"read\":%s,\"write\":%s,\"max_record_body_bytes\":%u,\"preserves_inactive_records\":%s},\"web_assets\":{\"versions\":[%u],"
      "\"delivery\":\"%s\"},\"reset\":{\"modes\":[\"customization\",\"factory\"],\"status\":\"/api/v1/reset\"}}",
      static_cast<unsigned>(PANEL_CONFIG_API_VERSION),
      panel_config_read_supported() && panel_config_write_supported() ? "1,2" : "1",
      static_cast<unsigned>(PANEL_CONFIG_DURABLE_WRITE_VERSION),
      static_cast<unsigned>(PANEL_CONFIG_V2_HTTP_MAX_BYTES),
      PANEL_CONFIG_READ_NAVIGATION_DEPTH,
      PANEL_CONFIG_WRITE_NAVIGATION_DEPTH,
      static_cast<unsigned>(PANEL_CONFIG_RESOURCE_LIMITS.cards),
      static_cast<unsigned>(PANEL_CONFIG_RESOURCE_LIMITS.pages),
      static_cast<unsigned>(PANEL_CONFIG_RESOURCE_LIMITS.page_store_bytes),
      static_cast<unsigned>(PANEL_CONFIG_RESOURCE_LIMITS.state_metadata_bytes),
      static_cast<unsigned>(PANEL_CONFIG_RESOURCE_LIMITS.ha_state_bytes),
      panel_config_read_supported() ? "true" : "false",
      panel_config_write_supported() ? "true" : "false",
      static_cast<unsigned>(PANEL_CONFIG_MAX_RECORD_BODY_BYTES),
      panel_config_read_supported() && panel_config_write_supported() ? "true" : "false",
      static_cast<unsigned>(PANEL_CONFIG_WEB_ASSET_VERSION),
      PANEL_CONFIG_WEB_ASSET_DELIVERY);
  if (written < 0 || static_cast<size_t>(written) >= output_capacity)
    return false;
  *output_size = static_cast<size_t>(written);
  return true;
}

}  // namespace espcontrol::configuration
