#pragma once

#include <cmath>
#include <cstdio>
#include <cstdlib>

// A read-only Home Assistant state-of-charge card. Both grid surfaces use the
// same lifecycle driver and the managed HA subscription registry.
namespace espcontrol::cards {

inline bool battery_driver_matches(const Context &context) {
  return context.runtime.driver == card_runtime::CardDriverId::BATTERY;
}

inline const char *battery_icon_for_soc(int soc) {
  if (soc <= 5) return find_icon("Battery Alert");
  if (soc < 15) return find_icon("Battery 10%");
  if (soc < 25) return find_icon("Battery 20%");
  if (soc < 35) return find_icon("Battery 30%");
  if (soc < 45) return find_icon("Battery 40%");
  if (soc < 55) return find_icon("Battery 50%");
  if (soc < 65) return find_icon("Battery 60%");
  if (soc < 75) return find_icon("Battery 70%");
  if (soc < 85) return find_icon("Battery 80%");
  if (soc < 95) return find_icon("Battery 90%");
  return find_icon("Battery");
}

inline void battery_driver_update(lv_obj_t *icon, lv_obj_t *label, esphome::StringRef state) {
  const std::string text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
  char *end = nullptr;
  const float value = std::strtof(text.c_str(), &end);
  if (text.size() != state.size() || end == text.c_str() || *end != '\0' || !std::isfinite(value)) {
    lv_label_set_display_text(icon, find_icon("Battery Unknown"));
    lv_label_set_display_text(label, "--%");
  } else {
    // Clamp before converting to int, including extremely large finite states.
    const int soc = value <= 0 ? 0 : value >= 100 ? 100 : static_cast<int>(value + 0.5f);
    char percentage[16];
    std::snprintf(percentage, sizeof(percentage), "%d%%", soc);
    lv_label_set_display_text(icon, battery_icon_for_soc(soc));
    lv_label_set_display_text(label, percentage);
  }
  notify_dashboard_content_changed();
}

inline bool battery_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &, const Context &context, const CardPalette &palette) {
  if (!battery_driver_matches(context)) return false;
  if (palette.has_sensor_color) {
    lv_obj_set_style_bg_color(slot.btn, lv_color_hex(palette.sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) |
        static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(slot.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_display_text(slot.icon_lbl, find_icon("Battery Unknown"));
  lv_label_set_display_text(slot.text_lbl, "--%");
  return true;
}

inline bool battery_driver_attach_interaction(BtnSlot &slot, const ParsedCfg &, const Context &context) {
  if (!battery_driver_matches(context)) return false;
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
  return true;
}

inline bool battery_driver_bind_data(BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  if (!battery_driver_matches(context)) return false;
  if (config.entity.empty()) return true;
  const uint32_t generation = ha_subscription_generation();
  ha_subscribe_state(config.entity,
    std::function<void(esphome::StringRef)>(
      [icon = slot.icon_lbl, label = slot.text_lbl, generation](esphome::StringRef state) {
        if (generation != ha_subscription_generation()) return;
        battery_driver_update(icon, label, state);
      }));
  return true;
}

inline bool battery_driver_refresh_layout(BtnSlot &, const ParsedCfg &, const Context &context) {
  return battery_driver_matches(context);
}

inline bool battery_driver_cleanup(BtnSlot &, const ParsedCfg &, const Context &context) {
  // No dynamic state; the shared registry releases subscriptions on rebuild.
  return battery_driver_matches(context);
}

}  // namespace espcontrol::cards
