#pragma once

#include "device_ui_limits.h"

// Production builds select their profile in generated device packages. Retain
// the historical fallback for standalone consumers and host-test harnesses.
#ifndef ESPCONTROL_MAX_GRID_SLOTS
#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
#define ESPCONTROL_MAX_GRID_SLOTS ESPCONTROL_DEVICE_UI_SLOT_COUNT
#else
#define ESPCONTROL_MAX_GRID_SLOTS 25
#endif
#endif

#ifdef ESPCONTROL_DEVICE_UI_SLOT_COUNT
static_assert(ESPCONTROL_MAX_GRID_SLOTS == ESPCONTROL_DEVICE_UI_SLOT_COUNT,
              "Grid capacity must match the generated device UI profile");
#endif

constexpr int MAX_GRID_SLOTS = ESPCONTROL_MAX_GRID_SLOTS;
static_assert(MAX_GRID_SLOTS > 0, "ESPCONTROL_MAX_GRID_SLOTS must be positive");
// Keep the existing conservative per-page scratch capacity during migration.
// DeviceUiLimits::max_child_cards() describes the actual configuration bound.
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;
