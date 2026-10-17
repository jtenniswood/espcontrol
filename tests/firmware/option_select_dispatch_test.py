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
source='#include <cassert>\n#include <string>\n#include <vector>\n#include <algorithm>\n#include <cctype>\n#include <utility>\nstruct OptionSelectCtx{std::string entity_id,current_option;bool available=true;};\nbool connected=true,accept=true;int sends=0;\nbool ha_api_state_connected(){return connected;}\nbool ha_send_entity_action(const std::string&,const char*,const char*,const char*){++sends;return accept;}\n'
for name in ['option_select_entity_supported', 'option_select_service_for_entity', 'send_option_select_action', 'option_select_can_open_modal']:source+=definition(name)
source += r'''int main(){
 OptionSelectCtx ctx{"input_select.mode","Home",true};
 assert(option_select_can_open_modal(&ctx));
 accept=false;assert(!send_option_select_action(&ctx,"Away"));assert(ctx.current_option=="Home"&&sends==1);
 connected=false;assert(!option_select_can_open_modal(&ctx));assert(!send_option_select_action(&ctx,"Away"));assert(sends==1&&ctx.current_option=="Home");
 connected=true;accept=true;assert(send_option_select_action(&ctx,"Away"));assert(ctx.current_option=="Home");
 ctx.current_option="Away"; // authoritative subscription echo
 assert(ctx.current_option=="Away");
 ctx.available=false;assert(!send_option_select_action(&ctx,"Home"));
 ctx.available=true;ctx.entity_id="select.mode";assert(send_option_select_action(&ctx,"Home"));assert(ctx.current_option=="Away");
 ctx.entity_id="sensor.mode";assert(!send_option_select_action(&ctx,"Home"));
}'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("option_select_dispatch regression passed.")
