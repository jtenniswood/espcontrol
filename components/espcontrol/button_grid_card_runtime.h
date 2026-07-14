#pragma once

// Firmware-facing card metadata boundary. Card behavior stays in the existing
// card files; shared identity, defaults, families, and capability checks flow
// through this helper.

#include "button_grid_contract_generated.h"
#include "button_grid_card_registry.h"

inline espcontrol::cards::Registration card_runtime_registration(const std::string &type) {
  const bool legacy_runtime_type = type == "local" || type == "text_sensor" || type == "todo";
  return espcontrol::cards::registration_for_type(
      type, legacy_runtime_type ? true : card_contract_allow_in_subpage(type));
}

inline espcontrol::cards::Family card_runtime_family(const std::string &type) {
  return espcontrol::cards::family_for_type(type);
}

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

constexpr const char *card_runtime_option_name_state_labels() {
  return CARD_CONTRACT_OPTION_NAME_STATE_LABELS;
}

constexpr const char *card_runtime_option_name_state_input() {
  return CARD_CONTRACT_OPTION_NAME_STATE_INPUT;
}

constexpr const char *card_runtime_option_name_state_output() {
  return CARD_CONTRACT_OPTION_NAME_STATE_OUTPUT;
}

constexpr const char *card_runtime_option_name_state_input_2() {
  return CARD_CONTRACT_OPTION_NAME_STATE_INPUT_2;
}

constexpr const char *card_runtime_option_name_state_output_2() {
  return CARD_CONTRACT_OPTION_NAME_STATE_OUTPUT_2;
}

constexpr const char *card_runtime_option_name_state_low_label() {
  return CARD_CONTRACT_OPTION_NAME_STATE_LOW_LABEL;
}

constexpr const char *card_runtime_option_name_state_high_label() {
  return CARD_CONTRACT_OPTION_NAME_STATE_HIGH_LABEL;
}

constexpr const char *card_runtime_option_name_image_label() {
  return CARD_CONTRACT_OPTION_NAME_IMAGE_LABEL;
}

constexpr const char *card_runtime_option_name_image_icon() {
  return CARD_CONTRACT_OPTION_NAME_IMAGE_ICON;
}

constexpr const char *card_runtime_option_name_image_modal_mode() {
  return CARD_CONTRACT_OPTION_NAME_IMAGE_MODAL_MODE;
}

constexpr const char *card_runtime_option_name_image_refresh() {
  return CARD_CONTRACT_OPTION_NAME_IMAGE_REFRESH;
}

constexpr const char *card_runtime_option_name_image_refresh_mode() {
  return CARD_CONTRACT_OPTION_NAME_IMAGE_REFRESH_MODE;
}

constexpr const char *card_runtime_option_name_media_cover_art() {
  return CARD_CONTRACT_OPTION_NAME_MEDIA_COVER_ART;
}

constexpr const char *card_runtime_option_name_cover_art_action() {
  return CARD_CONTRACT_OPTION_NAME_COVER_ART_ACTION;
}

constexpr const char *card_runtime_option_name_light_tabs() {
  return CARD_CONTRACT_OPTION_NAME_LIGHT_TABS;
}

constexpr const char *card_runtime_option_name_cover_tabs() {
  return CARD_CONTRACT_OPTION_NAME_COVER_TABS;
}

constexpr const char *card_runtime_option_name_fan_tabs() {
  return CARD_CONTRACT_OPTION_NAME_FAN_TABS;
}

constexpr const char *card_runtime_option_name_label_display() {
  return CARD_CONTRACT_OPTION_NAME_LABEL_DISPLAY;
}

constexpr const char *card_runtime_option_name_number_display() {
  return CARD_CONTRACT_OPTION_NAME_NUMBER_DISPLAY;
}

constexpr const char *card_runtime_option_name_temperature_step() {
  return CARD_CONTRACT_OPTION_NAME_TEMPERATURE_STEP;
}

constexpr const char *card_runtime_option_name_volume_max() {
  return CARD_CONTRACT_OPTION_NAME_VOLUME_MAX;
}

constexpr const char *card_runtime_option_name_playlist_content_id() {
  return CARD_CONTRACT_OPTION_NAME_PLAYLIST_CONTENT_ID;
}

constexpr const char *card_runtime_option_name_playlist_content_type() {
  return CARD_CONTRACT_OPTION_NAME_PLAYLIST_CONTENT_TYPE;
}

constexpr const char *card_runtime_option_name_playlist_player_source() {
  return CARD_CONTRACT_OPTION_NAME_PLAYLIST_PLAYER_SOURCE;
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

inline bool card_runtime_cover_control_tab_valid(const std::string &tab) {
  return card_contract_cover_control_tab_valid(tab);
}

inline const char *card_runtime_cover_control_tabs_default() {
  return CARD_CONTRACT_COVER_CONTROL_TABS_DEFAULT;
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

inline bool card_runtime_gate_mode_valid(const std::string &mode) {
  return card_contract_gate_mode_valid(mode);
}

inline bool card_runtime_gate_command_mode(const std::string &mode) {
  return card_runtime_gate_mode_valid(mode) && !mode.empty();
}

inline std::string card_runtime_gate_label_display(const std::string &value) {
  return card_contract_gate_label_display_valid(value)
    ? value
    : CARD_CONTRACT_GATE_LABEL_DISPLAY_DEFAULT;
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

inline int card_runtime_media_volume_max_min() {
  return CARD_CONTRACT_MEDIA_VOLUME_MAX_MIN;
}

inline int card_runtime_media_volume_max_max() {
  return CARD_CONTRACT_MEDIA_VOLUME_MAX_MAX;
}

inline int card_runtime_media_volume_max_default() {
  return CARD_CONTRACT_MEDIA_VOLUME_MAX_DEFAULT;
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

inline size_t card_runtime_alarm_default_action_count() {
  return card_contract_alarm_default_action_count();
}

inline const char *card_runtime_alarm_default_action_at(size_t index) {
  return card_contract_alarm_default_action_at(index);
}

inline size_t card_runtime_alarm_max_visible_actions() {
  return CARD_CONTRACT_ALARM_MAX_VISIBLE_ACTIONS;
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

inline std::string card_runtime_image_modal_mode(const std::string &value) {
  return card_contract_image_modal_mode_valid(value)
    ? value
    : CARD_CONTRACT_IMAGE_MODAL_MODE_DEFAULT;
}

inline const char *card_runtime_image_modal_mode_default() {
  return CARD_CONTRACT_IMAGE_MODAL_MODE_DEFAULT;
}

inline bool card_runtime_light_control_tab_valid(const std::string &tab) {
  return card_contract_light_control_tab_valid(tab);
}

inline const char *card_runtime_light_control_tabs_default() {
  return CARD_CONTRACT_LIGHT_CONTROL_TABS_DEFAULT;
}

inline std::string card_runtime_climate_label_display(const std::string &value) {
  return card_contract_climate_label_display_valid(value)
    ? value
    : CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_DEFAULT;
}

inline const char *card_runtime_climate_label_display_default() {
  return CARD_CONTRACT_CLIMATE_LABEL_DISPLAY_DEFAULT;
}

inline std::string card_runtime_climate_number_display(const std::string &value) {
  return card_contract_climate_number_display_valid(value)
    ? value
    : CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_DEFAULT;
}

inline const char *card_runtime_climate_number_display_default() {
  return CARD_CONTRACT_CLIMATE_NUMBER_DISPLAY_DEFAULT;
}

inline std::string card_runtime_climate_temperature_step(const std::string &value) {
  return card_contract_climate_temperature_step_valid(value)
    ? value
    : CARD_CONTRACT_CLIMATE_TEMPERATURE_STEP_DEFAULT;
}

inline const char *card_runtime_climate_temperature_step_default() {
  return CARD_CONTRACT_CLIMATE_TEMPERATURE_STEP_DEFAULT;
}

inline bool card_runtime_climate_precision_valid(const std::string &precision) {
  return card_contract_climate_precision_valid(precision);
}

inline constexpr bool card_runtime_weather_forecast_supported() {
#if defined(ESPCONTROL_DISABLE_WEATHER_FORECAST) && ESPCONTROL_DISABLE_WEATHER_FORECAST
  return false;
#else
  return true;
#endif
}

inline bool card_runtime_weather_forecast_precision(const std::string &precision) {
  if (!card_runtime_weather_forecast_supported()) return false;
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

inline std::string card_runtime_lawn_mower_mode(const std::string &mode) {
  if (mode == "status" || mode == "start_mowing" || mode == "dock" ||
      mode == "pause_resume") {
    return mode;
  }
  return "start_mowing";
}

inline bool card_runtime_lawn_mower_state_mode(const std::string &mode) {
  std::string normalized = card_runtime_lawn_mower_mode(mode);
  return normalized == "status" || normalized == "start_mowing" ||
         normalized == "dock" || normalized == "pause_resume";
}

inline const char *card_runtime_lawn_mower_default_icon_name(const std::string &mode) {
  std::string normalized = card_runtime_lawn_mower_mode(mode);
  if (normalized == "dock") return "Robot Mower Outline";
  return "Robot Mower";
}
