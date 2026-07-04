// =============================================================================
// BATTERY STATUS - Clock-bar battery icon (experimental)
// =============================================================================
#pragma once

#include <cmath>

constexpr const char *BATTERY_ICON_UNKNOWN = "\U000F0091";  // Battery Unknown
constexpr const char *BATTERY_ICON_ALERT = "\U000F0083";    // Battery Alert

// Ten evenly-spaced 10%-wide tiers, each keyed by its upper bound.
struct BatteryIconTier {
  float max_pct;
  const char *icon;
};
constexpr BatteryIconTier BATTERY_ICON_TIERS[] = {
    {15.0f, "\U000F007A"},  // Battery 10%
    {25.0f, "\U000F007B"},  // Battery 20%
    {35.0f, "\U000F007C"},  // Battery 30%
    {45.0f, "\U000F007D"},  // Battery 40%
    {55.0f, "\U000F007E"},  // Battery 50%
    {65.0f, "\U000F007F"},  // Battery 60%
    {75.0f, "\U000F0080"},  // Battery 70%
    {85.0f, "\U000F0081"},  // Battery 80%
    {95.0f, "\U000F0082"},  // Battery 90%
};
constexpr const char *BATTERY_ICON_FULL = "\U000F0079";

inline const char *battery_status_icon(float pct) {
  if (!std::isfinite(pct)) return BATTERY_ICON_UNKNOWN;
  if (pct <= 5.0f) return BATTERY_ICON_ALERT;
  for (const auto &tier : BATTERY_ICON_TIERS) {
    if (pct < tier.max_pct) return tier.icon;
  }
  return BATTERY_ICON_FULL;
}

inline void battery_status_set_icon(lv_obj_t *label, float pct) {
  if (!label) return;
  lv_label_set_text(label, battery_status_icon(pct));
}
