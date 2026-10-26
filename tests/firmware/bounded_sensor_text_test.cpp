#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "bounded_sensor_text.h"
#include "button_grid_subpage_parser.h"

static size_t allocations=0;
void *operator new(size_t bytes){++allocations;if(void *p=std::malloc(bytes?bytes:1))return p;std::abort();}
void *operator new[](size_t bytes){return ::operator new(bytes);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
int main() {
  std::vector<std::string>values={"", "pre-wash", "pre_wash", " \r\nfirst__line\n\nsecond-line \r\n", "a  b", "__a--b", "Unknown", "unavailable", "—test",u8"Übermorgen: Müll",u8"ätest ötest ütest ætest",u8"öätest",u8"ÜBERMORGEN",u8"čESKÝ",u8"ßtest ÿtest",u8"Приветtest",u8"Σtest 日本語test",u8"Ṣade ṣ",u8"\U000F0493 button",std::string(300,'a')};
  const std::array<std::string,12>fragments={"a","B","_","-"," ","\r","\n",u8"ä",u8"—",u8"日",u8"Пр",u8"Σ"};
  uint32_t seed=0x12345678;
  for(size_t i=0;i<1000;++i){std::string value;for(size_t part=0;part<20;++part){seed=seed*1664525+1013904223;value+=fragments[seed%fragments.size()];}values.push_back(value);}
  char output[257];
  for(const auto &input:values)for(size_t limit=0;limit<std::min<size_t>(input.size()+2,257);++limit) {
    const auto expected=text_sensor_display_text(esphome::StringRef(input),limit);
    const auto bytes=espcontrol::pages::format_sensor_text(output,sizeof(output),input,limit);
    assert(bytes==expected.size()&&std::string_view(output,bytes)==expected);
  }
  const std::string long_value=u8"Ätest__BÜRO-ÉTAGE 日本語";const auto before=allocations;
  for(size_t iteration=0;iteration<10000;++iteration)assert(espcontrol::pages::format_sensor_text(output,sizeof(output),long_value)>0);
  assert(allocations==before);
  for(const auto &point:{std::string(u8"ä"),std::string(u8"日"),std::string(u8"𐐀")}) {
    for(size_t cap=1;cap<=point.size();++cap){assert(espcontrol::pages::format_sensor_text(output,cap,point)==0);assert(output[0]=='\0');}
  }
  const char exact[]={'p','r','e','_','w','a','s','h','X'};
  assert(espcontrol::pages::format_sensor_text(output,sizeof(output),{exact,8})==8&&std::strcmp(output,"Pre Wash")==0);
  assert(espcontrol::pages::format_sensor_text(output,sizeof(output),"a\xFF" "b")==2&&std::strcmp(output,"Ab")==0);
}
