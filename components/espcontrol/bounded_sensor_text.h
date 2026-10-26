#pragma once

#include <algorithm>
#include <string_view>

#include "display_utf8_case.h"

namespace espcontrol::pages {
// Match text_sensor_display_text's title/whitespace rules using caller-owned
// storage. Each codepoint is appended atomically so the byte cap cannot leave
// a partial UTF-8 character. Configuration/state identity is untouched.
inline size_t format_sensor_text(char *output,size_t capacity,std::string_view input,size_t byte_limit=256) {
  if(!output||capacity==0)return 0;
  input=input.substr(0,std::min(input.size(),byte_limit));size_t used=0;
  bool cap_next=true,last_space=false;
  auto append=[&](char ch){if(used>=capacity-1)return false;output[used++]=ch;return true;};
  for(size_t offset=0;offset<input.size();) {
    const char ch=input[offset];const auto c=static_cast<unsigned char>(ch);
    if(ch=='\r'||ch=='\n') {
      if(ch=='\r'&&offset+1<input.size()&&input[offset+1]=='\n'){++offset;continue;}
      if(used&&output[used-1]==' ')--used;
      if(used&&output[used-1]!='\n'&&!append('\n'))break;
      cap_next=true;last_space=false;++offset;continue;
    }
    if(ch=='-'&&used&&output[used-1]!='\n'&&output[used-1]!=' ') {
      if(!append(ch))break;
      cap_next=true;last_space=false;++offset;continue;
    }
    if(ch=='_'||(c<0x80&&std::isspace(c))) {
      if(used&&!last_space&&output[used-1]!='\n'){if(!append(' '))break;last_space=true;}
      cap_next=true;++offset;continue;
    }
    uint32_t point=0;size_t length=0;const auto status=decode_utf8_codepoint(input,offset,point,length);
    if(status==Utf8DecodeStatus::INCOMPLETE)break;
    if(status==Utf8DecodeStatus::INVALID||point==0){offset+=length?length:1;continue;}
    if(utf8_letter_like(point)) {
      point=point<0x80?static_cast<uint32_t>(cap_next?std::toupper(static_cast<unsigned char>(point)):std::tolower(static_cast<unsigned char>(point))):utf8_latin_case(point,cap_next);
      cap_next=false;
    }
    char bytes[4];size_t count=0;
    if(point<=0x7F)bytes[count++]=static_cast<char>(point);
    else if(point<=0x7FF){bytes[count++]=static_cast<char>(0xC0|(point>>6));bytes[count++]=static_cast<char>(0x80|(point&0x3F));}
    else if(point<=0xFFFF){bytes[count++]=static_cast<char>(0xE0|(point>>12));bytes[count++]=static_cast<char>(0x80|((point>>6)&0x3F));bytes[count++]=static_cast<char>(0x80|(point&0x3F));}
    else {bytes[count++]=static_cast<char>(0xF0|(point>>18));bytes[count++]=static_cast<char>(0x80|((point>>12)&0x3F));bytes[count++]=static_cast<char>(0x80|((point>>6)&0x3F));bytes[count++]=static_cast<char>(0x80|(point&0x3F));}
    if(count>capacity-used-1)break;
    for(size_t index=0;index<count;++index)output[used++]=bytes[index];
    offset+=length;last_space=false;
  }
  while(used&&(output[used-1]==' '||output[used-1]=='\n'))--used;
  output[used]='\0';return used;
}
}  // namespace espcontrol::pages
