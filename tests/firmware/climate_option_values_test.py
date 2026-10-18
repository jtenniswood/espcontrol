"""Check production option parsing and exact outgoing values."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
text=(ROOT / 'components/espcontrol/button_grid_climate.h').read_text()
def definition(name):
    matches=re.findall(r"^inline [^\n]*\b"+name+r"\([^;{]*\)\s*\{.*?^}",text,re.M|re.S)
    assert len(matches)==1,name
    return matches[0]+"\n"
source = r'''#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <utility>
std::string espcontrol_i18n(const std::string&v){return v;}
std::string sentence_cap_text(std::string v){if(!v.empty())v[0]=std::toupper(static_cast<unsigned char>(v[0]));return v;}
namespace esphome {struct StringRef{std::string s;StringRef(const char*v):s(v){}};}
constexpr size_t HA_TEXT_SENSOR_STATE_MAX_LEN=4096;
std::string string_ref_limited(esphome::StringRef v,size_t n){return v.s.substr(0,n);}
struct lv_obj_t {};
struct lv_font_t {};
struct ClimateControlCtx {
  std::string entity_id="climate.room",hvac_mode,fan_mode,swing_mode,swing_horizontal_mode,preset_mode;
  std::vector<std::string> hvac_modes,fan_modes,swing_modes,swing_horizontal_modes,preset_modes;
  bool edit_high=false;
  uint32_t accent_color=0;
  lv_obj_t *btn=nullptr;
  const lv_font_t *card_icon_font=nullptr,*icon_font=nullptr,*option_menu_font=nullptr;
};
std::vector<std::pair<const char*,std::string>> sent;
void climate_send_action(const std::string&,const char*,const std::vector<std::pair<const char*,std::string>>&data){sent=data;}
void climate_select_target_for_mode(ClimateControlCtx*,const std::string&){}
void climate_update_card(ClimateControlCtx*){}void climate_control_set_modal_value(ClimateControlCtx*){}
'''
for name in ['climate_lower', 'climate_trim', 'climate_option_label', 'climate_option_choice_label', 'climate_clean_option_token', 'climate_hvac_service_value', 'climate_parse_options', 'climate_send_option', 'climate_option_current_value', 'climate_option_selected']:source+=definition(name)
# Compile and execute the complete production list renderer, including its
# section lambda. Only LVGL/theme boundaries are doubled, not the UI function.
inline_list = definition("climate_open_inline_option_list")
constants = sorted(set(re.findall(r"\bLV_[A-Z_]+\b", inline_list)))
source += "\n".join("constexpr int " + name + "=" + str(i) + ";" for i, name in enumerate(constants)) + "\n"
source += r'''
constexpr int CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_ICON_ZOOM=214,CLIMATE_OPTION_MAX_OPTIONS=32;
struct lv_event_t {void *data;};
struct ClimateOptionClick {ClimateControlCtx *ctx=nullptr;std::string kind,value;};
struct ClimateControlModalUi {lv_obj_t *panel=nullptr,*option_list_view=nullptr;int option_click_count=0;ClimateOptionClick option_clicks[CLIMATE_OPTION_MAX_OPTIONS];} ui;
ClimateControlModalUi &climate_control_modal_ui(){return ui;}
struct ControlModalLayout {};
ControlModalLayout climate_control_calc_layout(ClimateControlCtx*){return {};}
bool compact=false;
bool climate_control_uses_compact_portrait_modal_tuning(const ControlModalLayout&){return compact;}
std::vector<std::string> labels;
void climate_hide_inline_option_list(){labels.clear();}
void climate_control_layout_modal(ClimateControlCtx*){}
struct Theme {uint32_t surface_primary=0,text_primary=0;} theme;
Theme &current_theme(){return theme;}
uint32_t theme_display_color(uint32_t c){return c;}
uint32_t readable_text_color_for_bg(uint32_t c){return c;}
uint32_t lv_color_hex(uint32_t c){return c;}
void theme_set_content_background(lv_obj_t*,bool){}
int control_modal_card_radius(lv_obj_t*){return 0;}
void control_modal_apply_pressed_fill(lv_obj_t*){}
void apply_icon_width_compensation(lv_obj_t*,int){}
lv_obj_t widget;
lv_obj_t *lv_obj_create(lv_obj_t*){return &widget;}
lv_obj_t *lv_btn_create(lv_obj_t*){return &widget;}
lv_obj_t *lv_label_create(lv_obj_t*){return &widget;}
void lv_label_set_display_text(lv_obj_t*,const char*t){labels.emplace_back(t);}
void lv_label_set_long_mode(lv_obj_t*,int){}
int lv_pct(int v){return v;}
void *lv_event_get_user_data(lv_event_t*e){return e->data;}
void lv_obj_add_event_cb(lv_obj_t*,void(*)(lv_event_t*),int,void*){}
const char *climate_option_icon(const std::string&,const std::string&){return "icon";}
'''
# These are render-only LVGL calls. Keep the actual option iteration and data
# bindings compiled so an out-of-scope variable fails the test.
for name in sorted(set(re.findall(r"\blv_obj_\w+(?=\()", inline_list)) - {"lv_obj_create", "lv_obj_add_event_cb"}):
    source += "template<typename... Args> void " + name + "(Args...){}\n"
source += definition("climate_next_option_click") + inline_list
source += r'''int main(){
  auto values=climate_parse_options("['AUTO', 'ECO', 'eco', 'Sleep, Quiet', 'Profile: Night', '[Auto]', '  Guest  ', 'Été', 'Kid\\'s', 'C:\\\\Room']");
  const std::vector<std::string> expected={"AUTO","ECO","eco","Sleep, Quiet","Profile: Night","[Auto]","  Guest  ","Été","Kid's","C:\\Room"};
  assert(values==expected);
  assert(climate_option_choice_label("ECO",values)=="ECO");
  assert(climate_option_choice_label("eco",values)=="eco");
  const std::vector<std::string> autos={"Auto","auto","AUTO"};
  for(const auto&v:autos)assert(climate_option_choice_label(v,autos)==v);
  assert(climate_option_choice_label("auto",{"auto","low"})=="Auto");
  // Translations can collide too: preserve original choices in that case.
  assert(climate_option_choice_label("holduntil",{"holduntil","hold_until"})=="holduntil");
  ClimateControlCtx ctx;
  ui.panel=&widget;
  for(const bool layout:{false,true}){
    compact=layout;
    for(const std::string kind:{"preset","fan","swing","horizontal_swing"}){
      ctx.preset_modes=ctx.fan_modes=ctx.swing_modes=ctx.swing_horizontal_modes={"ECO","eco"};
      climate_open_inline_option_list(&ctx,kind);
      assert((labels==std::vector<std::string>{"icon","ECO","icon","eco"}));
      assert(ui.option_click_count==2);
      assert(ui.option_clicks[0].value=="ECO"&&ui.option_clicks[1].value=="eco");
    }
  }
  for(const char*kind:{"fan","swing","horizontal_swing","preset"})for(const auto&value:values){
    climate_send_option(&ctx,kind,value);assert(sent.size()==1&&sent[0].second==value);
    assert(climate_option_selected(&ctx,kind,value));
  }
  ctx.preset_mode="eco";assert(!climate_option_selected(&ctx,"preset","ECO"));
  assert(climate_parse_options("['unfinished]").empty());
  assert(climate_hvac_service_value("<HVACMode.HEAT: 'heat'>")=="heat");
  assert(climate_hvac_service_value("HEAT_COOL")=="heat_cool");
}'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("climate_option_values regression passed.")
