#!/usr/bin/env python3
"""Compile the production forecast widget view as a native pixel reference."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path);args=parser.parse_args()
root=Path(__file__).resolve().parents[2];source=(root/'components/espcontrol/button_grid_weather_daily_ui.h').read_text();data=(root/'components/espcontrol/button_grid_weather_daily_data.h').read_text()
text='''#pragma once
namespace forecast_reference {
static int32_t scale_x=256,scale_y=256;
static const char *(*translate)(const char *)=nullptr;
static float (*convert)(float,const char *)=nullptr;
struct Theme {uint32_t text_primary=0xFFFFFF;};
inline Theme current_theme(){return {};}
inline std::string espcontrol_i18n(const std::string &key){const auto *value=translate?translate(key.c_str()):nullptr;return value?value:key;}
inline float convert_temperature_value_for_display_float(float value,const char *unit){return convert?convert(value,unit):value;}
inline void apply_text_width_compensation(lv_obj_t *object){lv_obj_set_style_transform_scale_x(object,scale_x,LV_PART_MAIN);lv_obj_set_style_transform_scale_y(object,scale_y,LV_PART_MAIN);}
inline void theme_set_content_foreground(lv_obj_t *){}
inline void lv_label_set_display_text(lv_obj_t *label,const char *text){char value[257];espcontrol::pages::copy_display_text(value,sizeof(value),text);lv_label_set_text(label,value);}
inline const char *find_icon(const char *name){return std::strcmp(name,"Weather Sunny")==0?"SUN":"I";}
inline const char *weather_icon_for_state(const std::string &state){return espcontrol::weather::CONDITION_ROWS[espcontrol::weather::condition_index(state)].state=="sunny"?"SUN":"I";}
using WeatherForecastView=espcontrol::weather::View;
'''
for name,original in [('WeatherDailySource',data),('WeatherDailyColumn',source),('WeatherDailyCard',source)]:
 match=re.search(r'^struct '+name+r' \{.*?\};',original,re.M|re.S);assert match,name;text+=match[0]+'\n'
for name in ['weather_daily_temperature','weather_daily_temperature_range','weather_daily_weekday_label','weather_daily_date_label','weather_daily_selected_day','weather_daily_label','weather_daily_minimum_width','weather_daily_day_gap','weather_daily_forecast_minimum_width','weather_daily_render_card']:
 text+=definition(source,name)+'\n'
text+='} // namespace forecast_reference\n';args.output.write_text(text)
