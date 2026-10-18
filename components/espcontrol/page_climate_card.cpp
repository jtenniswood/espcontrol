#include "page_climate_card.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "bounded_display_text.h"
#include "bounded_sensor_text.h"
#include "climate_state_logic.h"
#include "climate_target_logic.h"
#include "ha_state_limits.h"

namespace espcontrol::pages {
namespace {
constexpr size_t STATE_LIMIT=HA_SHORT_STATE_MAX_LEN;
std::string_view trim(std::string_view value) {
  while(!value.empty()&&std::isspace(static_cast<unsigned char>(value.front())))value.remove_prefix(1);
  while(!value.empty()&&std::isspace(static_cast<unsigned char>(value.back())))value.remove_suffix(1);
  return value;
}
std::string_view lower(char *output,std::string_view value) {
  value=trim(value.substr(0,STATE_LIMIT));
  for(size_t i=0;i<value.size();++i)output[i]=static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
  output[value.size()]='\0';return {output,value.size()};
}
// Match the existing climate option cleaning, including HA enum spellings.
std::string_view mode(char *output,std::string_view value) {
  value=trim(value.substr(0,STATE_LIMIT));
  while(!value.empty()&&(value.front()=='\''||value.front()=='"'||value.front()=='['||value.front()=='<'))value.remove_prefix(1);
  while(!value.empty()&&(value.back()=='\''||value.back()=='"'||value.back()==']'||value.back()=='>'))value.remove_suffix(1);
  value=trim(value);const auto colon=value.find(':');
  if(colon!=std::string_view::npos) {
    auto left=trim(value.substr(0,colon));const auto dot=left.rfind('.');
    value=trim(dot!=std::string_view::npos&&dot+1<left.size()?left.substr(dot+1):value.substr(colon+1));
  } else {
    const auto dot=value.rfind('.');
    if(dot!=std::string_view::npos&&dot+1<value.size()) {
      char prefix[STATE_LIMIT+1];
      if(lower(prefix,value.substr(0,dot)).find("mode")!=std::string_view::npos)value=trim(value.substr(dot+1));
    }
  }
  while(!value.empty()&&(value.front()=='\''||value.front()=='"'||value.front()=='<'))value.remove_prefix(1);
  while(!value.empty()&&(value.back()=='\''||value.back()=='"'||value.back()=='>'))value.remove_suffix(1);
  auto normalized=lower(output,value);
  for(size_t i=0;i<normalized.size();++i)if(output[i]==' '||output[i]=='-'||output[i]=='/')output[i]='_';
  if(normalized=="heatcool"||normalized=="heat__cool"){std::strcpy(output,"heat_cool");return {output,9};}
  if(normalized=="fan"||normalized=="fanonly"){std::strcpy(output,"fan_only");return {output,8};}
  return normalized;
}
bool tenths(std::string_view value,int &result) {
  if(value.empty()||value.size()>256)return false;
  char bytes[257];std::memcpy(bytes,value.data(),value.size());bytes[value.size()]='\0';
  char *end=nullptr;const float number=std::strtof(bytes,&end);
  if(end==bytes||!std::isfinite(number))return false;
  const float rounded=number*10.0f+(number>=0?0.5f:-0.5f);
  if(!std::isfinite(rounded)||static_cast<double>(rounded)<std::numeric_limits<int>::min()||
      static_cast<double>(rounded)>std::numeric_limits<int>::max())return false;
  result=static_cast<int>(rounded);return true;
}
bool features(std::string_view value,int &result) {
  char bytes[STATE_LIMIT+1];lower(bytes,value);char *end=nullptr;
  const long number=std::strtol(bytes,&end,10);
  if(end==bytes||number<0||number>std::numeric_limits<int>::max())return false;
  result=static_cast<int>(number);return true;
}
void temperature(char *output,size_t capacity,int value,uint8_t precision) {
  const int64_t magnitude=value<0?-static_cast<int64_t>(value):value;
  if(precision==0)std::snprintf(output,capacity,"%lld",static_cast<long long>((static_cast<int64_t>(value)+(value>=0?5:-5))/10));
  else std::snprintf(output,capacity,"%s%lld.%lld%s",value<0?"-":"",static_cast<long long>(magnitude/10),static_cast<long long>(magnitude%10),precision==1?"":precision==2?"0":"00");
}
const char *status_key(climate::Status status,std::string_view mode) {
  switch(status) {
    case climate::Status::UNAVAILABLE:return "Unavailable";
    case climate::Status::OFF:return "Off";
    case climate::Status::HEATING:return "Heating";
    case climate::Status::COOLING:return "Cooling";
    case climate::Status::DRYING:return "Drying";
    case climate::Status::FAN:return "Fan";
    case climate::Status::IDLE:return "Idle";
    case climate::Status::MODE_FALLBACK:
      if(mode=="heat")return "Heat";
      if(mode=="cool")return "Cool";
      if(mode=="heat_cool")return "Heat/Cool";
      if(mode=="auto")return "Auto";
      if(mode=="dry")return "Dry";
      if(mode=="fan_only")return "Fan";
      if(mode=="nohold")return "Nohold";
      if(mode=="holduntil"||mode=="hold_until")return "Holduntil";
      if(mode=="permanenthold"||mode=="permanent_hold")return "Permanenthold";
      if(mode=="temporaryhold"||mode=="temporary_hold")return "Temporaryhold";
      if(mode=="vacationhold"||mode=="vacation_hold")return "Vacationhold";
      return nullptr;
  }
  return "Idle";
}
}
ClimateCardPresentation read_climate_card(std::string_view entity,std::string_view configured_label,
    ClimateCardOptions options,ConfigurationHandle card,PageEntityStateRuntime &state,
    std::string_view unit,PageCardTranslate translate) {
  ClimateCardPresentation out;char mode_buffer[STATE_LIMIT+1]{},action_buffer[STATE_LIMIT+1]{};
  std::string_view hvac_mode,hvac_action;bool available=false,known=false,has_target=false,has_current=false,has_low=false,has_high=false;
  int supported=0,target=0,current=0,low=0,high=0;
  const size_t count=state.dependency_count(card);
  for(size_t i=0;i<count;++i) {
    const auto value=state.dependency(card,i);
    if(value.entity!=entity||value.status!=PageEntityStatus::AVAILABLE)continue;
    if(value.attribute.empty()){hvac_mode=mode(mode_buffer,value.value);available=!climate::unavailable_value(hvac_mode);}
    else if(value.attribute=="hvac_action")hvac_action=lower(action_buffer,value.value);
    else if(value.attribute=="supported_features")known=features(value.value,supported);
    else if(value.attribute=="temperature")has_target=tenths(value.value,target);
    else if(value.attribute=="current_temperature")has_current=tenths(value.value,current);
    else if(value.attribute=="target_temp_low")has_low=tenths(value.value,low);
    else if(value.attribute=="target_temp_high")has_high=tenths(value.value,high);
  }
  out.active=climate::active(available,hvac_mode,hvac_action);out.icon_on=climate::icon_enabled(available,hvac_mode);
  char target_text[48]="--",actual_text[24]="--";
  const auto kind=climate::target_kind(known,supported,has_target,has_low,has_high);
  const uint8_t target_precision=options.half_step&&options.precision==0?1:options.precision;
  if(available&&kind==climate::TargetKind::SINGLE&&has_target)temperature(target_text,sizeof(target_text),target,target_precision);
  else if(available&&kind==climate::TargetKind::RANGE&&has_low&&has_high) {
    char first[24],second[24];temperature(first,sizeof(first),low,target_precision);temperature(second,sizeof(second),high,target_precision);
    std::snprintf(target_text,sizeof(target_text),"%.22s-%.22s",first,second);
  }
  if(available&&has_current)temperature(actual_text,sizeof(actual_text),current,options.precision);
  out.number=options.number!=ClimateCardNumber::ICON;
  const char *number=options.number==ClimateCardNumber::ACTUAL?actual_text:target_text;
  if(out.number){std::strcpy(out.value,number);if(std::strcmp(number,"--")!=0)out.unit=unit;}
  const auto translated=[translate](const char *key){const char *text=translate?translate(key):key;return text?text:key;};
  if(options.label==ClimateCardLabel::LABEL) {
    if(configured_label.empty())copy_display_text(out.label,sizeof(out.label),translated("Climate"));
    else out.configured_label=configured_label;
  }
  else if(options.label==ClimateCardLabel::STATUS) {
    const auto key=status_key(climate::status(available,hvac_mode,hvac_action),hvac_mode);
    if(key)copy_display_text(out.label,sizeof(out.label),translated(key));
    else {
      char text[STATE_LIMIT+1];std::memcpy(text,hvac_mode.data(),hvac_mode.size());text[hvac_mode.size()]='\0';
      for(size_t i=0;i<hvac_mode.size();++i)if(std::isspace(static_cast<unsigned char>(text[i])))text[i]='_';
      char fallback[257];format_sensor_text(fallback,sizeof(fallback),text);
      copy_display_text(out.label,sizeof(out.label),translated(fallback));
    }
  } else {
    const char *label=options.label==ClimateCardLabel::ACTUAL?actual_text:target_text;
    const size_t used=copy_display_text(out.label,sizeof(out.label),label).bytes;
    if(std::strcmp(label,"--")!=0)copy_display_text(out.label+used,sizeof(out.label)-used,unit);
  }
  return out;
}
}  // namespace espcontrol::pages
