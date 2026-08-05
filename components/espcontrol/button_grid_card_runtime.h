#pragma once

// Firmware-facing card metadata boundary. Card behavior stays in the existing
// card files; shared identity, defaults, families, and capability checks flow
// through this helper.

#include "button_grid_contract_generated.h"
#include "button_grid_card_registry.h"

namespace espcontrol::cards {

enum class Surface : uint8_t {
  MAIN_GRID,
  SUBPAGE,
};

struct Context {
  espcontrol::card_runtime::CardRuntimeSpec runtime;
  Family family = Family::UNKNOWN;
  Surface surface = Surface::MAIN_GRID;
  bool known = false;
  bool allow_in_subpage = false;
  bool legacy_dispatch = false;
};

inline Family family_for_runtime_type(espcontrol::card_runtime::CardTypeId type) {
  using Type = espcontrol::card_runtime::CardTypeId;
  switch (type) {
    case Type::SWITCH:
    case Type::LIGHT_SWITCH: return Family::TOGGLE;
    case Type::ACTION: return Family::ACTION;
    case Type::VACUUM: return Family::VACUUM;
    case Type::LAWN_MOWER: return Family::MOWER;
    case Type::ALARM: return Family::ALARM;
    case Type::ALARM_ACTION: return Family::ALARM_ACTION;
    case Type::CALENDAR:
    case Type::CLOCK:
    case Type::TIMEZONE: return Family::DATE_TIME;
    case Type::CLIMATE:
    case Type::CLIMATE_CONTROL: return Family::CLIMATE;
    case Type::COVER: return Family::COVER;
    case Type::DOOR_WINDOW:
    case Type::PRESENCE: return Family::OCCUPANCY;
    case Type::FAN_DIRECTION:
    case Type::FAN_OSCILLATE:
    case Type::FAN_PRESET:
    case Type::FAN_SPEED:
    case Type::FAN_CONTROL:
    case Type::FAN_SWITCH: return Family::FAN;
    case Type::GARAGE:
    case Type::GATE:
    case Type::LOCK: return Family::ACCESS;
    case Type::IMAGE: return Family::IMAGE;
    case Type::INTERNAL: return Family::INTERNAL;
    case Type::LIGHT_CONTROL: return Family::LIGHT_CONTROL;
    case Type::LIGHT_TEMPERATURE: return Family::LIGHT_TEMPERATURE;
    case Type::LOCAL_SENSOR: return Family::LOCAL_SENSOR;
    case Type::MEDIA: return Family::MEDIA;
    case Type::OPTION_SELECT: return Family::OPTION_SELECT;
    case Type::PUSH: return Family::PUSH;
    case Type::SCREEN_LOCK: return Family::SCREEN_LOCK;
    case Type::SENSOR: return Family::SENSOR;
    case Type::SLIDER:
    case Type::LIGHT_BRIGHTNESS: return Family::SLIDER;
    case Type::SUBPAGE: return Family::SUBPAGE;
    case Type::WEATHER:
    case Type::WEATHER_FORECAST: return Family::WEATHER;
    case Type::WEBHOOK: return Family::WEBHOOK;
    default: return Family::UNKNOWN;
  }
}

inline Context context_for(const std::string &type, const std::string &mode,
                           Surface surface = Surface::MAIN_GRID) {
  using namespace espcontrol::card_runtime;
  Context context;
  context.runtime = card_runtime_spec(card_type_id(type));
  context.runtime.driver = resolve_card_driver(context.runtime.type, mode);
  context.family = family_for_runtime_type(context.runtime.type);
  context.surface = surface;
  context.known = context.runtime.type != CardTypeId::UNKNOWN;
  context.allow_in_subpage = has_capability(context.runtime, CAPABILITY_SUBPAGE);
  // Todo was removed from the configurator but old saved cards remain
  // supported through one explicit compatibility driver.
  if (!context.known && type == "todo") {
    context.family = Family::TODO;
    context.known = true;
    context.allow_in_subpage = true;
    context.runtime.capabilities = static_cast<uint16_t>(
        CAPABILITY_SUBSCRIPTIONS | CAPABILITY_ACTIONS | CAPABILITY_MODAL |
        CAPABILITY_RUNTIME_ALLOCATION | CAPABILITY_SUBPAGE);
    context.legacy_dispatch = true;
  }
  return context;
}

}  // namespace espcontrol::cards

template<typename Config>
inline auto card_runtime_context(
    const Config &config,
    espcontrol::cards::Surface surface = espcontrol::cards::Surface::MAIN_GRID)
    -> decltype((void) config.type, (void) config.sensor,
                espcontrol::cards::Context()) {
  return espcontrol::cards::context_for(config.type, config.sensor, surface);
}

inline espcontrol::cards::Context card_runtime_context(
    const std::string &type,
    espcontrol::cards::Surface surface = espcontrol::cards::Surface::MAIN_GRID) {
  return espcontrol::cards::context_for(type, "", surface);
}

inline espcontrol::cards::Registration card_runtime_registration(const std::string &type) {
  const auto context = card_runtime_context(type);
  return espcontrol::cards::registration(
      context.family, context.known, context.allow_in_subpage);
}

inline espcontrol::cards::Family card_runtime_family(const std::string &type) {
  return card_runtime_context(type).family;
}

inline bool card_runtime_has_capability(
    const espcontrol::cards::Context &context,
    espcontrol::card_runtime::CardCapabilityFlag capability) {
  return espcontrol::card_runtime::has_capability(context.runtime, capability);
}

inline bool card_runtime_information_only(const espcontrol::cards::Context &context) {
  return card_runtime_has_capability(
      context, espcontrol::card_runtime::CAPABILITY_INFORMATION_ONLY);
}

inline bool card_runtime_passive(const espcontrol::cards::Context &context) {
  return card_runtime_information_only(context) &&
         !card_runtime_has_capability(
             context, espcontrol::card_runtime::CAPABILITY_ACTIONS);
}

inline const char *card_runtime_label(const std::string &type) {
  return card_contract_card_label(type);
}

inline bool card_runtime_allow_in_subpage(const std::string &type) {
  return card_runtime_context(type).allow_in_subpage;
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

constexpr const char *card_runtime_option_name_time_unit() {
  return CARD_CONTRACT_OPTION_NAME_TIME_UNIT;
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

constexpr const char *card_runtime_option_name_media_cover_art() {
  return CARD_CONTRACT_OPTION_NAME_MEDIA_COVER_ART;
}

constexpr const char *card_runtime_option_name_cover_art_action() {
  return CARD_CONTRACT_OPTION_NAME_COVER_ART_ACTION;
}

constexpr const char *card_runtime_option_name_cover_art_details() {
  return CARD_CONTRACT_OPTION_NAME_COVER_ART_DETAILS;
}

constexpr const char *card_runtime_option_name_cover_art_secondary_entity() {
  return CARD_CONTRACT_OPTION_NAME_COVER_ART_SECONDARY_ENTITY;
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

constexpr const char *card_runtime_option_name_speaker_group_entity() {
  return CARD_CONTRACT_OPTION_NAME_SPEAKER_GROUP_ENTITY;
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
