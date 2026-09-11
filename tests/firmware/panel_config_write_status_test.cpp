#include "panel_config_write_status.h"

int main() {
  using espcontrol::configuration::PanelConfigWriteResponse;
  using espcontrol::configuration::ServiceStatus;
  using espcontrol::configuration::panel_config_write_response;

  return panel_config_write_response(ServiceStatus::OK) ==
                 PanelConfigWriteResponse::NO_CONTENT &&
         panel_config_write_response(ServiceStatus::LEGACY_MIRROR_FAILED) ==
                 PanelConfigWriteResponse::ACCEPTED_WITH_LEGACY_WARNING &&
         panel_config_write_response(ServiceStatus::INVALID_DOCUMENT) ==
                 PanelConfigWriteResponse::BAD_REQUEST &&
         panel_config_write_response(ServiceStatus::UNSUPPORTED_VERSION) ==
                 PanelConfigWriteResponse::BAD_REQUEST &&
         panel_config_write_response(ServiceStatus::INVALID_ARGUMENT) ==
                 PanelConfigWriteResponse::BAD_REQUEST &&
         panel_config_write_response(ServiceStatus::GENERATION_CONFLICT) ==
                 PanelConfigWriteResponse::GENERATION_CONFLICT &&
         panel_config_write_response(ServiceStatus::STORE_FAILED) ==
                 PanelConfigWriteResponse::INTERNAL_ERROR &&
         panel_config_write_response(ServiceStatus::RUNTIME_APPLY_FAILED) ==
                 PanelConfigWriteResponse::INTERNAL_ERROR
             ? 0
             : 1;
}
