#pragma once

// Firmware-facing card metadata boundary. Card behavior stays in the existing
// card files; shared identity, defaults, families, and capability checks flow
// through this helper.

#include "button_grid_contract_generated.h"

inline const char *card_runtime_label(const std::string &type) {
  return card_contract_card_label(type);
}

inline bool card_runtime_allow_in_subpage(const std::string &type) {
  return card_contract_allow_in_subpage(type);
}

inline std::string card_runtime_subpage_type_from_code(const std::string &code) {
  return card_contract_subpage_type_from_code(code);
}

inline const char *card_runtime_default_icon_name(const std::string &type) {
  return card_contract_default_icon_name(type);
}

inline const char *card_runtime_default_icon_on_name(const std::string &type) {
  return card_contract_default_icon_on_name(type);
}

inline bool card_runtime_brightness_slider_type(const std::string &type) {
  return card_contract_is_brightness_slider_type(type);
}

inline bool card_runtime_fan_card_type(const std::string &type) {
  return card_contract_is_fan_card_type(type);
}

inline const char *card_runtime_fan_default_icon_name(const std::string &type) {
  return card_contract_fan_default_icon_name(type);
}

inline const char *card_runtime_fan_default_icon_on_name(const std::string &type) {
  return card_contract_fan_default_icon_on_name(type);
}

inline bool card_runtime_option_select_action(const std::string &action) {
  return card_contract_is_option_select_action(action);
}

inline const char *card_runtime_option_select_canonical_action() {
  return CARD_CONTRACT_OPTION_SELECT_ACTION;
}

inline bool card_runtime_large_numbers_supported(const std::string &type,
                                                 const std::string &precision) {
  return card_contract_large_numbers_supported(type, precision);
}

inline bool card_runtime_cover_mode_valid(const std::string &mode) {
  return card_contract_cover_mode_valid(mode);
}

inline bool card_runtime_cover_toggle_mode(const std::string &mode) {
  return mode == "toggle" && card_runtime_cover_mode_valid(mode);
}

inline bool card_runtime_cover_tilt_mode(const std::string &mode) {
  return mode == "tilt" && card_runtime_cover_mode_valid(mode);
}

inline bool card_runtime_cover_modal_mode(const std::string &mode) {
  return mode == "modal" && card_runtime_cover_mode_valid(mode);
}

inline bool card_runtime_cover_command_mode(const std::string &mode) {
  return card_runtime_cover_mode_valid(mode) &&
         mode != "" && mode != "toggle" && mode != "tilt" && mode != "modal";
}

inline const char *card_runtime_cover_command_service(const std::string &mode) {
  return card_contract_cover_command_service(mode);
}

inline bool card_runtime_garage_mode_valid(const std::string &mode) {
  return card_contract_garage_mode_valid(mode);
}

inline bool card_runtime_garage_command_mode(const std::string &mode) {
  return card_runtime_garage_mode_valid(mode) && !mode.empty();
}

inline std::string card_runtime_garage_label_display(const std::string &value) {
  return card_contract_garage_label_display_valid(value)
    ? value
    : CARD_CONTRACT_GARAGE_LABEL_DISPLAY_DEFAULT;
}

inline bool card_runtime_lock_mode_valid(const std::string &mode) {
  return card_contract_lock_mode_valid(mode);
}

inline bool card_runtime_lock_command_mode(const std::string &mode) {
  return card_runtime_lock_mode_valid(mode) && !mode.empty();
}

inline const char *card_runtime_lock_command_service(const std::string &mode) {
  return card_contract_lock_command_service(mode);
}

inline const char *card_runtime_lock_toggle_service(const std::string &state) {
  return card_contract_lock_toggle_service(state);
}

inline bool card_runtime_internal_push_mode(const std::string &mode) {
  return mode == "push" && card_contract_internal_mode_valid(mode);
}

inline std::string card_runtime_media_mode(const std::string &mode) {
  if (card_contract_media_mode_valid(mode)) return mode;
  if (card_contract_media_legacy_mode(mode)) return CARD_CONTRACT_MEDIA_DEFAULT_MODE;
  return CARD_CONTRACT_MEDIA_DEFAULT_MODE;
}

inline bool card_runtime_media_playback_button_mode(const std::string &mode) {
  std::string normalized = card_runtime_media_mode(mode);
  return normalized == "play_pause" || normalized == "previous" || normalized == "next";
}

inline const char *card_runtime_media_playback_service(const std::string &mode) {
  return card_contract_media_playback_service(card_runtime_media_mode(mode));
}

inline bool card_runtime_media_state_display_mode(const std::string &mode) {
  return card_contract_media_state_display_mode(card_runtime_media_mode(mode));
}

inline bool card_runtime_media_now_playing_control(const std::string &precision) {
  return card_contract_string_in(precision, CARD_CONTRACT_MEDIA_NOW_PLAYING_CONTROLS,
    sizeof(CARD_CONTRACT_MEDIA_NOW_PLAYING_CONTROLS) / sizeof(CARD_CONTRACT_MEDIA_NOW_PLAYING_CONTROLS[0]));
}

inline bool card_runtime_alarm_action_mode_valid(const std::string &mode) {
  return card_contract_alarm_action_mode_valid(mode);
}

inline size_t card_runtime_alarm_action_mode_count() {
  return sizeof(CARD_CONTRACT_ALARM_ACTION_MODES) / sizeof(CARD_CONTRACT_ALARM_ACTION_MODES[0]);
}

inline const char *card_runtime_alarm_action_mode_at(size_t index) {
  return index < card_runtime_alarm_action_mode_count()
    ? CARD_CONTRACT_ALARM_ACTION_MODES[index]
    : "";
}

inline const char *card_runtime_alarm_action_service(const std::string &mode) {
  return card_contract_alarm_action_service(mode);
}

inline const char *card_runtime_alarm_action_icon_name(const std::string &mode) {
  return card_contract_alarm_action_icon_name(mode);
}

inline bool card_runtime_alarm_action_legacy_icon_name(const std::string &mode,
                                                       const std::string &icon) {
  return card_contract_alarm_action_legacy_icon_name(mode, icon);
}

inline std::string card_runtime_alarm_icon_display(const std::string &value) {
  return card_contract_alarm_icon_display_valid(value)
    ? value
    : CARD_CONTRACT_ALARM_ICON_DISPLAY_DEFAULT;
}

inline std::string card_runtime_alarm_label_display(const std::string &value) {
  return card_contract_alarm_label_display_valid(value)
    ? value
    : CARD_CONTRACT_ALARM_LABEL_DISPLAY_DEFAULT;
}

inline std::string card_runtime_climate_label_display(const std::string &value) {
  return card_contract_climate_label_display_valid(value)
    ? value
    : CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_DEFAULT;
}

inline std::string card_runtime_climate_number_display(const std::string &value) {
  return card_contract_climate_number_display_valid(value)
    ? value
    : CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_DEFAULT;
}

inline bool card_runtime_weather_forecast_precision(const std::string &precision) {
  return card_contract_weather_forecast_precision(precision);
}

inline std::string card_runtime_vacuum_mode(const std::string &mode) {
  if (mode == "status" || mode == "start_stop" || mode == "dock" ||
      mode == "pause_resume" || mode == "clean_spot" || mode == "locate" ||
      mode == "clean_area") {
    return mode;
  }
  if (mode == "vacuum.start") return "start_stop";
  if (mode == "vacuum.return_to_base") return "dock";
  return "start_stop";
}

inline bool card_runtime_vacuum_state_mode(const std::string &mode) {
  std::string normalized = card_runtime_vacuum_mode(mode);
  return normalized == "status" || normalized == "start_stop" || normalized == "pause_resume";
}

inline const char *card_runtime_vacuum_default_icon_name(const std::string &mode) {
  std::string normalized = card_runtime_vacuum_mode(mode);
  if (normalized == "dock") return "Robot Vacuum Variant";
  if (normalized == "clean_spot") return "Vacuum";
  if (normalized == "locate") return "Robot Vacuum Alert";
  if (normalized == "clean_area") return "Vacuum Outline";
  return "Robot Vacuum";
}
