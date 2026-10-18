"""Check production option parsing and exact outgoing values."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
text=(ROOT / 'components/espcontrol/button_grid_option_select.h').read_text()
def definition(name):
    matches=re.findall(r"^inline [^\n]*\b"+name+r"\([^;{]*\)\s*\{.*?^}",text,re.M|re.S)
    assert len(matches)==1,name
    return matches[0]+"\n"
source = r'''#include <string>
#include <vector>
#include <map>
#include <functional>
#include <iostream>
#include <cassert>
#include <algorithm>
struct lv_obj_t{std::string label;};
namespace esphome {struct StringRef{std::string s;StringRef(const char*v):s(v){}bool empty()const{return s.empty();}};}
constexpr int OPTION_SELECT_MAX_OPTIONS=64;constexpr size_t OPTION_SELECT_OPTION_MAX_LEN=96, OPTION_SELECT_OPTIONS_TEXT_MAX_LEN=6400,HA_STATE_TEXT_MAX_LEN=256;
struct OptionSelectCtx{std::string entity_id,current_option;std::vector<std::string> options;bool available=true;unsigned accent_color=0;};
struct OptionSelectOptionClick{OptionSelectCtx*ctx=nullptr;std::string value;};
struct OptionSelectModalUi{OptionSelectCtx*active=nullptr;lv_obj_t*option_rows[64]={};OptionSelectOptionClick option_clicks[64];};
OptionSelectModalUi ui;OptionSelectModalUi&option_select_modal_ui(){return ui;}
std::map<std::string,std::function<void(esphome::StringRef)>> callbacks;
void ha_subscribe_state(const std::string&,std::function<void(esphome::StringRef)> cb){callbacks["state"]=cb;}
void ha_subscribe_attribute(const std::string&,const std::string&key,std::function<void(esphome::StringRef)> cb){callbacks[key]=cb;}
std::string string_ref_limited(esphome::StringRef s,size_t n){return s.s.substr(0,n);}
std::string normalized_state_text(esphome::StringRef s){return s.s;}
bool ha_entity_state_unavailable_ref(const std::string&,esphome::StringRef){return false;}
void option_select_apply_card_text(OptionSelectCtx*){}
void option_select_hide_modal(){ui.active=nullptr;}
std::string trim_display_unit(const std::string&s){auto a=s.find_first_not_of(" ");return a==std::string::npos?"":s.substr(a,s.find_last_not_of(" ")-a+1);}
constexpr int LV_PART_MAIN=0;
struct Theme{unsigned surface_primary=0,text_primary=0;};Theme theme;Theme&current_theme(){return theme;}
unsigned theme_display_color(unsigned c){return c;}unsigned lv_color_hex(unsigned c){return c;}unsigned readable_text_color_for_bg(unsigned c){return c;}
void lv_obj_set_style_bg_color(lv_obj_t*,unsigned,int){}void theme_set_content_background(lv_obj_t*,bool){}void apply_card_descendant_text_color(lv_obj_t*,unsigned){}
std::string sent;bool ha_send_entity_action(const std::string&,const char*,const char*,const char*v){sent=v;return true;}
'''
for name in ['option_select_trim', 'option_select_add_option', 'option_select_parse_delimited_options', 'option_select_parse_list_options', 'option_select_parse_options', 'option_select_refresh_modal_rows', 'subscribe_option_select_state']:source+=definition(name)
source += r'''int main(){
 OptionSelectCtx ctx;ctx.entity_id="input_select.mode";subscribe_option_select_state(&ctx);
 callbacks["options"]("['Home', 'Away']");ui.active=&ctx;
 callbacks["options"]("['Home', 'Away']");assert(ui.active==&ctx);
 callbacks["options"]("['Home', 'Holiday']");assert(ui.active==nullptr);assert(ctx.options[1]=="Holiday");
 ui.active=&ctx;callbacks["options"]("['Holiday', 'Home']");assert(ui.active==nullptr);
 ui.active=&ctx;callbacks["options"]("[]");assert(ui.active==nullptr&&ctx.options.empty());
 ui.active=&ctx;callbacks["options"]("['New']");assert(ui.active==nullptr&&ctx.options[0]=="New");
 OptionSelectCtx other;ui.active=&other;callbacks["options"]("['Other']");assert(ui.active==&other);
}'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("option_select_live_options regression passed.")
