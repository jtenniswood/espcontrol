#include "page_basic_card.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "button_grid_subpage_parser.h"

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
bool prepare_basic_card(const PageStore &store,CardId id,BasicCardDefinition &out) {
  out={};const auto *card=store.card(id);if(card==nullptr)return false;
  out.entity=store.field(*card,CardField::ENTITY);out.sensor=store.field(*card,CardField::SENSOR);
  out.label=store.field(*card,CardField::LABEL);out.unit=trim(store.field(*card,CardField::UNIT));
  out.icon=store.field(*card,CardField::ICON);out.icon_on=store.field(*card,CardField::ICON_ON);
  const auto type=store.field(*card,CardField::TYPE),precision=store.field(*card,CardField::PRECISION);
  const std::string options(store.field(*card,CardField::OPTIONS));
  if(type.empty() || type=="light_switch") {
    if(precision=="text" || (cfg_option_enabled(options,"confirm_off") || cfg_option_enabled(options,"confirm_on")))return false;
    out.kind=out.entity.empty()?BasicCardKind::EMPTY:BasicCardKind::TOGGLE;
  } else if(type=="sensor") {
    if(out.sensor=="local" || precision=="text" || precision=="time")return false;
    out.kind=out.sensor.empty()?BasicCardKind::EMPTY:precision=="icon"?BasicCardKind::ICON:BasicCardKind::NUMBER;
    out.active_color=cfg_option_enabled(options,"active_color");
  } else if(type=="subpage") {
    // Summary/indicator values require the aggregate driver, still to be ported.
    if(!out.sensor.empty() || !out.entity.empty())return false;
    out.kind=BasicCardKind::FOLDER;
  } else return false;
  out.precision=static_cast<uint8_t>(parse_precision(std::string(precision)));
  out.large_numbers=out.kind==BasicCardKind::NUMBER&&!large_numbers_explicitly_disabled(options);
  const auto mode=cfg_option_value(options,"card_active_color");out.theme_active=mode=="theme";
  auto color=cfg_option_value(options,"card_off_color");
  if(!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.off_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_off=true;}
  color=cfg_option_value(options,"card_on_color");
  if(mode=="custom"&&!color.empty()){color=normalize_card_color_value(color);if(color.empty())return false;out.on_color=static_cast<uint32_t>(std::strtoul(color.c_str(),nullptr,16));out.custom_on=true;}
  return true;
}
BasicCardState read_basic_card(const BasicCardDefinition &card,ConfigurationHandle handle,PageEntityStateRuntime &state) {
  BasicCardState result;result.label=card.label;result.unit=card.unit;
  const auto source=card.kind==BasicCardKind::NUMBER || card.kind==BasicCardKind::ICON?card.sensor:card.entity;
  const auto primary=find(state,handle,source);
  result.on=available(primary)&&on(primary.value);
  if(result.label.empty()&&!source.empty()) {
    const auto friendly=find(state,handle,source,"friendly_name");
    if(friendly.status==PageEntityStatus::AVAILABLE)result.label=friendly.value.substr(0,HA_FRIENDLY_NAME_MAX_LEN);
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
        // Same decimal rounding as format_fixed_decimal for ordinary values;
        // avoid its int overflow for out-of-range HA numbers.
        int scale=1;for(uint8_t i=0;i<card.precision;++i)scale*=10;
        const double scaled=std::floor(static_cast<double>(std::fabs(value))*scale+0.5);
        const char *sign=value<0?"-":"";
        const double rounded=std::fabs(value)<static_cast<float>(INT32_MAX/scale-1)?static_cast<double>(static_cast<int>(std::fabs(value)*scale+0.5f)):scaled;
        const double whole=std::floor(rounded/scale);const auto fraction=static_cast<unsigned>(std::fmod(rounded,scale));
        if(card.precision==0)std::snprintf(result.number,sizeof(result.number),"%s%.0f",sign,whole);
        else std::snprintf(result.number,sizeof(result.number),"%s%.0f.%0*u",sign,whole,card.precision,fraction);
        result.numeric=true;
        if(card.kind==BasicCardKind::NUMBER&&card.active_color&&value>0)result.on=true;
      }
    }
  }
  if(!result.numeric)result.unit={};
  return result;
}
}  // namespace espcontrol::pages
