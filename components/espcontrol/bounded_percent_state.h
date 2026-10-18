#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string_view>
namespace espcontrol::pages {
// Packed state has no terminator. Keep the existing numeric-prefix policy, but
// reject oversized/non-finite input and clamp before converting to an integer.
inline bool parse_bounded_percent(std::string_view text,uint8_t &percent) {
  char input[65];if(text.empty()||text.size()>=sizeof(input))return false;
  std::memcpy(input,text.data(),text.size());input[text.size()]='\0';char *end=nullptr;
  const float value=std::strtof(input,&end);if(end==input||!std::isfinite(value))return false;
  percent=static_cast<uint8_t>(std::max(0.0f,std::min(100.0f,value))+0.5f);return true;
}
}  // namespace espcontrol::pages
