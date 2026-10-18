#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace espcontrol::pages {
struct DisplayTextResult { size_t bytes{0}; bool truncated{false}, invalid{false}; };

// A caller-owned, NUL-terminated buffer for LVGL's static-text API. Never split
// UTF-8 at the byte limit; apply the same S-with-dot-below display fallback as
// normalize_display_text without constructing a temporary std::string. When
// changed is supplied, the caller must initialize every byte of the buffer.
inline DisplayTextResult copy_display_text(char *output,size_t capacity,std::string_view input,
                                          size_t byte_limit=SIZE_MAX,bool *changed=nullptr) {
  DisplayTextResult result;
  if(changed)*changed=false;
  if(output==nullptr || capacity==0){result.truncated=!input.empty();return result;}
  const size_t limit=input.size()<byte_limit?input.size():byte_limit;
  result.truncated=limit<input.size();
  auto append=[&](char ch){if(changed&&output[result.bytes]!=ch)*changed=true;output[result.bytes++]=ch;};
  for(size_t offset=0;offset<limit;) {
    const auto lead=static_cast<unsigned char>(input[offset]);
    size_t length=lead<0x80?1:lead>=0xC2&&lead<=0xDF?2:lead>=0xE0&&lead<=0xEF?3:lead>=0xF0&&lead<=0xF4?4:0;
    if(length==0 || lead==0){result.invalid=true;++offset;continue;}
    if(length>limit-offset){result.truncated=true;break;}
    uint32_t point=length==1?lead:lead&((1u<<(7-length))-1u);
    bool valid=true;
    for(size_t index=1;index<length;++index) {
      const auto next=static_cast<unsigned char>(input[offset+index]);
      if((next&0xC0)!=0x80){valid=false;break;}
      point=(point<<6)|(next&0x3F);
    }
    if(!valid || (length==2&&point<0x80) || (length==3&&point<0x800) ||
       (length==4&&point<0x10000) || point>0x10FFFF || (point>=0xD800&&point<=0xDFFF)) {
      result.invalid=true;++offset;continue;
    }
    const bool fallback=point==0x1E62 || point==0x1E63;
    const size_t bytes=fallback?1:length;
    if(bytes>=capacity-result.bytes){result.truncated=true;break;}
    if(fallback)append(point==0x1E62?'S':'s');
    else for(size_t index=0;index<length;++index)append(input[offset+index]);
    offset+=length;
  }
  if(changed&&output[result.bytes]!='\0')*changed=true;
  output[result.bytes]='\0';return result;
}
}  // namespace espcontrol::pages
