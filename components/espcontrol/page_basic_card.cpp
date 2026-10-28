#include "page_basic_card.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "bounded_sensor_text.h"
#include "button_grid_contract_generated.h"
#include "button_grid_subpage_parser.h"
#include "calendar_card_text.h"
#include "climate_state_logic.h"
#include "media_playlist_match.h"
#include "page_fan_mode.h"
#include "page_media_group.h"
#include "page_toggle_state.h"
#include "timer_state_policy.h"
#include "weather_condition.h"
#include "wifi_qr_codec.h"

namespace espcontrol::pages {
namespace {
std::string_view trim(std::string_view text) {
  while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.front())))text.remove_prefix(1);
  while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.back())))text.remove_suffix(1);
  return text;
}
bool equal_state(std::string_view text,std::string_view expected) {
  text=trim(text.substr(0,HA_SHORT_STATE_MAX_LEN));
  if(text.size()!=expected.size())return false;
  for(size_t i=0;i<text.size();++i)if(std::tolower(static_cast<unsigned char>(text[i]))!=expected[i])return false;
  return true;
}
bool on(std::string_view text) {
  return page_toggle_on(text);
}
bool available(const PageEntityValue &value) {
  return value.status==PageEntityStatus::AVAILABLE && !equal_state(value.value,"unknown") &&
         !equal_state(value.value,"unavailable");
}
PageEntityValue find(PageEntityStateRuntime &state,ConfigurationHandle handle,
                     std::string_view entity,std::string_view attribute={}) {
  for(size_t ordinal=0;ordinal<state.dependency_count(handle);++ordinal) {
    auto value=state.dependency(handle,ordinal);
    if(value.entity==entity && value.attribute==attribute)return value;
  }
  return {};
}
}
bool basic_card_is_forecast(const PageStore &store,CardId id) {
  const auto *card=store.card(id);if(!card)return false;
  const auto type=store.field(*card,CardField::TYPE);
  return card_runtime_weather_forecast_supported()&&(type=="weather_forecast"||
    (type=="weather"&&card_runtime_weather_forecast_precision(std::string(store.field(*card,CardField::PRECISION)))));
}
bool prepare_basic_card(const PageStore& store, CardId id, BasicCardDefinition& out,
                        BasicCardTextSink* strings, PageLocalRelayRuntime* relay) {
  out={};const auto *card=store.card(id);if(card==nullptr)return false;
  out.entity=store.field(*card,CardField::ENTITY);out.sensor=store.field(*card,CardField::SENSOR);
  out.label=store.field(*card,CardField::LABEL);out.unit=trim(store.field(*card,CardField::UNIT));
  out.icon=store.field(*card,CardField::ICON);out.icon_on=store.field(*card,CardField::ICON_ON);
  const auto type=store.field(*card,CardField::TYPE),precision=store.field(*card,CardField::PRECISION);
  const std::string options(store.field(*card,CardField::OPTIONS));
  const auto copy=[strings](const std::string &value,std::string_view &stored) {
    if(value.empty()){stored={};return true;}
    return strings&&strings->copy(value,stored);
  };
  if (const auto slider = PagePercentSliderRuntime::classify(store, id);
      slider != PagePercentSliderKind::NONE) {
    out.kind = BasicCardKind::PERCENT_SLIDER;
    out.percent_slider = slider;
    out.unit = {};
    out.sensor = {};
    if (type == "cover" && (out.icon.empty() || out.icon == "Auto")) out.icon = "Blinds";
    if (out.icon_on.empty() || out.icon_on == "Auto")
      out.icon_on = type == "cover" ? "Blinds Open" : out.icon;
  } else if (PageNumberSliderRuntime::supported(store, id)) {
    out.kind = BasicCardKind::NUMBER_SLIDER;
    out.unit = {};
    out.sensor = {};
    if (type == "cover" && (out.icon.empty() || out.icon == "Auto")) out.icon = "Blinds";
    out.icon_on = out.icon;
  } else if (type == "light_temperature") {
    out.kind = BasicCardKind::LIGHT_TEMPERATURE;
    out.unit = {};
    out.sensor = {};
    if (out.icon.empty() || out.icon == "Auto") out.icon = "Lightbulb";
    out.icon_on = out.icon;
  } else if (type == "wifi_qr" || type == "wifi_qr_card") {
    out.kind = BasicCardKind::WIFI_QR;
    out.qr_tile = type == "wifi_qr_card";
    out.unit = out.sensor = {};
    out.push_feedback = true;
    if (out.label.empty() || out.label == "Connect") {
      out.label = {};
      out.default_label = "Connect";
    }
    if (out.icon.empty() || out.icon == "Auto") out.icon = "Wifi";
    out.icon_on = out.icon;
    std::string payload, ssid;
    if (wifi_qr_build_payload(cfg_option_value(options, "ssid64"), cfg_option_value(options, "security"),
                              cfg_option_value(options, "pass64"),
                              cfg_option_token_present(options, "hidden"), &payload, &ssid) &&
        !copy(payload, out.time_unit))
      return false;
  } else if (type == "screen_lock") {
    out.kind = BasicCardKind::SCREEN_LOCK;
    out.entity = out.sensor = out.label = out.unit = {};
    out.icon = "Lock Open";
    out.icon_on = "Lock";
    out.default_label = "Screen Unlocked";
    out.active_color = out.push_feedback = true;
    out.screensaver_on_lock = cfg_option_enabled(options, "screensaver_on_lock");
  } else if (type == "image") {
    out.kind = BasicCardKind::IMAGE;
    out.unit = {};
    out.sensor = {};
    out.image = {};
    out.image.refresh =
        static_cast<uint8_t>(camera::refresh_mode(cfg_option_value(options, "image_modal_refresh_mode")));
    out.image.interval_seconds =
        camera::refresh_interval_ms(cfg_option_value(options, "image_modal_refresh_interval")) / 1000;
    out.image.fit = normalize_image_modal_mode(cfg_option_value(options, IMAGE_MODAL_MODE_OPTION)) == "fit";
    out.image.label = cfg_option_token_present(options, IMAGE_LABEL_OPTION);
    out.image.icon = cfg_option_token_present(options, IMAGE_ICON_OPTION);
    const auto trigger = cfg_option_value(options, "image_modal_refresh_trigger");
    if (out.image.refresh == static_cast<uint8_t>(camera::RefreshMode::ACTIVITY) &&
        camera::valid_trigger(trigger)) {
      if (!copy(trigger, out.time_unit)) return false;
    } else
      out.time_unit = {};
    if (out.icon.empty() || out.icon == "Auto") out.icon = "Camera";
    out.icon_on = out.icon;
  } else if (type == "timer") {
    out.kind = BasicCardKind::TIMER;
    out.unit = {};
    out.sensor = {};
    out.active_color = true;
    out.icon = out.icon_on = "";
    out.default_label = "Timer";
  } else if (type == "internal") {
    out.kind = BasicCardKind::LOCAL_RELAY;
    out.unit = {};
    out.sensor = {};
    out.push_feedback = card_runtime_internal_push_mode(std::string(store.field(*card, CardField::SENSOR)));
    out.active_color = !out.push_feedback;
    out.default_label = "Relay";
    if (out.icon.empty() || out.icon == "Auto") out.icon = out.push_feedback ? "Gesture Tap" : "Power Plug";
    if (out.icon_on.empty() || out.icon_on == "Auto" || out.push_feedback) out.icon_on = out.icon;
    if (out.label.empty() && !out.entity.empty()) {
      const auto registered = relay ? relay->registered_label(out.entity) : std::string_view{};
      if (!registered.empty()) {
        if (!strings || !strings->copy(registered, out.label)) return false;
      } else if (!copy(sentence_cap_text(std::string(out.entity)), out.label))
        return false;
    }
  } else if (type == "option_select" ||
             (type == "action" && action_card_option_select_action(std::string(out.sensor)))) {
    out.kind = BasicCardKind::OPTION_SELECT;
    out.push_feedback = true;
    out.sensor = {};
    out.unit = {};
    out.default_label = "Option";
    // The text value replaces the icon for both the dedicated and Action form.
    out.icon = out.icon_on = "";
  } else if (type.empty() || type == "light_switch") {
    out.text_overlay=precision=="text";
    out.kind=out.entity.empty()?BasicCardKind::EMPTY:BasicCardKind::TOGGLE;
  } else if (type == "sensor") {
    if (out.sensor == "local") {
      out.kind = out.entity.empty()    ? BasicCardKind::EMPTY
                 : precision == "text" ? BasicCardKind::LOCAL_TEXT
                                       : BasicCardKind::LOCAL_NUMBER;
      if (out.kind == BasicCardKind::LOCAL_TEXT) {
        out.label = {};
        out.unit = {};
      }
    } else {
      out.kind = out.sensor.empty()    ? BasicCardKind::EMPTY
                 : precision == "icon" ? BasicCardKind::ICON
                 : precision == "text" ? BasicCardKind::TEXT
                 : precision == "time" ? BasicCardKind::DURATION
                                       : BasicCardKind::NUMBER;
      out.active_color = out.kind != BasicCardKind::DURATION && cfg_option_enabled(options, "active_color");
    }
  } else if (type == "door_window" || type == "presence") {
    out.kind=out.sensor.empty()?BasicCardKind::EMPTY:BasicCardKind::STATUS;
    out.presence=type=="presence";out.active_color=cfg_option_enabled(options,"active_color");
    const bool window=normalize_door_window_subtype(std::string(precision))=="window";
    if(out.kind==BasicCardKind::STATUS) {
      out.default_label=out.presence?"Presence":window?"Window":"Door";
      if(out.icon.empty()||out.icon=="Auto")out.icon=out.presence?"Motion Sensor Off":window?"Window Closed":"Door";
      if(out.icon_on.empty()||out.icon_on=="Auto")out.icon_on=out.presence?"Motion Sensor":window?"Window Open":"Door Open";
    }
  } else if (type == "action" || type == "local" || type == "push" || type == "webhook" ||
             type == "screensaver") {
    ParsedCfg config;config.type=type;config.entity=out.entity;config.sensor=out.sensor;config.options=options;
    if (action_card_option_select(config)) return false;
    out.kind=BasicCardKind::ACTION;
    const bool local=type=="local"||action_card_local_action(config);
    out.ha_input = !local && type != "webhook" && type != "screensaver";
    out.push_feedback=type=="action"||local||type=="screensaver"||
      (type=="push"&&out.entity.empty()&&(out.icon.empty()||out.icon=="Auto"));
    if(type=="action"||local) {
      if(out.icon.empty()||out.icon=="Auto")out.icon=local?"Gesture Tap":"Flash";
      if(out.label.empty()) {
        if(out.entity.empty())out.default_label=local?"Local Action":"Action";
        else if(local) {if(!copy(sentence_cap_text(std::string(out.entity)),out.label))return false;}
        else out.label=out.entity;
      }
    } else if(type=="screensaver") {
      out.label={};out.default_label="Screensaver";out.icon="Monitor";
    } else if(type=="push"&&out.entity.empty()) {
      if(out.label.empty())out.default_label="Push";
      if(out.icon.empty()||out.icon=="Auto")out.icon="Gesture Tap";
    }
    const auto state=action_card_state_entity(config);
    out.active_color=!state.empty();
    // ACTION declares exactly one primary state dependency. Read that channel
    // by ordinal instead of retaining another decoded entity-name copy here.
    out.sensor={};
    if(action_card_state_display_enabled(config)) {
      const auto format=action_card_state_precision(config);
      out.action_display=format=="icon"?BasicActionDisplay::ICON:format=="text"?BasicActionDisplay::TEXT:BasicActionDisplay::NUMBER;
      out.precision=static_cast<uint8_t>(parse_precision(format));
      const auto unit=trim_display_unit(action_card_state_unit(config));
      if(unit.empty())out.unit={};
      else if(!strings||!strings->copy_unit(unit,out.unit))return false;
    }
  } else if (type == "media") {
    const auto mode=card_runtime_media_mode(std::string(out.sensor));
    if (mode != "play_pause" && mode != "previous" && mode != "next" && mode != "playlist" &&
        mode != "position" && mode != "now_playing" && mode != "cover_art" && mode != "volume" &&
        mode != "control_modal" && mode != "speaker_group")
      return false;
    const bool control = mode == "volume" || mode == "control_modal" || mode == "speaker_group";
    const bool cover = mode == "cover_art" ||
                       (mode == "now_playing" && cfg_option_token_present(options, MEDIA_COVER_ART_OPTION));
    out.kind = control                 ? BasicCardKind::MEDIA_CONTROL
               : cover                 ? BasicCardKind::MEDIA_COVER_ART
               : mode == "now_playing" ? BasicCardKind::MEDIA_NOW_PLAYING
               : mode == "position"    ? BasicCardKind::MEDIA_POSITION
                                       : BasicCardKind::MEDIA_ACTION;
    out.push_feedback = cover                   ? false
                        : mode == "now_playing" ? precision == "play_pause"
                                                : mode != "position";
    out.unit = {};
    out.media.kind = mode == "volume"          ? MediaActionKind::VOLUME
                     : mode == "control_modal" ? MediaActionKind::CONTROL_MODAL
                     : mode == "speaker_group" ? MediaActionKind::SPEAKER_GROUP
                     : cover                   ? MediaActionKind::COVER_ART
                     : mode == "previous"      ? MediaActionKind::PREVIOUS
                     : mode == "next"          ? MediaActionKind::NEXT
                     : mode == "playlist"      ? MediaActionKind::PLAYLIST
                     : mode == "position"      ? MediaActionKind::POSITION
                     : mode == "now_playing"   ? MediaActionKind::NOW_PLAYING
                                               : MediaActionKind::PLAY_PAUSE;
    out.media.show_state=(mode=="play_pause"||mode=="position")&&precision=="state";
    out.media.track_details = cover && cfg_option_token_present(options, MEDIA_COVER_ART_DETAILS_OPTION);
    out.media.progress = !cover && mode == "now_playing" && precision == "progress";
    out.media.play_pause_background = !cover && mode == "now_playing" && precision == "play_pause";
    out.active_color = mode == "control_modal" || mode == "speaker_group" || mode == "play_pause" ||
                       mode == "playlist" || out.media.play_pause_background;
    out.default_label=mode=="previous"?"Previous":mode=="next"?"Next":mode=="playlist"?"Playlist":mode=="position"?"Position":"Play/Pause";
    if (cover) out.icon = "Music";
    if (out.icon.empty() || out.icon == "Auto")
      out.icon = mode == "previous"        ? "Skip Previous"
                 : mode == "next"          ? "Skip Next"
                 : mode == "playlist"      ? "Music"
                 : mode == "speaker_group" ? "Speaker Multiple"
                                           : "Play Pause";
    if (control) {
      out.push_feedback = true;
      out.media.show_state =
          mode == "control_modal" && cfg_option_value(options, LABEL_DISPLAY_OPTION) != "label";
      out.media.volume_number =
          mode == "volume" ||
          (mode == "control_modal" && cfg_option_value(options, NUMBER_DISPLAY_OPTION) == "volume");
      out.media.volume_max = static_cast<uint8_t>(
          normalize_media_volume_max_percent(cfg_option_value(options, VOLUME_MAX_OPTION)));
      out.default_label = mode == "volume"          ? "Volume"
                          : mode == "speaker_group" ? "Speaker Group"
                                                    : "Media Control";
    }
    // Existing media action buttons keep one configured icon in both states.
    out.icon_on=out.icon;
    if(mode=="playlist") {
      if(!copy(cfg_option_value(options,MEDIA_PLAYLIST_CONTENT_ID_OPTION),out.media.playlist_id))return false;
      auto content_type=cfg_option_value(options,MEDIA_PLAYLIST_CONTENT_TYPE_OPTION);
      if(content_type.empty())out.media.playlist_type="playlist";
      else if(!copy(content_type,out.media.playlist_type))return false;
    }
  } else if (type == "weather" || type == "weather_forecast") {
    out.kind=basic_card_is_forecast(store,id)?BasicCardKind::FORECAST:BasicCardKind::WEATHER;
    out.forecast_view=precision=="today"?weather::View::TODAY:precision=="tomorrow"||type=="weather_forecast"?weather::View::TOMORROW:weather::View::DAILY;out.label={};out.sensor={};out.unit={};
    out.icon=out.icon_on="Weather Cloudy";
  } else if (type == "alarm" || type == "alarm_action") {
    out.kind = BasicCardKind::ALARM;
    const bool panel = type == "alarm";
    const std::string inherited(PageAlarmStateRuntime::effective_options(store, id));
    auto mode = alarm::action(out.sensor);
    if (!panel && mode == alarm::Action::NONE) mode = alarm::Action::AWAY;
    out.alarm = {
        panel ? alarm::Action::NONE : mode,
        panel,
        panel && normalize_alarm_icon_display(cfg_option_value(inherited, "icon_display")) == "status",
        panel && normalize_alarm_label_display(cfg_option_value(inherited, "label_display")) == "status",
        cfg_option_value(inherited, "pin_arm") != "0",
        cfg_option_value(inherited, "pin_disarm") != "0",
        panel && cfg_option_value(inherited, "card_off_color").empty()};
    out.entity = PageAlarmStateRuntime::effective_entity(store, id);
    out.default_label = panel ? "Alarm" : alarm_card_action_label(mode);
    if (out.icon.empty() || out.icon == "Auto")
      out.icon = panel ? "Security" : alarm_card_glyph_name(alarm_card_glyph(alarm::achieved(mode), {}));
    out.icon_on = out.icon;
    out.unit = {};
    out.sensor = {};
    out.active_color = true;
  } else if (type == "clock" || type == "timezone") {
    out.kind = BasicCardKind::CLOCK;
    out.clock = {type == "timezone", cfg_option_token_present(options, "large_numbers"),
                 cfg_option_token_present(options, "center_clock")};
    out.sensor = {};
    out.unit = {};
    out.icon = out.icon_on = "";
    if (!out.clock.show_label) out.label = {};
  } else if (type == "calendar") {
    out.kind = BasicCardKind::CALENDAR;
    out.calendar_time = precision == "datetime";
    out.sensor = {};
    out.label = {};
    out.unit = {};
    out.icon = out.icon_on = "";
    out.default_label = "Date";
  } else if (type == "vacuum" || type == "lawn_mower") {
    const bool mower = type == "lawn_mower";
    out.kind = BasicCardKind::CLEANING;
    out.cleaning = {cleaning::mode(out.sensor, mower), mower, cleaning::automatic_icon(out.icon, mower)};
    out.default_label = cleaning::default_label(out.cleaning);
    if (out.icon.empty() || out.icon == "Auto") out.icon = cleaning::default_icon(out.cleaning);
    out.icon_on = out.icon;
    out.sensor = out.cleaning.mode == cleaning::Mode::CLEAN_AREA ? store.field(*card, CardField::UNIT)
                                                                 : std::string_view{};
    out.unit = {};
    out.push_feedback = out.cleaning.mode != cleaning::Mode::STATUS;
    out.active_color = !mower && out.cleaning.mode == cleaning::Mode::START_STOP;
  } else if (type == "garage" || type == "gate" || type == "lock") {
    const std::string mode(out.sensor);
    const bool lock = type == "lock", gate = type == "gate";
    const bool command = lock   ? card_runtime_lock_command_mode(mode)
                         : gate ? card_runtime_gate_command_mode(mode)
                                : card_runtime_garage_command_mode(mode);
    const auto label_display = cfg_option_value(options, "label_display");
    const bool status = !lock && (gate ? normalize_gate_label_display(label_display)
                                       : normalize_garage_label_display(label_display)) == "status";
    out.kind = BasicCardKind::ACCESS;
    out.access = {lock   ? BasicAccessKind::LOCK
                  : gate ? BasicAccessKind::GATE
                         : BasicAccessKind::GARAGE,
                  command, status};
    out.unit = {};
    out.push_feedback = command;
    out.active_color = !command || status;
    out.default_label = command ? (out.sensor == "open"     ? "Open"
                                   : out.sensor == "close"  ? "Close"
                                   : out.sensor == "stop"   ? "Stop"
                                   : out.sensor == "unlock" ? "Unlock"
                                                            : "Lock")
                        : lock  ? "Lock"
                        : gate  ? "Gate"
                                : "Garage Door";
    if (out.icon.empty() || out.icon == "Auto") out.icon = lock ? "Lock" : gate ? "Gate" : "Garage";
    if (out.icon_on.empty() || out.icon_on == "Auto")
      out.icon_on = lock ? "Lock Open" : gate ? "Gate Open" : "Garage Open";
    if (command && !status) {
      const auto configured = store.field(*card, CardField::ICON);
      if (configured.empty() || configured == "Auto")
        out.icon = out.sensor == "open" || out.sensor == "unlock" ? (lock   ? "Lock Open"
                                                                     : gate ? "Gate Open"
                                                                            : "Garage Open")
                   : out.sensor == "stop"                         ? "Stop"
                                                                  : out.icon;
      out.icon_on = out.icon;
    }
  } else if (type == "light_control") {
    out.unit={};out.kind=BasicCardKind::LIGHT;out.active_color=true;out.push_feedback=true;out.default_label="Light";
    if(out.icon.empty()||out.icon=="Auto")out.icon="Lightbulb Outline";
    if(out.icon_on.empty()||out.icon_on=="Auto")out.icon_on="Lightbulb";
  } else if (type == "cover") {
    if (out.sensor == "toggle") {
      out.kind = BasicCardKind::COVER_TOGGLE;
      out.unit = {};
      out.active_color = true;
      out.default_label = "Cover";
      if (out.icon.empty() || out.icon == "Auto") out.icon = "Blinds";
      if (out.icon_on.empty() || out.icon_on == "Auto") out.icon_on = "Blinds Open";
    } else if (out.sensor == "open" || out.sensor == "close" || out.sensor == "stop" ||
               out.sensor == "set_position") {
      out.kind = BasicCardKind::COVER_COMMAND;
      out.unit = {};
      out.push_feedback = true;
      out.default_label = "Cover";
      if (out.icon.empty() || out.icon == "Auto") out.icon = "Blinds";
      out.icon_on = out.icon;
    } else {
      if (espcontrol::card_runtime::resolve_card_driver(espcontrol::card_runtime::CardTypeId::COVER,
                                                        std::string(out.sensor)) !=
          espcontrol::card_runtime::CardDriverId::COVER_MODAL)
        return false;
      out.unit = {};
      out.kind = BasicCardKind::COVER;
      out.push_feedback = true;
      out.default_label = "Cover";
      if (out.icon.empty() || out.icon == "Auto") out.icon = "Blinds";
      if (out.icon_on.empty() || out.icon_on == "Auto") out.icon_on = "Blinds Open";
    }
  } else if (type == "fan_switch" || type == "fan_oscillate") {
    const bool power = type == "fan_switch";
    out.kind = power ? BasicCardKind::FAN_SWITCH : BasicCardKind::FAN_OSCILLATE;
    out.active_color = true;
    out.push_feedback = !power;
    out.unit = {};
    out.default_label = power ? "Fan" : "Oscillation";
    if (out.icon.empty() || out.icon == "Auto") out.icon = fan_card_default_icon_name(std::string(type));
    if (!power)
      out.icon_on = out.icon;
    else if (out.icon_on.empty() || out.icon_on == "Auto")
      out.icon_on = "Fan";
  } else if (type == "fan_direction" || type == "fan_preset") {
    out.kind = type == "fan_direction" ? BasicCardKind::FAN_DIRECTION : BasicCardKind::FAN_PRESET;
    out.active_color = out.push_feedback = true;
    out.label = {};
    out.unit = {};
    out.default_label = type == "fan_direction" ? "Direction" : "Preset";
    if (out.icon.empty() || out.icon == "Auto") out.icon = fan_card_default_icon_name(std::string(type));
    out.icon_on = out.icon;
  } else if (type == "fan_control") {
    out.kind = BasicCardKind::FAN_CONTROL;
    out.active_color = out.push_feedback = true;
    out.unit = {};
    out.default_label = "Fan";
    if (out.icon.empty() || out.icon == "Auto") out.icon = "Fan";
    if (out.icon_on.empty() || out.icon_on == "Auto") out.icon_on = out.icon;
  } else if (type == "climate" || type == "climate_control") {
    out.kind=BasicCardKind::CLIMATE;out.active_color=true;
    const auto label=normalize_climate_label_display(cfg_option_value(options,"label_display"));
    out.climate.label=label=="status"?ClimateCardLabel::STATUS:label=="actual"?ClimateCardLabel::ACTUAL:label=="target"?ClimateCardLabel::TARGET:ClimateCardLabel::LABEL;
    const auto number=normalize_climate_number_display(cfg_option_value(options,"number_display"));
    out.climate.number=number=="actual"?ClimateCardNumber::ACTUAL:number=="icon"?ClimateCardNumber::ICON:ClimateCardNumber::TARGET;
    out.climate.half_step=normalize_climate_temperature_step(cfg_option_value(options,"temperature_step"))=="0.5";
  } else if (type == "subpage") {
    out.kind=BasicCardKind::FOLDER;
    if(out.sensor=="indicator") {
      if(out.entity.empty())out.folder_mode=BasicFolderMode::CHILDREN;
      else {
        const auto kind=normalize_subpage_kind(cfg_option_value(options,"subpage_kind"));
        out.folder_mode=kind=="climate"?BasicFolderMode::CLIMATE:kind=="lawn_mower"?BasicFolderMode::MOWER:BasicFolderMode::ENTITY;
      }
      out.active_color=true;
    } else if(!out.sensor.empty())out.folder_mode=precision=="text"?BasicFolderMode::TEXT:BasicFolderMode::NUMBER;
  } else
    return false;
  switch (out.kind) {
    case BasicCardKind::TOGGLE:
    case BasicCardKind::CLIMATE:
    case BasicCardKind::LIGHT:
    case BasicCardKind::COVER:
    case BasicCardKind::MEDIA_ACTION:
    case BasicCardKind::MEDIA_POSITION:
    case BasicCardKind::MEDIA_NOW_PLAYING:
    case BasicCardKind::MEDIA_COVER_ART:
    case BasicCardKind::MEDIA_CONTROL:
    case BasicCardKind::FAN_CONTROL:
    case BasicCardKind::FAN_DIRECTION:
    case BasicCardKind::FAN_PRESET:
    case BasicCardKind::OPTION_SELECT:
    case BasicCardKind::FAN_SWITCH:
    case BasicCardKind::FAN_OSCILLATE:
    case BasicCardKind::PERCENT_SLIDER:
    case BasicCardKind::NUMBER_SLIDER:
    case BasicCardKind::LIGHT_TEMPERATURE:
    case BasicCardKind::TIMER:
    case BasicCardKind::COVER_COMMAND:
    case BasicCardKind::COVER_TOGGLE:
    case BasicCardKind::ACCESS:
    case BasicCardKind::CLEANING:
    case BasicCardKind::ALARM:
    case BasicCardKind::IMAGE:
      out.ha_input = true;
      break;
    default:
      break;
  }
  out.confirmation = page_confirmation_policy(store, {store.generation(), id}).flags();
  if(out.kind==BasicCardKind::TEXT&&cfg_option_enabled(options,SENSOR_STATE_LABELS_OPTION)) {
    auto input=cfg_option_value(options,SENSOR_STATE_INPUT_OPTION),output=cfg_option_value(options,SENSOR_STATE_OUTPUT_OPTION);
    const auto high=cfg_option_value(options,SENSOR_STATE_HIGH_LABEL_OPTION),low=cfg_option_value(options,SENSOR_STATE_LOW_LABEL_OPTION);
    if(input.empty()&&!high.empty()){input="high";output=high;}
    else if(input.empty()&&!low.empty()){input="low";output=low;}
    if(!copy(sensor_state_translation_key(input),out.text_input)||!copy(output,out.text_output)||
       !copy(sensor_state_translation_key(cfg_option_value(options,SENSOR_STATE_INPUT_2_OPTION)),out.text_input_2)||
       !copy(cfg_option_value(options,SENSOR_STATE_OUTPUT_2_OPTION),out.text_output_2))return false;
  }
  if(out.kind==BasicCardKind::DURATION&&!copy(cfg_option_value(options,SENSOR_TIME_UNIT_OPTION),out.time_unit))return false;
  if(out.kind!=BasicCardKind::ACTION)out.precision=static_cast<uint8_t>(parse_precision(std::string(precision)));
  out.climate.precision=out.precision;
  out.large_numbers =
      (out.kind == BasicCardKind::CLOCK || out.kind == BasicCardKind::CALENDAR ||
       out.kind == BasicCardKind::NUMBER || out.kind == BasicCardKind::DURATION ||
       (out.kind == BasicCardKind::CLIMATE && out.climate.number != ClimateCardNumber::ICON)) &&
      !large_numbers_explicitly_disabled(options);
  out.explicit_large =
      out.kind == BasicCardKind::CLIMATE && cfg_option_token_present(options, "large_numbers");
  const auto mode=cfg_option_value(options,"card_active_color");out.theme_active=mode=="theme";
  out.on_pattern = normalize_card_on_pattern(cfg_option_value(options, "on_pattern")) == "stripes";
  auto color=cfg_option_value(options,"card_off_color");
  if(!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.off_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_off=true;}
  color=cfg_option_value(options,"card_on_color");
  if(mode=="custom"&&!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.on_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_on=true;}
  return true;
}
std::string_view basic_folder_child_indicator(const PageStore &store,CardId id) {
  const auto *card=store.card(id);if(!card)return {};
  const auto type=store.field(*card,CardField::TYPE);
  // The currently ported children which contributed to the old indicator.
  // Sensors never contributed. Extend this policy when other drivers port their
  // indicator semantics (mower/local controls use different predicates).
  if (PagePercentSliderRuntime::classify(store, id) != PagePercentSliderKind::NONE ||
      PageNumberSliderRuntime::supported(store,id))
    return store.field(*card, CardField::ENTITY);
  if (type == "alarm") return PageAlarmStateRuntime::effective_entity(store, id);
  if (type == "timer")
    return PageTimerRuntime::supported(store, id) ? store.field(*card, CardField::ENTITY)
                                                  : std::string_view{};
  if (type == "garage" || type == "gate" || type == "lock")
    return PageAccessStateRuntime::classify(store, id) != PageAccessHistory::NONE
               ? store.field(*card, CardField::ENTITY)
               : std::string_view{};
  if (type == "cover") {
    const auto mode = store.field(*card, CardField::SENSOR);
    // The empty/tilt variants were classified above. Commands never
    // contribute. Toggle and other modes use the ordinary entity predicate.
    if (mode.empty() || mode == "tilt" || mode == "open" || mode == "close" || mode == "stop" ||
        mode == "set_position")
      return {};
    return store.field(*card, CardField::ENTITY);
  }
  if (type == "internal")
    return PageLocalRelayRuntime::toggle(store, *card) ? store.field(*card, CardField::ENTITY)
                                                       : std::string_view{};
  return type.empty() || type == "light_switch" || type == "light_temperature" || type == "light_control" ||
                 type == "vacuum" || type == "lawn_mower" || type == "media" || type == "fan_control" ||
                 type == "fan_direction" || type == "fan_preset" || type == "fan_switch" ||
                 type == "fan_oscillate"
             ? store.field(*card, CardField::ENTITY)
             : std::string_view{};
}
bool read_basic_indicator(ConfigurationHandle handle,std::string_view entity,PageEntityStateRuntime &state,BasicFolderMode mode) {
  const auto primary=find(state,handle,entity);if(!available(primary))return false;
  if (mode == BasicFolderMode::TIMER) return primary.value == "active";
  if(mode==BasicFolderMode::MOWER)return equal_state(primary.value,"mowing")||equal_state(primary.value,"returning");
  if(mode==BasicFolderMode::CLIMATE) {
    const auto action=find(state,handle,entity,"hvac_action");if(action.status!=PageEntityStatus::AVAILABLE)return false;
    char mode_text[33],action_text[33];
    const auto normalized=[](std::string_view value,char *output) {
      value=trim(value.substr(0,HA_SHORT_STATE_MAX_LEN));
      for(size_t i=0;i<value.size();++i)output[i]=static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
      return std::string_view(output,value.size());
    };
    const auto mode=normalized(primary.value,mode_text),activity=normalized(action.value,action_text);
    return climate::parent_indicator_active(!mode.empty(),mode,activity);
  }
  return on(primary.value);
}
PagePercentSliderSnapshot read_basic_percent_slider(const BasicCardDefinition& card,
                                                    ConfigurationHandle handle, PageEntityStateRuntime& state,
                                                    PagePercentSliderRuntime* runtime) {
  if (!runtime || card.kind != BasicCardKind::PERCENT_SLIDER) return {};
  auto snapshot = runtime->read(handle);
  if (snapshot.kind != card.percent_slider) return {};
  const auto primary = find(state, handle, card.entity);
  const auto level =
      find(state, handle, card.entity, PagePercentSliderRuntime::attribute(card.percent_slider));
  if (primary.status == PageEntityStatus::EXHAUSTED || level.status == PageEntityStatus::EXHAUSTED) {
    snapshot.available = false;
    snapshot.percent = 0;
    snapshot.icon_on = false;
    snapshot.preview = false;
    snapshot.fill_known = false;
  }
  return snapshot;
}
PageNumberSliderSnapshot read_basic_number_slider(const BasicCardDefinition& card, ConfigurationHandle handle,
                                                  PageEntityStateRuntime& state,
                                                  PageNumberSliderRuntime* runtime) {
  if (!runtime || card.kind != BasicCardKind::NUMBER_SLIDER) return {};
  auto snapshot = runtime->read(handle);
  for (const auto attribute : {"", "min", "max", "step"}) {
    if (find(state, handle, card.entity, attribute).status == PageEntityStatus::EXHAUSTED) {
      snapshot.available = snapshot.ready = snapshot.dragging = snapshot.gesture = false;
      break;
    }
  }
  return snapshot;
}
PageLightTemperatureSnapshot read_basic_light_temperature(const BasicCardDefinition& card,
                                                          ConfigurationHandle handle,
                                                          PageEntityStateRuntime& state,
                                                          PageLightTemperatureRuntime* runtime) {
  if (!runtime || card.kind != BasicCardKind::LIGHT_TEMPERATURE) return {};
  auto snapshot = runtime->read(handle);
  for (const auto attribute : {"", "color_temp_kelvin"})
    if (find(state, handle, card.entity, attribute).status == PageEntityStatus::EXHAUSTED)
      snapshot.available = snapshot.dragging = snapshot.gesture = false;
  return snapshot;
}
PageTimerSnapshot read_basic_timer(const BasicCardDefinition& card, ConfigurationHandle handle,
                                   PageEntityStateRuntime& state, PageTimerRuntime* runtime) {
  if (!runtime || card.kind != BasicCardKind::TIMER) return {};
  auto snapshot = runtime->read(handle);
  for (const auto attribute : {"", "duration", "remaining", "finishes_at"})
    if (find(state, handle, card.entity, attribute).status == PageEntityStatus::EXHAUSTED)
      snapshot.mode = PageTimerMode::UNAVAILABLE;
  return snapshot;
}
PageCoverCommandSnapshot read_basic_cover_command(const BasicCardDefinition& card, ConfigurationHandle handle,
                                                  PageEntityStateRuntime& state) {
  if (card.kind != BasicCardKind::COVER_COMMAND || card.entity.empty()) return {};
  const auto value = find(state, handle, card.entity, "supported_features");
  PageCoverCommandSnapshot result;
  if (value.status == PageEntityStatus::EXHAUSTED) return result;
  result.known = value.status == PageEntityStatus::AVAILABLE &&
                 cover_command::parse_features(value.value, result.features);
  result.supported = cover_command::supported(card.sensor, result.known, result.features);
  result.tilt = cover_command::use_tilt(card.sensor, result.known, result.features);
  return result;
}
PageCleaningSnapshot read_basic_cleaning(const BasicCardDefinition& card, ConfigurationHandle handle,
                                         PageEntityStateRuntime& state) {
  PageCleaningSnapshot result;
  if (card.kind != BasicCardKind::CLEANING || card.entity.empty()) return result;
  std::string_view raw;
  bool exhausted = false;
  if (cleaning::needs_state(card.cleaning)) {
    const auto primary = find(state, handle, card.entity);
    if (primary.entity != card.entity) return result;
    exhausted = primary.status == PageEntityStatus::EXHAUSTED;
    result.state_seen = primary.status != PageEntityStatus::UNKNOWN || primary.revision != 0;
    if (primary.status == PageEntityStatus::AVAILABLE)
      raw =
          cleaning::unavailable(card.entity, primary.value) ? std::string_view("unavailable") : primary.value;
    else if (result.state_seen)
      raw = "unavailable";
    result.glyph = cleaning::state_glyph(raw, card.cleaning.mower);
    result.on = !card.cleaning.mower && card.cleaning.mode == cleaning::Mode::START_STOP && raw == "cleaning";
    if (card.cleaning.mode == cleaning::Mode::STATUS && result.state_seen)
      result.label_key = cleaning::state_label(raw, card.cleaning.mower);
  }
  if (!exhausted && (card.cleaning.mode != cleaning::Mode::CLEAN_AREA || !card.sensor.empty()))
    result.service = cleaning::service(card.cleaning, raw);
  return result;
}
PageAccessCardSnapshot read_basic_access(const BasicCardDefinition& card, ConfigurationHandle handle,
                                         PageEntityStateRuntime& state, PageAccessStateRuntime* access) {
  PageAccessCardSnapshot result;
  if (card.kind != BasicCardKind::ACCESS || !access || !access->owns(handle) || card.entity.empty())
    return result;
  result.history = access->read(handle);
  const auto primary = find(state, handle, card.entity);
  if (primary.status == PageEntityStatus::EXHAUSTED) return result;
  result.enabled = card.access.kind != BasicAccessKind::GATE ||
                   (primary.status == PageEntityStatus::UNKNOWN && primary.revision == 0) ||
                   (primary.status == PageEntityStatus::AVAILABLE && !equal_state(primary.value, "unknown") &&
                    !equal_state(primary.value, "unavailable") &&
                    !trim(primary.value.substr(0, HA_SHORT_STATE_MAX_LEN)).empty());
  return result;
}
PageAlarmCardSnapshot read_basic_alarm(const BasicCardDefinition& card, ConfigurationHandle handle,
                                       PageEntityStateRuntime& cache, PageAlarmStateRuntime* owner) {
  PageAlarmCardSnapshot result;
  if (!owner || card.kind != BasicCardKind::ALARM) return result;
  owner->loop();
  result.history = owner->read(handle);
  if (find(cache, handle, card.entity).status == PageEntityStatus::EXHAUSTED)
    result.history.command_available = false;
  result.enabled = result.history.valid && (card.alarm.panel || result.history.command_available);
  result.dynamic_icon = card.alarm.panel && card.alarm.status_icon && result.history.icon_reported;
  result.glyph = alarm_card_glyph(result.history.state, result.history.arm_mode);
  return result;
}
BasicCardState read_basic_card(
    const BasicCardDefinition& card, ConfigurationHandle handle, PageEntityStateRuntime& state,
    int duration_components, std::string_view temperature_unit, PageCardTranslate translate,
    PageMediaStateRuntime* media_owner, PageLocalSensorRuntime* local, PageLocalRelayRuntime* relay,
    PageFanStatusRuntime* fan_status, PagePercentSliderRuntime* percent_slider,
    PageNumberSliderRuntime* number_slider, PageLightTemperatureRuntime* temperature, PageTimerRuntime* timer,
    PageCoverToggleRuntime* cover_toggle, PageAccessStateRuntime* access, PageCalendarRuntime* calendar,
    PageTimezoneRuntime* timezone, PageAlarmStateRuntime* alarm_owner, bool screen_locked) {
  BasicCardState result;
  if(card.kind==BasicCardKind::FORECAST)return result;
  if (card.kind == BasicCardKind::SCREEN_LOCK) {
    result.on = screen_locked;
    const auto* key = screen_locked ? "Screen Locked" : "Screen Unlocked";
    const auto* caption = translate ? translate(key) : nullptr;
    result.label = caption ? caption : key;
    return result;
  }

  if (card.kind == BasicCardKind::WIFI_QR) {
    result.label = card.label;
    if (result.label.empty()) {
      const auto* caption = translate ? translate("Connect") : nullptr;
      result.label = caption ? caption : "Connect";
    }
    return result;
  }
  if (card.kind == BasicCardKind::IMAGE) {
    result.label = card.label;
    if (result.label.empty()) {
      result.label = card.entity;
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE) {
        const auto raw = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
        result.label = raw.substr(0, raw.find('\0'));
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::PERCENT_SLIDER) {
    result.percent_slider = read_basic_percent_slider(card, handle, state, percent_slider);
    const bool inverted = card.percent_slider == PagePercentSliderKind::COVER_POSITION ||
                          card.percent_slider == PagePercentSliderKind::COVER_TILT;
    result.fill_percent = !result.percent_slider.fill_known ? 0
                          : inverted                        ? 100 - result.percent_slider.percent
                                                            : result.percent_slider.percent;
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
    }
    return result;
  }
  if (card.kind == BasicCardKind::ALARM) {
    result.alarm = read_basic_alarm(card, handle, state, alarm_owner);
    const auto& history = result.alarm.history;
    result.on = card.alarm.panel
                    ? history.active
                    : history.available && alarm::matches(card.alarm.mode, history.state, history.arm_mode);
    const auto* translated = translate && card.default_label ? translate(card.default_label) : nullptr;
    result.label = card.label.empty() ? std::string_view(translated           ? translated
                                                         : card.default_label ? card.default_label
                                                                              : "Alarm")
                                      : card.label;
    if (history.valid && card.alarm.panel) {
      if (card.alarm.status_label)
        result.label =
            history.status_known ? alarm_card_visible_caption(history.status) : std::string_view("--");
      else if (history.status_showing)
        result.label = alarm_card_visible_caption(history.status);
      else if (card.label.empty()) {
        const auto name = find(state, handle, card.entity, "friendly_name");
        if (name.status == PageEntityStatus::AVAILABLE)
          result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::CLOCK) {
    const auto text = format_clock_card(timezone ? timezone->read(handle) : PageTimezoneSnapshot{},
                                        card.clock.show_label, card.label, translate);
    result.numeric = true;
    std::memcpy(result.number, text.value, sizeof(text.value));
    result.label = text.label;
    result.clock_city_label = text.city;
    return result;
  }
  if (card.kind == BasicCardKind::CALENDAR) {
    const auto text = format_calendar_card(calendar ? calendar->read(handle) : PageCalendarState{},
                                           card.calendar_time, translate);
    result.numeric = true;
    std::memcpy(result.number, text.value, sizeof(text.value));
    if (text.static_label)
      result.label = text.static_label;
    else {
      std::memcpy(result.text, text.label, sizeof(text.label));
      result.formatted_label = true;
    }
    return result;
  }
  if (card.kind == BasicCardKind::CLEANING) {
    result.cleaning = read_basic_cleaning(card, handle, state);
    result.on = result.cleaning.on;
    if (result.cleaning.label_key) {
      const auto* caption = translate ? translate(result.cleaning.label_key) : nullptr;
      result.label = caption ? caption : result.cleaning.label_key;
    } else if (!card.label.empty())
      result.label = card.label;
    else {
      const auto* caption = translate ? translate(card.default_label) : nullptr;
      result.label = caption ? caption : card.default_label;
    }
    return result;
  }
  if (card.kind == BasicCardKind::ACCESS) {
    result.access = read_basic_access(card, handle, state, access);
    const auto& history = result.access.history;
    result.on = history.on;
    result.cover_icon_open = history.icon_on;
    if (find(state, handle, card.entity).status == PageEntityStatus::EXHAUSTED) {
      result.on = result.cover_icon_open = false;
      const auto* caption = translate ? translate("Unavailable") : nullptr;
      result.label = caption ? caption : "Unavailable";
    } else if (history.showing)
      result.label = history.status;
    else if (card.access.status)
      result.label = "--";
    else if (!card.label.empty())
      result.label = card.label;
    else if (!card.access.command &&
             find(state, handle, card.entity, "friendly_name").status == PageEntityStatus::AVAILABLE)
      result.label =
          find(state, handle, card.entity, "friendly_name").value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
    else {
      const auto* caption = translate ? translate(card.default_label) : nullptr;
      result.label = caption ? caption : card.default_label;
    }
    return result;
  }
  if (card.kind == BasicCardKind::COVER_TOGGLE) {
    const auto snapshot = cover_toggle ? cover_toggle->read(handle) : PageCoverToggleSnapshot{};
    result.on = snapshot.on;
    result.cover_icon_open = snapshot.icon_open;
    if (find(state, handle, card.entity).status == PageEntityStatus::EXHAUSTED) {
      result.on = result.cover_icon_open = false;
      const auto* caption = translate ? translate("Unavailable") : nullptr;
      result.label = caption ? caption : "Unavailable";
    } else if (snapshot.showing)
      result.label = snapshot.status;
    else if (!card.label.empty())
      result.label = card.label;
    else {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
      else {
        const auto* caption = translate ? translate("Cover") : nullptr;
        result.label = caption ? caption : "Cover";
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::COVER_COMMAND) {
    result.cover_command = read_basic_cover_command(card, handle, state);
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE && !name.value.empty())
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
      else {
        const auto* text = translate ? translate("Cover") : nullptr;
        result.label = text ? text : "Cover";
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::TIMER) {
    result.timer = read_basic_timer(card, handle, state, timer);
    result.numeric = true;
    result.on = result.timer.mode == PageTimerMode::ACTIVE;
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE && !name.value.empty())
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
      else {
        const auto* text = translate ? translate("Timer") : nullptr;
        result.label = text ? text : "Timer";
      }
    }
    if (result.timer.mode == PageTimerMode::UNAVAILABLE)
      std::snprintf(result.number, sizeof(result.number), "--:--");
    else
      timer_state::format_seconds(result.timer.seconds, result.number, sizeof(result.number));
    return result;
  }
  if (card.kind == BasicCardKind::LIGHT_TEMPERATURE) {
    result.light_temperature = read_basic_light_temperature(card, handle, state, temperature);
    result.fill_percent = result.light_temperature.percent;
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
    }
    if (result.light_temperature.dragging) {
      result.formatted_label = true;
      std::snprintf(result.text, sizeof(result.text), "%dK", result.light_temperature.display_kelvin);
    }
    return result;
  }
  if (card.kind == BasicCardKind::NUMBER_SLIDER) {
    result.number_slider = read_basic_number_slider(card, handle, state, number_slider);
    result.fill_percent = result.number_slider.fill_percent;
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
    }
    if (result.number_slider.dragging && result.number_slider.available && result.number_slider.ready) {
      result.numeric = true;
      number_slider::format_value_into(
          result.number,
          number_slider::value_for_position(result.number_slider.metadata, result.number_slider.position),
          result.number_slider.metadata);
      const auto unit = find(state, handle, card.entity, "unit_of_measurement");
      if (unit.status == PageEntityStatus::AVAILABLE && !equal_state(unit.value, "none") &&
          !equal_state(unit.value, "null") && !equal_state(unit.value, "unknown") &&
          !equal_state(unit.value, "unavailable"))
        result.unit = unit.value.substr(0, HA_SHORT_STATE_MAX_LEN);
    }
    return result;
  }
  if (card.kind == BasicCardKind::LOCAL_RELAY) {
    result.label = card.label;
    if (result.label.empty()) {
      const auto* text = translate ? translate("Relay") : nullptr;
      result.label = text ? text : "Relay";
    }
    result.on = !card.push_feedback && relay && relay->on(handle.generation, card.entity);
    return result;
  }
  if (card.kind == BasicCardKind::LOCAL_NUMBER || card.kind == BasicCardKind::LOCAL_TEXT) {
    const bool text = card.kind == BasicCardKind::LOCAL_TEXT;
    const auto value = local ? local->read(handle, card.entity, text) : PageEntityValue{};
    if (text) {
      result.label = value.status == PageEntityStatus::AVAILABLE
                         ? value.value.substr(0, value.value.find('\0'))
                         : std::string_view("--");
      return result;
    }
    result.label = card.label;
    result.unit = card.unit;
    result.numeric = true;
    float number = std::numeric_limits<float>::quiet_NaN();
    if (value.status == PageEntityStatus::AVAILABLE && value.value.size() == sizeof(number))
      std::memcpy(&number, value.value.data(), sizeof(number));
    if (!std::isfinite(number))
      std::strcpy(result.number, "--");
    else
      std::snprintf(result.number, 32,
                    card.precision == 1   ? "%.1f"
                    : card.precision == 2 ? "%.2f"
                                          : "%.0f",
                    static_cast<double>(number));
    return result;
  }
  if (card.kind == BasicCardKind::OPTION_SELECT) {
    result.label = card.label;
    if (result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
    }
    if (result.label.empty()) result.label = card.entity;
    if (result.label.empty()) {
      const auto* text = translate ? translate("Option") : nullptr;
      result.label = text ? text : "Option";
    }
    const auto primary = find(state, handle, card.entity);
    const auto value = available(primary) && !equal_state(primary.value, {})
                           ? primary.value.substr(0, HA_STATE_TEXT_MAX_LEN)
                           : std::string_view("--");
    const auto length = std::min(value.size(), sizeof(result.text) - 1);
    if (length) std::memcpy(result.text, value.data(), length);
    result.text[length] = '\0';
    result.numeric = true;
    return result;
  }
  if (card.kind == BasicCardKind::FAN_SWITCH || card.kind == BasicCardKind::FAN_OSCILLATE) {
    const auto snapshot = fan_status ? fan_status->read(handle) : PageFanStatusSnapshot{};
    result.on = snapshot.on;
    const auto primary = find(state, handle, card.entity);
    const auto oscillation = find(state, handle, card.entity, "oscillating");
    if (primary.status == PageEntityStatus::EXHAUSTED ||
        (card.kind == BasicCardKind::FAN_OSCILLATE && oscillation.status == PageEntityStatus::EXHAUSTED)) {
      result.on = false;
      const char* key = primary.status == PageEntityStatus::EXHAUSTED ? "Unavailable" : "Unsupported";
      const auto* translated = translate ? translate(key) : nullptr;
      result.label = translated ? translated : key;
      return result;
    }
    const char* key = snapshot.showing ? PageFanStatusRuntime::text(snapshot.status) : card.default_label;
    result.label = card.label;
    if (snapshot.showing || result.label.empty()) {
      const auto name = find(state, handle, card.entity, "friendly_name");
      if (!snapshot.showing && card.label.empty() && name.status == PageEntityStatus::AVAILABLE)
        result.label = name.value.substr(0, HA_FRIENDLY_NAME_MAX_LEN);
      else {
        const auto* translated = translate && key ? translate(key) : nullptr;
        result.label = translated ? translated : key ? key : "";
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::FAN_DIRECTION || card.kind == BasicCardKind::FAN_PRESET) {
    const bool direction = card.kind == BasicCardKind::FAN_DIRECTION;
    const auto primary = find(state, handle, card.entity);
    const auto field = find(state, handle, card.entity, direction ? "direction" : "preset_mode");
    const auto value = field.status == PageEntityStatus::AVAILABLE
                           ? field.value.substr(0, HA_SHORT_STATE_MAX_LEN)
                           : std::string_view{};
    result.on = direction ? fan_mode_equal(value, "reverse") : fan_mode_preset_active(value);
    const auto label = [&](const char* key) {
      const auto* translated = translate ? translate(key) : nullptr;
      result.label = translated ? translated : key;
    };
    if (!available(primary) || equal_state(primary.value, {})) {
      label("Unavailable");
      return result;
    }
    if (direction) {
      if (!fan_mode_equal(value, "forward") && !fan_mode_equal(value, "reverse")) {
        label("Unsupported");
        return result;
      }
    } else {
      const auto options = find(state, handle, card.entity, "preset_modes");
      if (options.status != PageEntityStatus::AVAILABLE ||
          !fan_mode_has_options(options.value.substr(0, HA_TEXT_SENSOR_STATE_MAX_LEN))) {
        label("Unsupported");
        return result;
      }
      if (!result.on) {
        label("Preset");
        return result;
      }
    }
    format_fan_mode_text(result.text, sizeof(result.text), value);
    const auto* translated = translate ? translate(result.text) : nullptr;
    if (translated && translated != result.text) {
      const size_t length = std::min(std::strlen(translated), sizeof(result.text) - 1);
      std::memmove(result.text, translated, length);
      result.text[length] = '\0';
    }
    result.formatted_label = true;
    return result;
  }
  if (card.kind == BasicCardKind::MEDIA_NOW_PLAYING || card.kind == BasicCardKind::MEDIA_COVER_ART) {
    const bool cover = card.kind == BasicCardKind::MEDIA_COVER_ART;
    PageMediaRouteSnapshot route;
    if (cover && (!media_owner || !media_owner->cover_route(handle, route))) return result;
    const auto entity = cover ? route.entity : card.entity;
    result.media_idle = cover && route.idle_placeholder;
    result.media_source_title = cover && route.external_fallback;
    const auto primary = find(state, handle, entity);
    result.on=available(primary)&&primary.value=="playing";
    if(media_owner) {
      for(const auto attribute:{std::string_view("media_title"),std::string_view("media_artist")}) {
        const auto value = find(state, handle, entity, attribute);
        if (value.status == PageEntityStatus::AVAILABLE && media_owner->current(handle, entity, attribute)) {
          if (attribute == "media_title")
            result.media_title = value.value.substr(0, HA_STATE_TEXT_MAX_LEN);
          else
            result.media_artist = value.value.substr(0, HA_STATE_TEXT_MAX_LEN);
        }
      }
      result.media_external_input = media_owner->external_input(handle, entity);
      if (result.media_source_title) {
        const auto raw = find(state, handle, entity, "source");
        char decoded[HA_STATE_TEXT_MAX_LEN + 1];
        const auto source = raw.status == PageEntityStatus::AVAILABLE
                                ? decode_html_entities_bounded(decoded, sizeof(decoded),
                                                               raw.value.substr(0, HA_STATE_TEXT_MAX_LEN))
                                : std::string_view{};
        const auto effective = media_owner->effective_source(handle, entity, source);
        const auto count = std::min(effective.size(), sizeof(result.text) - 1);
        if (count) std::memcpy(result.text, effective.data(), count);
        result.text[count] = '\0';
      }
      if (card.media.progress) {
        PageMediaPlaybackSnapshot progress;
        if (media_owner->playback(handle, entity, progress)) result.fill_percent = progress.percent;
      }
    }
    return result;
  }
  if (card.kind == BasicCardKind::MEDIA_ACTION || card.kind == BasicCardKind::MEDIA_POSITION ||
      card.kind == BasicCardKind::MEDIA_CONTROL) {
    result.label=card.label;
    if(result.label.empty()&&card.default_label) {
      const auto *translated=translate?translate(card.default_label):nullptr;
      result.label=translated?translated:card.default_label;
    }
    const auto playback=find(state,handle,card.entity);
    // Playback state uses the same exact "playing" predicate as the existing
    // media owner; previous/next never acquire a checked background.
    const bool playing=playback.status==PageEntityStatus::AVAILABLE&&playback.value=="playing";
    if(card.media.kind==MediaActionKind::PLAY_PAUSE)result.on=playing;
    else if(card.media.kind==MediaActionKind::PLAYLIST) {
      const auto content=find(state,handle,card.entity,"media_content_id");
      const auto type=find(state,handle,card.entity,"media_content_type");
      char id_text[HA_STATE_TEXT_MAX_LEN+1],type_text[HA_SHORT_STATE_MAX_LEN+1];
      const auto metadata = [](char* output, size_t capacity, const PageEntityValue& value) {
        auto text = value.status == PageEntityStatus::AVAILABLE
                        ? decode_html_entities_bounded(output, capacity, value.value.substr(0, capacity - 1))
                        : std::string_view{};
        return text=="unknown"||text=="unavailable"?std::string_view{}:text;
      };
      const auto id = metadata(id_text, sizeof(id_text), content),
                 content_type = metadata(type_text, sizeof(type_text), type);
      const bool id_current = media_owner && media_owner->current(handle, card.entity, "media_content_id");
      const bool type_current =
          media_owner && media_owner->current(handle, card.entity, "media_content_type");
      result.on = playing && id_current && !id.empty() &&
                  media::playlist_content_id_matches(card.media.playlist_id, id) &&
                  type.status != PageEntityStatus::EXHAUSTED &&
                  (!type_current || content_type.empty() ||
                   media::playlist_content_type_matches(card.media.playlist_type, content_type));
    }
    if (card.kind == BasicCardKind::MEDIA_CONTROL) {
      if (card.media.kind == MediaActionKind::SPEAKER_GROUP) {
        const auto members = find(state, handle, card.entity, "group_members");
        result.on = playback.status == PageEntityStatus::AVAILABLE &&
                    media_group_source_available(playback.value.substr(0, HA_SHORT_STATE_MAX_LEN)) &&
                    members.status == PageEntityStatus::AVAILABLE &&
                    media_group_has_multiple(card.entity, members.value);
        return result;
      }
      result.on = card.media.kind == MediaActionKind::CONTROL_MODAL && playing;
      result.media_volume = read_media_volume(state, handle, card.entity, card.media.volume_max);
      if (card.media.volume_number) {
        result.numeric = true;
        if (result.media_volume.known)
          std::snprintf(result.number, sizeof(result.number), "%u",
                        static_cast<unsigned>(result.media_volume.display));
        else
          std::strcpy(result.number, "--");
      }
    }
    if(card.kind==BasicCardKind::MEDIA_POSITION) {
      PageMediaPlaybackSnapshot progress;
      if(media_owner)media_owner->playback(handle,card.entity,progress);
      media::format_playback_time(progress.seconds,result.number,sizeof(result.number));
      result.numeric=true;result.fill_percent=progress.percent;
    }
    if(card.media.show_state) {
      const char *key=nullptr;
      if (playback.status != PageEntityStatus::AVAILABLE || playback.value.empty() ||
          playback.value == "unknown")
        key = "Unknown";
      else if (playback.value == "unavailable")
        key = "Unavailable";
      else if(playback.value=="playing")key="Playing";
      else if(playback.value=="paused")key="Paused";
      else if(playback.value=="idle")key="Idle";
      else if(playback.value=="off")key="Off";
      if(key){const auto *translated=translate?translate(key):nullptr;result.label=translated?translated:key;}
      else {format_sensor_text(result.text,sizeof(result.text),playback.value,HA_SHORT_STATE_MAX_LEN);result.formatted_label=true;}
    }
    return result;
  }
  if(card.kind==BasicCardKind::WEATHER) {
    const auto value=find(state,handle,card.entity);
    const auto text=card.entity.empty()?std::string_view("cloudy"):value.status==PageEntityStatus::AVAILABLE?value.value:std::string_view("unavailable");
    const auto condition=espcontrol::weather::condition_text(text,result.text,sizeof(result.text));
    result.weather_condition=condition.index;result.formatted_label=condition.label==nullptr;
    if(condition.label){const auto *translated=translate?translate(condition.label):nullptr;result.label=translated?translated:condition.label;}
    return result;
  }
  if(card.kind==BasicCardKind::CLIMATE) {
    const auto climate=read_climate_card(card.entity,card.label,card.climate,handle,state,temperature_unit,translate);
    result.on=climate.active;result.climate_icon_on=climate.icon_on;result.numeric=climate.number;result.unit=climate.unit;
    result.label=climate.configured_label;result.formatted_label=climate.configured_label.empty();
    std::memcpy(result.number,climate.value,sizeof(result.number));std::memcpy(result.text,climate.label,sizeof(result.text));return result;
  }
  result.label=card.label;result.unit=card.unit;
  const bool folder_number=card.folder_mode==BasicFolderMode::NUMBER,folder_text=card.folder_mode==BasicFolderMode::TEXT;
  const bool action=card.kind==BasicCardKind::ACTION;
  const bool action_number=action&&card.action_display==BasicActionDisplay::NUMBER;
  const bool sensor=action||card.kind==BasicCardKind::STATUS||folder_number||folder_text||card.kind==BasicCardKind::NUMBER||card.kind==BasicCardKind::ICON||card.kind==BasicCardKind::TEXT||card.kind==BasicCardKind::DURATION;
  const auto source=sensor?card.sensor:card.kind==BasicCardKind::FOLDER&&card.folder_mode==BasicFolderMode::NONE?std::string_view{}:card.entity;
  const auto primary=action?(card.active_color?state.dependency(handle,0):PageEntityValue{}):find(state,handle,source);
  result.on=available(primary)&&on(primary.value);
  if(card.kind==BasicCardKind::COVER) {
    std::optional<uint8_t> percent;
    for(size_t ordinal=0;ordinal<state.dependency_count(handle);++ordinal) {
      const auto value=state.dependency(handle,ordinal);
      if(value.entity==card.entity&&value.attribute=="current_position"){percent=state.last_percent(handle,ordinal);break;}
    }
    if(percent) {result.on=*percent>0;result.fill_percent=100-*percent;}
    else {
      result.on=available(primary)&&(equal_state(primary.value,"open")||equal_state(primary.value,"opening"));
      result.fill_percent=available(primary)&&(equal_state(primary.value,"closed")||equal_state(primary.value,"closing"))?100:0;
    }
  }
  if(card.kind==BasicCardKind::STATUS&&card.presence&&available(primary))result.on|=equal_state(primary.value,"detected");
  if(action_number)result.on=false;
  if(card.kind==BasicCardKind::FOLDER) {
    result.on=card.folder_mode==BasicFolderMode::ENTITY||card.folder_mode==BasicFolderMode::CLIMATE||card.folder_mode==BasicFolderMode::MOWER?
      read_basic_indicator(handle,card.entity,state,card.folder_mode):false;
  }
  if(!action&&result.label.empty()&&!source.empty()) {
    const auto friendly=find(state,handle,source,"friendly_name");
    if(friendly.status==PageEntityStatus::AVAILABLE)result.label=friendly.value.substr(0,HA_FRIENDLY_NAME_MAX_LEN);
  }
  if(result.label.empty()&&card.default_label) {
    const auto *translated=translate?translate(card.default_label):nullptr;
    result.label=translated?translated:card.default_label;
  }
  const auto text_value=[&](const PageEntityValue &value,bool mapping) {
    result.unit={};
    if(value.status!=PageEntityStatus::AVAILABLE){result.label="--";return;}
    if(mapping) {
      const auto normalized=trim(value.value.substr(0,HA_TEXT_SENSOR_STATE_MAX_LEN));
      const auto equal=[normalized](std::string_view expected) {
        if(expected.empty()||expected.size()!=normalized.size())return false;
        for(size_t i=0;i<expected.size();++i)if(std::tolower(static_cast<unsigned char>(normalized[i]))!=expected[i])return false;
        return true;
      };
      if(equal(card.text_input)){result.label=card.text_output;return;}
      if(equal(card.text_input_2)){result.label=card.text_output_2;return;}
    }
    format_sensor_text(result.text,sizeof(result.text),value.value);result.formatted_label=true;
  };
  if(action&&card.action_display==BasicActionDisplay::TEXT) {
    result.unit={};
    if(primary.status!=PageEntityStatus::AVAILABLE)result.label="--";
    else {format_sensor_text(result.text,sizeof(result.text),primary.value,HA_STATE_TEXT_MAX_LEN);result.formatted_label=true;}
    return result;
  }
  if(card.kind==BasicCardKind::TEXT||folder_text){text_value(primary,!folder_text);return result;}
  if(card.kind==BasicCardKind::TOGGLE&&card.text_overlay) {
    result.unit={};if(result.on)text_value(find(state,handle,card.sensor),false);return result;
  }
  if(card.kind==BasicCardKind::DURATION) {
    result.unit={};
    const auto unit=card.time_unit.empty()?find(state,handle,card.sensor,"unit_of_measurement"):PageEntityValue{};
    if(available(primary)&&primary.value.size()<=HA_TEXT_SENSOR_STATE_MAX_LEN&&(!card.time_unit.empty()||unit.status==PageEntityStatus::AVAILABLE)) {
      char input[HA_TEXT_SENSOR_STATE_MAX_LEN+1];std::memcpy(input,primary.value.data(),primary.value.size());input[primary.value.size()]='\0';
      char *end=nullptr;const auto value=std::strtod(input,&end);
      if(end==input)return result;
      while(end<input+primary.value.size()&&std::isspace(static_cast<unsigned char>(*end)))++end;
      if(end!=input&&end==input+primary.value.size())result.numeric=format_duration_value(result.number,sizeof(result.number),value,card.time_unit.empty()?unit.value:card.time_unit,duration_components);
    }
    return result;
  }
  const auto number=action_number||card.kind==BasicCardKind::NUMBER||folder_number?primary:find(state,handle,card.sensor);
  if((action_number||folder_number||card.kind==BasicCardKind::NUMBER || (card.kind==BasicCardKind::TOGGLE&&!card.sensor.empty()))&&available(number)) {
    // Packed cache views have no trailing NUL. Never pass their data() directly
    // to strtof, and reject oversized input instead of reading its neighbour.
    char text[HA_TEXT_SENSOR_STATE_MAX_LEN+1];
    if(number.value.size()<=sizeof(text)-1) {
      std::memcpy(text,number.value.data(),number.value.size());text[number.value.size()]='\0';
      char *end=nullptr;const float value=std::strtof(text,&end);
      if(end!=text&&std::isfinite(value)) {
        // Keep the existing formatter's fast integer path and rounding for
        // normal values. A bounded float fallback handles its former overflow.
        const int precision=std::min<int>(card.precision,3);
        int scale=1;for(uint8_t i=0;i<precision;++i)scale*=10;
        const float rounded=std::fabs(value)*scale+0.5f;
        if(static_cast<double>(rounded)<2147483648.0) {
          const int scaled=static_cast<int>(rounded),whole=scaled/scale,fraction=scaled%scale;
          const char *sign=value<0?"-":"";
          if(precision==0)std::snprintf(result.number,sizeof(result.number),"%s%d",sign,whole);
          else std::snprintf(result.number,sizeof(result.number),"%s%d.%0*d",sign,whole,precision,fraction);
        } else std::snprintf(result.number,sizeof(result.number),"%.*f",precision,static_cast<double>(value));
        result.numeric=true;
        if(action_number)result.on=value>0;
        if(card.kind==BasicCardKind::NUMBER&&card.active_color&&value>0)result.on=true;
      }
    }
  }
  if(!result.numeric)result.unit={};
  return result;
}
}  // namespace espcontrol::pages
