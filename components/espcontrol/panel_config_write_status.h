#pragma once

#include "configuration_service.h"

namespace espcontrol::configuration {

enum class PanelConfigWriteResponse : uint8_t {
  NO_CONTENT,
  ACCEPTED_WITH_LEGACY_WARNING,
  BAD_REQUEST,
  GENERATION_CONFLICT,
  INTERNAL_ERROR,
};

inline PanelConfigWriteResponse panel_config_write_response(
    ServiceStatus status) {
  switch (status) {
    case ServiceStatus::OK:
      return PanelConfigWriteResponse::NO_CONTENT;
    case ServiceStatus::LEGACY_MIRROR_FAILED:
      return PanelConfigWriteResponse::ACCEPTED_WITH_LEGACY_WARNING;
    case ServiceStatus::INVALID_DOCUMENT:
    case ServiceStatus::UNSUPPORTED_VERSION:
    case ServiceStatus::INVALID_ARGUMENT:
      return PanelConfigWriteResponse::BAD_REQUEST;
    case ServiceStatus::GENERATION_CONFLICT:
      return PanelConfigWriteResponse::GENERATION_CONFLICT;
    default:
      return PanelConfigWriteResponse::INTERNAL_ERROR;
  }
}

}  // namespace espcontrol::configuration
