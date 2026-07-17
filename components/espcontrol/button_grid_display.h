#pragma once

#include "button_grid_modal_layout.h"

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Shared display-profile helpers for firmware card rendering. Device YAML still
// fills GridConfig; runtime code reads normalized display tokens from here.

struct DisplayFontRoles {
  const lv_font_t *icon = nullptr;
  const lv_font_t *sensor = nullptr;
  const lv_font_t *large_sensor = nullptr;
  const lv_font_t *media_title = nullptr;
  const lv_font_t *media_control_title = nullptr;
  const lv_font_t *media_control_artist = nullptr;
  const lv_font_t *media_cover_art_title = nullptr;
  const lv_font_t *media_cover_art_artist = nullptr;
  const lv_font_t *option_select_value = nullptr;
  const lv_font_t *volume_number = nullptr;
  const lv_font_t *volume_label = nullptr;
  const lv_font_t *climate_card_icon = nullptr;
  const lv_font_t *climate_option_title = nullptr;
  const lv_font_t *climate_option_value = nullptr;
  const lv_font_t *volume_icon = nullptr;
};

struct DisplayWidthTokens {
  bool vertical_axis = false;
  int main_percent = 100;
  int volume_percent = 100;
};

struct DisplayLargeNumberTokens {
  const lv_font_t *font = nullptr;
  int unit_offset_percent = -10;
};

struct DisplayColorCorrectionTokens {
  int red_percent = COLOR_CORRECTION_RED_PERCENT;
  int green_percent = COLOR_CORRECTION_GREEN_PERCENT;
  int blue_percent = COLOR_CORRECTION_BLUE_PERCENT;
};

constexpr lv_coord_t DISPLAY_MODAL_REFERENCE_SIDE_PX = 480;
constexpr lv_coord_t DISPLAY_MODAL_ARC_STROKE_REF_PX = 17;
constexpr lv_coord_t DISPLAY_MODAL_BACK_BUTTON_REF_PX = 46;
constexpr lv_coord_t DISPLAY_MODAL_BUTTON_REF_PX = 80;
constexpr lv_coord_t DISPLAY_MODAL_INSET_REF_PX = 18;
constexpr lv_coord_t DISPLAY_MODAL_CONTROLS_GAP_REF_PX = 24;
constexpr lv_coord_t DISPLAY_MODAL_CONTROLS_DOWN_REF_PX = 22;
constexpr lv_coord_t DISPLAY_MODAL_TITLE_GAP_REF_PX = 10;

using DisplayModalLayoutFamily = espcontrol::modal::LayoutFamily;
using DisplayModalDensity = espcontrol::modal::Density;
using DisplayModalMemoryTier = espcontrol::modal::MemoryTier;
using DisplayModalProfile = espcontrol::modal::DeviceProfile;

struct DisplayProfile {
  DisplayFontRoles fonts;
  DisplayWidthTokens width;
  DisplayLargeNumberTokens large_numbers;
  DisplayColorCorrectionTokens color;
  DisplayModalProfile modal;
};

inline int display_width_percent(int percent) {
  return normalize_width_compensation_percent(percent);
}

inline void display_set_width_axis(const DisplayProfile &profile) {
  set_width_compensation_vertical_axis(profile.width.vertical_axis);
}

inline DisplayModalProfile &display_active_modal_profile() {
  static DisplayModalProfile profile;
  return profile;
}

inline void display_activate_profile(const DisplayProfile &profile) {
  display_set_width_axis(profile);
  display_active_modal_profile() = profile.modal;
}

inline bool display_modal_uses_family(const DisplayModalProfile &profile,
                                      DisplayModalLayoutFamily family) {
  return espcontrol::modal::uses_family(profile, family);
}

inline bool display_modal_is_square_family(const DisplayModalProfile &profile) {
  return espcontrol::modal::is_square_family(profile);
}

inline bool display_modal_is_constrained(const DisplayModalProfile &profile) {
  return espcontrol::modal::is_constrained(profile);
}

inline int display_main_width_percent(const DisplayProfile &profile) {
  return display_width_percent(profile.width.main_percent);
}

inline int display_volume_width_percent(const DisplayProfile &profile) {
  return display_width_percent(profile.width.volume_percent);
}

inline void display_apply_main_width(lv_obj_t *obj, const DisplayProfile &profile) {
  apply_width_compensation(obj, display_main_width_percent(profile));
}

inline void display_apply_slot_text_width(const BtnSlot &slot, const DisplayProfile &profile) {
  apply_slot_text_width_compensation(slot, display_main_width_percent(profile));
}

inline uint32_t display_correct_color(uint32_t rgb, const DisplayProfile &profile) {
  return correct_display_color(
    rgb, profile.color.red_percent, profile.color.green_percent,
    profile.color.blue_percent);
}

inline const lv_font_t *display_icon_font(const DisplayProfile &profile) {
  return profile.fonts.icon;
}

inline const lv_font_t *display_sensor_font(const DisplayProfile &profile) {
  return profile.fonts.sensor;
}

inline const lv_font_t *display_large_sensor_font(const DisplayProfile &profile) {
  return profile.large_numbers.font;
}

inline int display_large_sensor_unit_offset_percent(const DisplayProfile &profile) {
  return profile.large_numbers.unit_offset_percent;
}

inline const lv_font_t *display_media_title_font(const DisplayProfile &profile) {
  return profile.fonts.media_title ? profile.fonts.media_title : profile.fonts.sensor;
}

inline const lv_font_t *display_media_control_title_font(const DisplayProfile &profile) {
  return profile.fonts.media_control_title
    ? profile.fonts.media_control_title
    : display_media_title_font(profile);
}

inline const lv_font_t *display_media_control_artist_font(
    const DisplayProfile &profile, const lv_font_t *fallback = nullptr) {
  if (profile.fonts.media_control_artist) return profile.fonts.media_control_artist;
  return profile.fonts.volume_label ? profile.fonts.volume_label : fallback;
}

inline const lv_font_t *display_media_cover_art_title_font(
    const DisplayProfile &profile) {
  return profile.fonts.media_cover_art_title
    ? profile.fonts.media_cover_art_title
    : display_media_title_font(profile);
}

inline const lv_font_t *display_media_cover_art_artist_font(
    const DisplayProfile &profile, const lv_font_t *fallback = nullptr) {
  return profile.fonts.media_cover_art_artist
    ? profile.fonts.media_cover_art_artist
    : fallback;
}

inline const lv_font_t *display_optional_media_title_font(const DisplayProfile &profile) {
  return profile.fonts.media_title;
}

inline const lv_font_t *display_media_title_font_or(
    const DisplayProfile &profile, const lv_font_t *fallback) {
  return profile.fonts.media_title ? profile.fonts.media_title : fallback;
}

inline const lv_font_t *display_option_select_value_font_or(
    const DisplayProfile &profile, const lv_font_t *fallback) {
  if (profile.fonts.option_select_value) return profile.fonts.option_select_value;
  return display_media_title_font_or(profile, fallback);
}

inline const lv_font_t *display_switch_confirmation_message_font(const DisplayProfile &profile) {
  return profile.fonts.media_title ? profile.fonts.media_title : profile.fonts.volume_label;
}

inline const lv_font_t *display_volume_number_font(const DisplayProfile &profile) {
  return profile.fonts.volume_number ? profile.fonts.volume_number : profile.fonts.sensor;
}

inline const lv_font_t *display_volume_label_font(
    const DisplayProfile &profile, const lv_font_t *fallback = nullptr) {
  return profile.fonts.volume_label ? profile.fonts.volume_label : fallback;
}

inline const lv_font_t *display_climate_card_icon_font(const DisplayProfile &profile) {
  return profile.fonts.climate_card_icon ? profile.fonts.climate_card_icon : profile.fonts.icon;
}

inline const lv_font_t *display_climate_option_title_font(
    const DisplayProfile &profile, const lv_font_t *fallback = nullptr) {
  return profile.fonts.climate_option_title ? profile.fonts.climate_option_title : fallback;
}

inline const lv_font_t *display_climate_option_value_font(
    const DisplayProfile &profile, const lv_font_t *fallback = nullptr) {
  return profile.fonts.climate_option_value ? profile.fonts.climate_option_value : fallback;
}

inline lv_coord_t display_modal_scaled_px(lv_coord_t px, lv_coord_t short_side) {
  return px * short_side / DISPLAY_MODAL_REFERENCE_SIDE_PX;
}
