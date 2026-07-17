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
#include <cstdio>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <vector>
#include <functional>
#ifdef ESP_PLATFORM
#include "esp_system.h"
#endif
#include "esphome/components/api/homeassistant_service.h"
#include "esphome/components/http_request/http_request.h"
#include "esphome/components/artwork_image/artwork_image.h"
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "esphome/core/string_ref.h"
#include "i18n_generated.h"
#include "icons.h"
#include "backlight.h"

// Public compatibility include. Device YAML includes this file, while the
// implementation is split into focused headers below for easier review.
#include "button_grid_limits.h"
#include "button_grid_string.h"
#include "button_grid_ha.h"
#include "button_grid_config.h"
#include "button_grid_style.h"
#include "button_grid_card_runtime.h"
#include "button_grid_layout.h"
#include "button_grid_display.h"
#include "button_grid_cards.h"
#include "button_grid_modal.h"
#include "button_grid_pin_keypad.h"
#include "button_grid_subscriptions.h"
#include "button_grid_vacuum.h"
#include "button_grid_lawn_mower.h"
#include "button_grid_actions.h"
#include "button_grid_sliders.h"
#include "button_grid_fan.h"
#include "button_grid_climate.h"
#include "button_grid_confirm.h"
#include "button_grid_option_select.h"
#include "button_grid_todo.h"
#include "network_status.h"
#include "button_grid_media.h"
#include "button_grid_subpages.h"
#include "button_grid_alarm.h"
#include "button_grid_navigation.h"
#include "button_grid_grid.h"
