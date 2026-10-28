#pragma once

#include <array>

#include "configuration_service.h"

namespace espcontrol::configuration {

// The authoritative release policy source is product/release_contract.json.
// Keep this value in lock-step with that declaration; check_release_contract.py
// protects the relationship before a release is published.
constexpr LegacyConfigurationMode PANEL_CONFIG_LEGACY_MODE =
    LegacyConfigurationMode::DUAL_WRITE;
constexpr std::array<uint16_t, 2> PANEL_CONFIG_STORAGE_READ_VERSIONS = {1, 2};
constexpr uint16_t PANEL_CONFIG_DURABLE_WRITE_VERSION = 1;
static_assert(PANEL_CONFIG_DURABLE_WRITE_VERSION == CURRENT_CONFIGURATION_DOCUMENT_VERSION);

}  // namespace espcontrol::configuration
