#include "page_basic_card.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "button_grid_subpage_parser.h"
#include "bounded_sensor_text.h"

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
  if(type.empty() || type=="light_switch") {
    if(cfg_option_enabled(options,"confirm_off") || cfg_option_enabled(options,"confirm_on"))return false;
    out.text_overlay=!out.sensor.empty()&&precision=="text";
    out.kind=out.entity.empty()?BasicCardKind::EMPTY:BasicCardKind::TOGGLE;
  } else if(type=="sensor") {
    if(out.sensor=="local")return false;
    out.kind=out.sensor.empty()?BasicCardKind::EMPTY:precision=="icon"?BasicCardKind::ICON:precision=="text"?BasicCardKind::TEXT:precision=="time"?BasicCardKind::DURATION:BasicCardKind::NUMBER;
    out.active_color=out.kind!=BasicCardKind::DURATION&&cfg_option_enabled(options,"active_color");
  } else if(type=="subpage") {
    // Summary/indicator values require the aggregate driver, still to be ported.
    if(!out.sensor.empty() || !out.entity.empty())return false;
    out.kind=BasicCardKind::FOLDER;
  } else return false;
  const auto copy=[strings](const std::string &value,std::string_view &stored) {
    if(value.empty()){stored={};return true;}
    return strings&&strings->copy(value,stored);
  };
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
  out.precision=static_cast<uint8_t>(parse_precision(std::string(precision)));
  out.large_numbers=(out.kind==BasicCardKind::NUMBER||out.kind==BasicCardKind::DURATION)&&!large_numbers_explicitly_disabled(options);
  const auto mode=cfg_option_value(options,"card_active_color");out.theme_active=mode=="theme";
  auto color=cfg_option_value(options,"card_off_color");
  if(!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.off_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_off=true;}
  color=cfg_option_value(options,"card_on_color");
  if(mode=="custom"&&!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.on_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_on=true;}
  return true;
}
BasicCardState read_basic_card(const BasicCardDefinition &card,ConfigurationHandle handle,PageEntityStateRuntime &state,int duration_components) {
  BasicCardState result;result.label=card.label;result.unit=card.unit;
  const bool sensor=card.kind==BasicCardKind::NUMBER||card.kind==BasicCardKind::ICON||card.kind==BasicCardKind::TEXT||card.kind==BasicCardKind::DURATION;
  const auto source=sensor?card.sensor:card.entity;
  const auto primary=find(state,handle,source);
  result.on=available(primary)&&on(primary.value);
  if(result.label.empty()&&!source.empty()) {
    const auto friendly=find(state,handle,source,"friendly_name");
    if(friendly.status==PageEntityStatus::AVAILABLE)result.label=friendly.value.substr(0,HA_FRIENDLY_NAME_MAX_LEN);
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
  if(card.kind==BasicCardKind::TEXT){text_value(primary,true);return result;}
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
  const auto number=card.kind==BasicCardKind::NUMBER?primary:find(state,handle,card.sensor);
  if((card.kind==BasicCardKind::NUMBER || (card.kind==BasicCardKind::TOGGLE&&!card.sensor.empty()))&&available(number)) {
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
        if(card.kind==BasicCardKind::NUMBER&&card.active_color&&value>0)result.on=true;
      }
    }
  }
  if(!result.numeric)result.unit={};
  return result;
}
}  // namespace espcontrol::pages
