#include "page_basic_card.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "button_grid_subpage_parser.h"
#include "bounded_sensor_text.h"
#include "climate_state_logic.h"

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
  for(const auto state:{"on","true","1","home","playing","open","opened","opening","closing","unlocked","unlocking","jammed"})
    if(equal_state(text,state))return true;
  return false;
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
bool prepare_basic_card(const PageStore &store,CardId id,BasicCardDefinition &out,BasicCardTextSink *strings) {
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
  if(type.empty() || type=="light_switch") {
    if(cfg_option_enabled(options,"confirm_off") || cfg_option_enabled(options,"confirm_on"))return false;
    out.text_overlay=precision=="text";
    out.kind=out.entity.empty()?BasicCardKind::EMPTY:BasicCardKind::TOGGLE;
  } else if(type=="sensor") {
    if(out.sensor=="local")return false;
    out.kind=out.sensor.empty()?BasicCardKind::EMPTY:precision=="icon"?BasicCardKind::ICON:precision=="text"?BasicCardKind::TEXT:precision=="time"?BasicCardKind::DURATION:BasicCardKind::NUMBER;
    out.active_color=out.kind!=BasicCardKind::DURATION&&cfg_option_enabled(options,"active_color");
  } else if(type=="door_window"||type=="presence") {
    out.kind=out.sensor.empty()?BasicCardKind::EMPTY:BasicCardKind::STATUS;
    out.presence=type=="presence";out.active_color=cfg_option_enabled(options,"active_color");
    const bool window=normalize_door_window_subtype(std::string(precision))=="window";
    if(out.kind==BasicCardKind::STATUS) {
      out.default_label=out.presence?"Presence":window?"Window":"Door";
      if(out.icon.empty()||out.icon=="Auto")out.icon=out.presence?"Motion Sensor Off":window?"Window Closed":"Door";
      if(out.icon_on.empty()||out.icon_on=="Auto")out.icon_on=out.presence?"Motion Sensor":window?"Window Open":"Door Open";
    }
  } else if(type=="action"||type=="local"||type=="push"||type=="webhook"||type=="screensaver") {
    ParsedCfg config;config.type=type;config.entity=out.entity;config.sensor=out.sensor;config.options=options;
    if(action_card_option_select(config)||action_script_confirmation_enabled(config))return false;
    out.kind=BasicCardKind::ACTION;
    const bool local=type=="local"||action_card_local_action(config);
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
  } else if(type=="climate"||type=="climate_control") {
    out.kind=BasicCardKind::CLIMATE;out.active_color=true;
    const auto label=normalize_climate_label_display(cfg_option_value(options,"label_display"));
    out.climate.label=label=="status"?ClimateCardLabel::STATUS:label=="actual"?ClimateCardLabel::ACTUAL:label=="target"?ClimateCardLabel::TARGET:ClimateCardLabel::LABEL;
    const auto number=normalize_climate_number_display(cfg_option_value(options,"number_display"));
    out.climate.number=number=="actual"?ClimateCardNumber::ACTUAL:number=="icon"?ClimateCardNumber::ICON:ClimateCardNumber::TARGET;
    out.climate.half_step=normalize_climate_temperature_step(cfg_option_value(options,"temperature_step"))=="0.5";
  } else if(type=="subpage") {
    out.kind=BasicCardKind::FOLDER;
    if(out.sensor=="indicator") {
      if(out.entity.empty())out.folder_mode=BasicFolderMode::CHILDREN;
      else {
        const auto kind=normalize_subpage_kind(cfg_option_value(options,"subpage_kind"));
        out.folder_mode=kind=="climate"?BasicFolderMode::CLIMATE:kind=="lawn_mower"?BasicFolderMode::MOWER:BasicFolderMode::ENTITY;
      }
      out.active_color=true;
    } else if(!out.sensor.empty())out.folder_mode=precision=="text"?BasicFolderMode::TEXT:BasicFolderMode::NUMBER;
  } else return false;
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
  out.large_numbers=(out.kind==BasicCardKind::NUMBER||out.kind==BasicCardKind::DURATION||(out.kind==BasicCardKind::CLIMATE&&out.climate.number!=ClimateCardNumber::ICON)||out.folder_mode==BasicFolderMode::NUMBER||(out.kind==BasicCardKind::ACTION&&out.action_display==BasicActionDisplay::NUMBER))&&!large_numbers_explicitly_disabled(options);
  const auto mode=cfg_option_value(options,"card_active_color");out.theme_active=mode=="theme";
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
  // indicator semantics (timer/mower/local controls use different predicates).
  return type.empty()||type=="light_switch"?store.field(*card,CardField::ENTITY):std::string_view{};
}
bool read_basic_indicator(ConfigurationHandle handle,std::string_view entity,PageEntityStateRuntime &state,BasicFolderMode mode) {
  const auto primary=find(state,handle,entity);if(!available(primary))return false;
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
BasicCardState read_basic_card(const BasicCardDefinition &card,ConfigurationHandle handle,PageEntityStateRuntime &state,int duration_components,std::string_view temperature_unit,PageCardTranslate translate) {
  BasicCardState result;
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
        int scale=1;for(int i=0;i<precision;++i)scale*=10;
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
