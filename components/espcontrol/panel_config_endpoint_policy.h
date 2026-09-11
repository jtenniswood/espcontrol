#pragma once

#include "configuration_service.h"

namespace espcontrol::configuration {

// Native discovery is useful only when the endpoint can read the current
// document (or report an empty store). This also covers oversized legacy
// imports and pre-upgrade NVS blobs whose former slot size is no longer
// readable. Keep the browser on the legacy entity API for every initial load
// failure so ordinary edits remain available.
inline bool panel_config_load_allows_native_endpoints(ServiceStatus status) {
  return status == ServiceStatus::OK ||
         status == ServiceStatus::IMPORTED_LEGACY ||
         status == ServiceStatus::EMPTY;
}

}  // namespace espcontrol::configuration
