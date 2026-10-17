#pragma once
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string_view>
#include "weather_condition_maps_generated.h"
#include "bounded_sensor_text.h"
namespace espcontrol::weather {
inline std::string_view condition_trim(std::string_view text) {
  while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.front())))text.remove_prefix(1);
  while(!text.empty()&&std::isspace(static_cast<unsigned char>(text.back())))text.remove_suffix(1);return text;
}
inline bool condition_lower_equal(std::string_view text,std::string_view expected) {
  if(text.size()!=expected.size())return false;
  for(size_t i=0;i<text.size();++i)if(std::tolower(static_cast<unsigned char>(text[i]))!=expected[i])return false;return true;
}
inline uint8_t condition_index(std::string_view input) {
  char normalized[33];size_t used=0;bool dash=false;
  for(char ch:condition_trim(input.substr(0,32))) {
    const auto c=static_cast<unsigned char>(ch);
    if(std::isalnum(c)){normalized[used++]=static_cast<char>(std::tolower(c));dash=false;}
    else if(!dash){normalized[used++]='-';dash=true;}
  }
  while(used&&normalized[used-1]=='-')--used;
  std::string_view key(normalized,used);
  if(key.substr(0,4)=="mdi-")key.remove_prefix(4);
  if(key.substr(0,8)=="weather-")key.remove_prefix(8);
  for(const auto &alias:CONDITION_ALIASES)if(key==alias.from){key=alias.to;break;}
  if(key.empty())key="unavailable";
  for(size_t i=0;i<CONDITION_COUNT;++i)if(key==CONDITION_ROWS[i].state)return static_cast<uint8_t>(i);
  // The fallback icon is the same as the legacy helper's unknown condition.
  for(size_t i=0;i<CONDITION_COUNT;++i)if(CONDITION_ROWS[i].state=="cloudy-alert")return static_cast<uint8_t>(i);
  return 0;
}
struct ConditionText {uint8_t index{0};const char *label{nullptr};};
// A configured current-weather card receives at most the existing short-state
// cap. Caption storage is supplied by its reusable view, never a std::string.
inline ConditionText condition_text(std::string_view input,char *output,size_t capacity) {
  input=condition_trim(input.substr(0,32));ConditionText result{condition_index(input),nullptr};
  if(input.empty()||condition_lower_equal(input,"unavailable"))result.label="Unavailable";
  else if(condition_lower_equal(input,"unknown"))result.label="Unknown";
  else if(condition_lower_equal(input,CONDITION_ROWS[result.index].state))result.label=CONDITION_ROWS[result.index].label;
  else {
    char title[33];size_t used=0;for(char ch:input){const auto c=static_cast<unsigned char>(ch);title[used++]=(ch=='-'||(c<0x80&&std::isspace(c)))?'_':ch;}
    espcontrol::pages::format_sensor_text(output,capacity,{title,used});
  }
  return result;
}
}  // namespace espcontrol::weather
