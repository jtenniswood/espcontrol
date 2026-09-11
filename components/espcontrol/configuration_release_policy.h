#pragma once

#include "configuration_service.h"

namespace espcontrol::configuration {

// The authoritative release policy source is product/release_contract.json.
// Keep this value in lock-step with that declaration; check_release_contract.py
// protects the relationship before a release is published.
constexpr LegacyConfigurationMode PANEL_CONFIG_LEGACY_MODE =
    LegacyConfigurationMode::DUAL_WRITE;

}  // namespace espcontrol::configuration
