#!/usr/bin/env python3
"""Exercise light-temperature state against the production report and input callbacks."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_sliders.h').read_text()
def function(name):
    result = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(result) == 1, name
    return result[0]
reference = r'''
struct SliderCtx {
 lv_obj_t* fill=nullptr,*text_lbl=nullptr;
 int kelvin_min=2000,kelvin_max=6500,light_temp_last_kelvin=2000;
 bool available=true,light_on=false,light_state_known=false,light_temp_has_kelvin=false;
 bool light_temp_dragging=false,kelvin_color=true;
 int radius=0;
 std::string entity_id="light.room",cached_label="Room";
};
struct LegacyBinding {std::string attribute;std::function<void(esphome::StringRef)> callback;};
std::vector<LegacyBinding> legacy_bindings;
void ha_subscribe_attribute(const std::string&,const std::string& attribute,std::function<void(esphome::StringRef)> callback) {legacy_bindings.push_back({attribute,std::move(callback)});}
void ha_subscribe_state(const std::string&,std::function<void(esphome::StringRef)> callback) {legacy_bindings.push_back({"",std::move(callback)});}
void slider_update_fill(lv_obj_t* fill,lv_obj_t*,int percent,bool,bool,int) {fill->value=percent;}
int kelvin_to_fill_color(int kelvin,int,int) {return kelvin;}
bool slider_apply_vertical_pointer_value(lv_obj_t*) {return false;}
int legacy_target=-1;
void send_light_temp_action(const std::string&,int percent,int minimum,int maximum) {legacy_target=minimum+percent*(maximum-minimum)/100;}
'''
for name in ['light_temp_rounded_kelvin', 'light_temp_show_drag_kelvin', 'light_temp_restore_label',
             'light_temp_apply_kelvin_state', 'subscribe_light_temp_state']:
    reference += '\n' + function(name)
setup = function('setup_light_temp_visual')
for name, event in [('legacy_drag_event', 'VALUE_CHANGED'), ('legacy_release_event', 'RELEASED')]:
    callbacks = re.findall(r'lv_obj_add_event_cb\(slider, (\[\]\(lv_event_t \*e\) \{.*?\}), LV_EVENT_' + event + r', nullptr\);', setup, re.S)
    assert len(callbacks) == 1, event
    # Restrict each match to its final lambda, rather than preceding registrations.
    callback = callbacks[0][callbacks[0].rfind('[](lv_event_t *e) {'):]
    reference += '\nconst auto ' + name + ' = ' + callback + ';\n'
text = Path(__file__).with_name('page_light_temperature_runtime_test.cpp.in').read_text()
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(text.replace('// @legacy-light-temperature-reference', reference))
