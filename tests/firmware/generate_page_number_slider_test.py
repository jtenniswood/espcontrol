#!/usr/bin/env python3
"""Compare numeric state ownership against production slider subscriptions."""
import argparse
import re
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
source=(root/'components/espcontrol/button_grid_sliders.h').read_text()
def function(name):
    result=re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}',source,re.M|re.S)
    assert len(result)==1,name
    return result[0]
reference=r'''
#define ESP_LOGI(...)
#define ESP_LOGW(...)
struct SliderCtx {
 std::string entity_id,numeric_unit;
 lv_obj_t* fill=nullptr,*icon_lbl=nullptr,*sensor_container=nullptr,*sensor_lbl=nullptr,*unit_lbl=nullptr;
 bool horizontal=false,inverted=false,logged_level=false,logged_state=false,numeric=true,interactive=true;
 bool available=false,numeric_state_known=false,numeric_dragging=false,numeric_metadata_ready=false;
 bool numeric_min_known=false,numeric_max_known=false,numeric_step_known=false;
 double numeric_state=0,numeric_min=0,numeric_max=0,numeric_step=0;
 lv_coord_t radius=0;
};
struct LegacyBinding{std::string attribute;std::function<void(esphome::StringRef)> callback;};
std::vector<LegacyBinding> legacy_bindings;
void ha_subscribe_attribute(const std::string&,const std::string& attr,std::function<void(esphome::StringRef)> callback){legacy_bindings.push_back({attr,std::move(callback)});}
void ha_subscribe_state(const std::string&,std::function<void(esphome::StringRef)> callback){legacy_bindings.push_back({"",std::move(callback)});}
bool is_cover_entity(const std::string& entity){return entity.rfind("cover.",0)==0;}
bool is_fan_entity(const std::string& entity){return entity.rfind("fan.",0)==0;}
void slider_update_fill(lv_obj_t* fill,lv_obj_t*,int value,bool,bool,lv_coord_t){if(fill)fill->value=value;}
void slider_update_ctx_fill(SliderCtx* context,lv_obj_t*,int value){context->fill->value=value;}
'''
for name in ['slider_numeric_metadata','slider_position_percent','slider_clamp_pct','slider_parse_pct',
             'slider_parse_light_brightness_pct','slider_attribute_missing_ref','slider_entity_is_light',
             'slider_set_value_safe','slider_set_icon_safe','slider_numeric_restore_visual',
             'slider_numeric_show_value','slider_numeric_apply_state','slider_numeric_update_interaction',
             'slider_numeric_cancel_drag','slider_numeric_refresh_metadata','slider_numeric_apply_metadata_value',
             'subscribe_slider_state']:
    reference+='\n'+function(name)
reference+='\n#undef ESP_LOGI\n#undef ESP_LOGW\n'
text=Path(__file__).with_name('page_number_slider_runtime_test.cpp.in').read_text()
args.output.write_text(text.replace('// @legacy-number-slider-reference',reference))
