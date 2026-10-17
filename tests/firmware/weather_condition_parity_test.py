#!/usr/bin/env python3
"""Compare the bounded weather presenter with production icon/caption helpers."""
from pathlib import Path
import os
import subprocess
import tempfile
from generate_climate_update_test import definition
ROOT=Path(__file__).resolve().parents[2]
subprocess.run(['python3',str(ROOT/'scripts/generate_weather_condition_maps.py'),'--check'],check=True)
legacy=(ROOT/'components/espcontrol/button_grid_weather_forecast.h').read_text()
# The ordinary 65-byte bank also covers a language switch without rebuilding.
import re
maps=(ROOT/'components/espcontrol/weather_condition_maps_generated.h').read_text()
keys=set(re.findall(r'\{\"[^\"]+\",\"[^\"]+\",\"([^\"]+)\"\}',maps))|{'Unknown','Unavailable'}
translations=(ROOT/'components/espcontrol/i18n_generated.h').read_text()
values=[value for key,value in re.findall(r'if \(std::strcmp\(text, \"([^\"]+)\"\) == 0\) return \"([^\"]*)\";',translations) if key in keys]
assert values and max(len(value.encode()) for value in values)<=64, 'Increase the fixed weather caption capacity for new translations.'

source=r'''#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <random>
#include "button_grid_subpage_parser.h"
#include "weather_condition.h"
size_t allocations=0;
void *operator new(size_t size){++allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}void operator delete[](void *p)noexcept{std::free(p);}
const char *find_icon(const char *name){return name;}
'''
for name in ['normalize_weather_state','weather_icon_for_state','weather_state_has_localized_label','weather_label_for_state']:
 source+=definition(legacy,name)
source+=r'''
void compare(const std::string &raw){const auto input=raw.substr(0,32);const auto icon=weather_icon_for_state(input);const auto label=weather_label_for_state(input);char text[257]{};const auto before=allocations;const auto actual=espcontrol::weather::condition_text(input,text,sizeof(text));assert(allocations==before);assert(std::string_view(icon)==espcontrol::weather::CONDITION_ROWS[actual.index].icon);if(std::string_view(actual.label?actual.label:text)!=label){std::fprintf(stderr,"Mismatch [%s]: expected [%s], got [%s]\\n",input.c_str(),label.c_str(),actual.label?actual.label:text);std::abort();}}
int main(){
 for(const auto &condition:espcontrol::weather::CONDITION_ROWS){compare(std::string(condition.state));compare("  "+std::string(condition.state)+"  ");compare("mdi-weather-"+std::string(condition.state));}
 for(const auto &alias:espcontrol::weather::CONDITION_ALIASES){compare(std::string(alias.from));compare("weather-"+std::string(alias.from));}
 for(const auto input:{"","unknown","unavailable","Custom_state","NIGHT_CLEAR","SUNNY",u8"état spécial",u8"Ṣade ṣ", "custom\nstate","custom--state", " mdi:weather-sunny ", "??? "})compare(input);
 std::mt19937 random(1825);const std::string alphabet="abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRSTUVWXYZ_-:./!?0123456789\t\n";
 for(int n=0;n<10000;++n){std::string text;for(size_t j=0,count=random()%65;j<count;++j)text.push_back(alphabet[random()%alphabet.size()]);compare(text);}
}
'''
with tempfile.TemporaryDirectory(prefix='weather-condition-') as folder:
 cpp=Path(folder)/'test.cpp';exe=Path(folder)/'test';cpp.write_text(source)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-UNDEBUG','-I',str(ROOT/'tests/firmware/stubs'),'-I',str(ROOT/'components/espcontrol'),str(cpp),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
print('Weather condition production parity and zero-allocation checks passed.')
