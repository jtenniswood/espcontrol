#!/usr/bin/env python3
"""Check bounded playback timing against the current production helpers."""
import os
import shlex
import subprocess
import tempfile
from pathlib import Path
from generate_climate_update_test import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
sliders = (root / 'components/espcontrol/button_grid_sliders.h').read_text()
helpers = '\n'.join(definition(media, name) for name in [
    'media_parse_fixed_int', 'media_days_from_civil', 'media_parse_ha_timestamp',
    'media_position_timestamp_ms', 'media_playback_current_position_seconds',
]).replace('std::time(nullptr)', 'reference_epoch')
helpers += '\n' + definition(sliders, 'media_format_time')
source = r'''
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <string>
#include <vector>
#include "button_grid_string.h"
#include "media_playback_clock.h"
static size_t allocations=0;
void *operator new(size_t size){++allocations;if(void *p=std::malloc(size?size:1))return p;std::abort();}
void *operator new[](size_t size){return ::operator new(size);}
void operator delete(void *p)noexcept{std::free(p);}
void operator delete[](void *p)noexcept{std::free(p);}
void operator delete(void *p,std::size_t)noexcept{std::free(p);}
void operator delete[](void *p,std::size_t)noexcept{std::free(p);}
static uint32_t reference_ms=1000000;
static time_t reference_epoch=1800000000;
namespace esphome {uint32_t millis(){return reference_ms;}}
struct MediaPlaybackState {
 bool has_position=false,playing=false,has_duration=false;
 float position_seconds=0,duration=0;uint32_t position_updated_ms=0;
};
''' + helpers + r'''
void timestamp(const std::string &text) {
 time_t old_epoch=0;int64_t new_epoch=0;
 const bool old_valid=media_parse_ha_timestamp(esphome::StringRef(text),old_epoch);
 const auto before=allocations;
 const bool new_valid=espcontrol::media::parse_timestamp(text,new_epoch);
 assert(old_valid==new_valid&&(!old_valid||old_epoch==new_epoch));assert(allocations==before);
 uint32_t old_ms=0,new_ms=0;
 const bool old_time=media_position_timestamp_ms(esphome::StringRef(text),old_ms);
 const auto clock_before=allocations;
 const bool new_time=espcontrol::media::timestamp_milliseconds(text,reference_epoch,reference_ms,new_ms);
 assert(old_time==new_time&&(!old_time||old_ms==new_ms));assert(allocations==clock_before);
}
int main() {
 for(const auto *value:{"","bad","1970-01-01T00:00:01Z","2026-10-17T12:34:56Z",
     "2026-10-17T12:34:56.123456+01:00","2026-10-17 12:34:56-03:30",
     "2026-10-17T12:34:56+24:00","2026-02-31T00:00:00Z","2026-10-17T12x34x56Z",
     "2026-10-17T12:34:56.12345678901234567890+01:00","9999-12-31T23:59:60Z"}) timestamp(value);
 uint32_t random=47;auto next=[&](){random=random*1664525+1013904223;return random;};
 for(size_t i=0;i<10000;++i) {
  char date[64];std::snprintf(date,sizeof(date),"%04u-%02u-%02uT%02u:%02u:%02u%c%02u:%02u",1970+next()%8030,1+next()%12,1+next()%31,next()%24,next()%60,next()%61,i%2?'+':'-',next()%25,next()%61);
  reference_ms=next();timestamp(date);
  MediaPlaybackState old;old.has_position=next()%2;old.playing=next()%2;old.has_duration=next()%2;
  old.position_seconds=(next()%10000000)/10.0f;old.duration=old.has_duration?(1+next()%10000000)/10.0f:0;old.position_updated_ms=1+next()%1000000;reference_ms=1000000;
  espcontrol::media::PlaybackClock clock;clock.has_position=old.has_position;clock.playing=old.playing;clock.has_duration=old.has_duration;clock.duration=old.duration;clock.position=old.position_seconds;clock.updated_ms=old.position_updated_ms;clock.anchored=true;
  assert(clock.seconds(reference_ms)==media_playback_current_position_seconds(&old));
  char old_text[16],new_text[16];media_format_time(old.position_seconds,old_text,sizeof(old_text));
  const auto before=allocations;espcontrol::media::format_playback_time(old.position_seconds,new_text,sizeof(new_text));assert(std::strcmp(old_text,new_text)==0&&allocations==before);
 }
}
'''
with tempfile.TemporaryDirectory(prefix='espcontrol-media-clock-') as directory:
    unit = Path(directory) / 'test.cpp'
    executable = Path(directory) / 'test'
    unit.write_text(source)
    subprocess.run(shlex.split(os.environ.get('CXX', 'clang++')) + [
        '-std=c++17', '-Wall', '-Wextra', '-Werror',
        '-I' + str(root / 'tests/firmware/stubs'), '-I' + str(root / 'components/espcontrol'),
        str(unit), '-o', str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
print('Playback clock matches production timestamp/format/progress helpers without C++ allocations.')
