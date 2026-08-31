#pragma once

#include "configuration_service.h"

namespace espcontrol::configuration {

// An oversized legacy document cannot be read through the native endpoint
// either, because both paths share the same bounded document buffer. Keep the
// browser on the legacy entity API so ordinary edits remain available. Other
// load failures can still be repaired by replacing the native document.
inline bool panel_config_load_allows_native_endpoints(ServiceStatus status) {
  return status != ServiceStatus::BUFFER_TOO_SMALL;
}

}  // namespace espcontrol::configuration
