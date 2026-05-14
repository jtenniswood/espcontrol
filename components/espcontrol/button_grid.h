// =============================================================================
// BUTTON GRID - LVGL button grid layout, parsing, and HA entity binding
// =============================================================================
// Shared C++ utilities included by each device's sensors.yaml lambda. Handles:
//   - Parsing semicolon-delimited button config strings into structured fields
//   - Grid layout with double-height (d), double-wide (w), and 2×2 big (b) cells
//   - LVGL visual setup for toggle buttons, sensor/time cards, and slider widgets
//   - Home Assistant state subscriptions and action dispatch
//   - Subpage creation (nested grid screens with back button)
// =============================================================================
#pragma once
#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <vector>
#include <functional>
#include "esphome/components/api/homeassistant_service.h"
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "esphome/core/string_ref.h"
#include "icons.h"
#include "backlight.h"

// Public compatibility include. Device YAML includes this file, while the
// implementation is split into focused headers below for easier review.
#include "button_grid_config.h"
#include "button_grid_layout.h"
#include "button_grid_cards.h"
#include "button_grid_subscriptions.h"
#include "button_grid_actions.h"
#include "button_grid_sliders.h"
#include "button_grid_fan.h"
#include "button_grid_climate.h"
#include "button_grid_confirm.h"
#include "network_status.h"
#include "button_grid_media.h"
#include "button_grid_subpages.h"
#include "button_grid_alarm.h"
#include "button_grid_grid.h"
