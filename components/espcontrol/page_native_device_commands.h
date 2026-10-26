#pragma once

// Include after button_grid.h. Selection was validated by the actor; dispatch
// consumes its owned config and never consults a legacy widget/context.
#include "page_device_command_policy.h"

namespace espcontrol::pages {
inline bool send_page_device_action(const ParsedCfg& config, PageDeviceAction action) {
  using A = PageDeviceAction;
  if (config.entity.empty()) return false;
  if (action == A::COVER || action == A::COVER_TILT)
    return send_cover_command_action(config, action == A::COVER_TILT);
  const auto* service = page_device_service(action);
  if (!service) return false;
  if (action == A::OSCILLATE_ON || action == A::OSCILLATE_OFF)
    return ha_send_entity_action(config.entity, service, "oscillating",
                                 action == A::OSCILLATE_ON ? "true" : "false");
  if (action == A::FORWARD || action == A::REVERSE)
    return ha_send_entity_action(config.entity, service, "direction",
                                 action == A::FORWARD ? "forward" : "reverse");
  if (action == A::VACUUM_AREA) {
    if (config.unit.empty()) return false;
    return ha_send_entity_action(config.entity, service, "cleaning_area_id", config.unit.c_str());
  }
  return ha_send_entity_action(config.entity, service);
}
}  // namespace espcontrol::pages
