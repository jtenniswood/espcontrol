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
struct ClimateControlCtx {std::string entity_id="climate.room",hvac_mode,fan_mode,swing_mode,swing_horizontal_mode,preset_mode;bool edit_high=false;};
std::vector<std::pair<const char*,std::string>> sent;
void climate_send_action(const std::string&,const char*,const std::vector<std::pair<const char*,std::string>>&data){sent=data;}
void climate_select_target_for_mode(ClimateControlCtx*,const std::string&){}
void climate_update_card(ClimateControlCtx*){}void climate_control_set_modal_value(ClimateControlCtx*){}
'''
for name in ['climate_lower', 'climate_trim', 'climate_option_label', 'climate_option_choice_label', 'climate_clean_option_token', 'climate_hvac_service_value', 'climate_parse_options', 'climate_send_option', 'climate_option_current_value', 'climate_option_selected']:source+=definition(name)
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
