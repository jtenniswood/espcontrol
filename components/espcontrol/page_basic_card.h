#pragma once

#include "alarm_card_view.h"
#include "cleaning_card_policy.h"
#include "clock_card_view.h"
#include "cover_command_policy.h"
#include "page_access_state_runtime.h"
#include "page_calendar_runtime.h"
#include "page_climate_card.h"
#include "page_cover_toggle_runtime.h"
#include "page_entity_state_runtime.h"
#include "page_fan_status_runtime.h"
#include "page_light_temperature_runtime.h"
#include "page_local_relay_runtime.h"
#include "page_local_sensor_runtime.h"
#include "page_media_action.h"
#include "page_media_state_runtime.h"
#include "page_media_volume.h"
#include "page_number_slider_runtime.h"
#include "page_percent_slider_runtime.h"
#include "page_renderer_runtime.h"
#include "page_timer_runtime.h"
#include "weather_card_view.h"

namespace espcontrol::pages {
enum class BasicCardKind : uint8_t {
  EMPTY,
  TOGGLE,
  NUMBER,
  ICON,
  FOLDER,
  TEXT,
  DURATION,
  CLIMATE,
  STATUS,
  ACTION,
  LIGHT,
  COVER,
  WEATHER,
  FORECAST,
  MEDIA_ACTION,
  MEDIA_POSITION,
  MEDIA_NOW_PLAYING,
  MEDIA_COVER_ART,
  MEDIA_CONTROL,
  FAN_CONTROL,
  FAN_DIRECTION,
  FAN_PRESET,
  OPTION_SELECT,
  LOCAL_NUMBER,
  LOCAL_TEXT,
  LOCAL_RELAY,
  FAN_SWITCH,
  FAN_OSCILLATE,
  PERCENT_SLIDER,
  NUMBER_SLIDER,
  LIGHT_TEMPERATURE,
  TIMER,
  COVER_COMMAND,
  COVER_TOGGLE,
  ACCESS,
  CLEANING,
  CALENDAR,
  CLOCK,
  ALARM
};
enum class BasicActionDisplay:uint8_t { NONE, ICON, TEXT, NUMBER };
enum class BasicFolderMode : uint8_t { NONE, ENTITY, CLIMATE, MOWER, TIMER, CHILDREN, NUMBER, TEXT };
enum class BasicAccessKind : uint8_t { GARAGE, GATE, LOCK };
struct BasicAccessOptions {
  BasicAccessKind kind;
  bool command, status;
};
struct BasicCardDefinition {
  BasicCardKind kind{BasicCardKind::EMPTY};
  BasicFolderMode folder_mode{BasicFolderMode::NONE};
  ClimateCardOptions climate;
  MediaActionOptions media;
  union {
    weather::View forecast_view{weather::View::DAILY};
    PagePercentSliderKind percent_slider;
    BasicAccessOptions access;
    cleaning::Options cleaning;
    bool calendar_time;
    ClockCardOptions clock;
    AlarmCardOptions alarm;
  };
  BasicActionDisplay action_display{BasicActionDisplay::NONE};
  std::string_view entity,sensor,label,unit,icon,icon_on;
  const char *default_label{nullptr};
  std::string_view text_input,text_output,text_input_2,text_output_2,time_unit;
  bool text_overlay{false};
  uint32_t off_color{0},on_color{0};
  uint8_t precision{0};
  bool custom_off{false},custom_on{false},theme_active{false},active_color{false},large_numbers{false};
  bool presence{false}, push_feedback{false}, explicit_large{false};
};
// Decoded mapping labels/time units are copied once into preparation storage.
class BasicCardTextSink {
 public:
  virtual ~BasicCardTextSink()=default;
  virtual bool copy(std::string_view value,std::string_view &stored)=0;
  virtual bool copy_unit(std::string_view value,std::string_view &stored){return copy(value,stored);}
};
// Preparation only: reject not-yet-ported variants instead of showing a partial
// interpretation. Fields borrow PageStore; decoded mapping/unit strings borrow
// the supplied sink and must remain alive until this generation retires.
bool basic_card_is_forecast(const PageStore &,CardId);
bool prepare_basic_card(const PageStore&, CardId, BasicCardDefinition&, BasicCardTextSink* text = nullptr,
                        PageLocalRelayRuntime* relay = nullptr);
struct PageCoverCommandSnapshot {
  int features{0};
  bool known{false}, supported{false}, tilt{false};
};
struct PageAccessActionSnapshot {
  bool on{false}, locked{false};
};
struct PageAccessCardSnapshot {
  PageAccessSnapshot history;
  bool enabled{false};
};
struct PageCleaningSnapshot {
  const char* service{nullptr};
  const char* label_key{nullptr};
  cleaning::Glyph glyph{cleaning::Glyph::VACUUM};
  bool state_seen{false}, on{false};
};
struct PageAlarmCardSnapshot {
  PageAlarmSnapshot history;
  AlarmCardGlyph glyph{AlarmCardGlyph::ALARM};
  bool enabled{false}, dynamic_icon{false};
};
struct BasicCardState {
  bool on{false}, numeric{false}, formatted_label{false}, climate_icon_on{false}, cover_icon_open{false};
  std::string_view label,unit,media_title,media_artist;
  bool media_external_input{false}, media_idle{false}, media_source_title{false};
  PageMediaVolumeSnapshot media_volume;
  PagePercentSliderSnapshot percent_slider;
  PageNumberSliderSnapshot number_slider;
  PageLightTemperatureSnapshot light_temperature;
  PageTimerSnapshot timer;
  PageCoverCommandSnapshot cover_command;
  PageAccessCardSnapshot access;
  PageCleaningSnapshot cleaning;
  PageAlarmCardSnapshot alarm;
  bool clock_city_label{false};
  uint8_t fill_percent{0},weather_condition{0};
  char number[48]{},text[257]{};
};
// Pull only from the configuration-owned cache. It neither registers a callback
// nor owns a copy of HA state. The result's views last until the next delivery.
std::string_view basic_folder_child_indicator(const PageStore &,CardId);
bool read_basic_indicator(ConfigurationHandle,std::string_view,PageEntityStateRuntime &,BasicFolderMode=BasicFolderMode::ENTITY);
PagePercentSliderSnapshot read_basic_percent_slider(const BasicCardDefinition&, ConfigurationHandle,
                                                    PageEntityStateRuntime&, PagePercentSliderRuntime*);
PageNumberSliderSnapshot read_basic_number_slider(const BasicCardDefinition&, ConfigurationHandle,
                                                  PageEntityStateRuntime&, PageNumberSliderRuntime*);
PageLightTemperatureSnapshot read_basic_light_temperature(const BasicCardDefinition&, ConfigurationHandle,
                                                          PageEntityStateRuntime&,
                                                          PageLightTemperatureRuntime*);
PageTimerSnapshot read_basic_timer(const BasicCardDefinition&, ConfigurationHandle, PageEntityStateRuntime&,
                                   PageTimerRuntime*);
PageCoverCommandSnapshot read_basic_cover_command(const BasicCardDefinition&, ConfigurationHandle,
                                                  PageEntityStateRuntime&);
PageAccessCardSnapshot read_basic_access(const BasicCardDefinition&, ConfigurationHandle,
                                         PageEntityStateRuntime&, PageAccessStateRuntime*);
PageCleaningSnapshot read_basic_cleaning(const BasicCardDefinition&, ConfigurationHandle,
                                         PageEntityStateRuntime&);
PageAlarmCardSnapshot read_basic_alarm(const BasicCardDefinition&, ConfigurationHandle,
                                       PageEntityStateRuntime&, PageAlarmStateRuntime*);
BasicCardState read_basic_card(
    const BasicCardDefinition&, ConfigurationHandle, PageEntityStateRuntime&, int duration_components = 2,
    std::string_view temperature_unit = "°C", PageCardTranslate translate = nullptr,
    PageMediaStateRuntime* media = nullptr, PageLocalSensorRuntime* local = nullptr,
    PageLocalRelayRuntime* relay = nullptr, PageFanStatusRuntime* fan_status = nullptr,
    PagePercentSliderRuntime* percent_slider = nullptr, PageNumberSliderRuntime* number_slider = nullptr,
    PageLightTemperatureRuntime* temperature = nullptr, PageTimerRuntime* timer = nullptr,
    PageCoverToggleRuntime* cover_toggle = nullptr, PageAccessStateRuntime* access = nullptr,
    PageCalendarRuntime* calendar = nullptr, PageTimezoneRuntime* timezone = nullptr,
    PageAlarmStateRuntime* alarm = nullptr);
}  // namespace espcontrol::pages
