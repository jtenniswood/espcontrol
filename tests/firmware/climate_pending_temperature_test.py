"""Exercise ordered HA echoes and pending edits in the production Climate path."""
from pathlib import Path
import os
import re
import runpy
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=runpy.run_path(str(ROOT/"scripts/generate_climate_subscription_runtime_test.py"))["generate"]()
source=source[:source.index("int main(")]
source=source.replace("struct lv_timer_t { void (*callback)(lv_timer_t *); };", "struct lv_timer_t { void (*callback)(lv_timer_t *); void *data; };")
source=source.replace("uint32_t delay, void *) {", "uint32_t delay, void *data) {").replace("assert(delay == 250);", "assert(delay == 250 || delay == 450);").replace("new lv_timer_t{callback}","new lv_timer_t{callback,data}")
source=source.replace("void climate_cancel_temperature_send(ClimateControlCtx *) {}", "void climate_cancel_temperature_send(ClimateControlCtx *);")
source=source.replace("struct ModalUi { ClimateControlCtx *active = nullptr; } modal;", "struct ModalUi { ClimateControlCtx *active = nullptr; bool dragging_arc=false; } modal;")
source+=r'''
using ClimateControlModalUi=ModalUi;
void*lv_timer_get_user_data(lv_timer_t*t){return t->data;}
void lv_timer_pause(lv_timer_t*){}void lv_timer_resume(lv_timer_t*){}void lv_timer_reset(lv_timer_t*){}void lv_timer_set_period(lv_timer_t*,uint32_t){}
constexpr uint32_t CLIMATE_TEMP_DEBOUNCE_MS=450;
auto&ha_read_coordinator(){return coordinator;}
bool accept=true;
std::vector<std::vector<std::pair<const char*,std::string>>> commands;
bool climate_send_action(const std::string&,const char*,const std::vector<std::pair<const char*,std::string>>&data){if(!accept)return false;commands.push_back(data);return true;}
int climate_round_to_step(ClimateControlCtx*,int v){return v;}int climate_constrain_selected_target(ClimateControlCtx*,int v){return v;}int climate_clamp_tenths(ClimateControlCtx*,int v){return v;}
'''
text=(ROOT/"components/espcontrol/button_grid_climate.h").read_text()
for name in ["climate_range_target","climate_modal_temperature_controls_enabled","climate_service_temp_value","climate_cancel_temperature_send","climate_send_temperature","climate_debounce_timer_cb","climate_schedule_temperature_send","climate_apply_selected_target"]:
    matches=re.findall(r"^inline [^\n]*\b"+name+r"\([^;{]*\)\s*\{.*?^}",text,re.M|re.S)
    assert len(matches)==1,name
    source+=matches[0]+"\n"
source+=r'''
int main(){
 auto*ctx=add_card("climate.room","temperature");auto&t=coordinator.transport();
 auto publish=[&](const char*key,const char*value){t.publish("climate.room",key,value);};
 auto fire=[&](){ctx->debounce_timer->callback(ctx->debounce_timer);};
 publish("","heat");publish("supported_features","1");publish("temperature","20");
 climate_apply_selected_target(ctx,210,false,true);fire();assert(commands.back()[0].second=="21.0");
 climate_apply_selected_target(ctx,220,false,true);publish("temperature","21");assert(ctx->target_tenths==220);fire();assert(commands.back()[0].second=="22.0");
 climate_apply_selected_target(ctx,230,false,true);publish("temperature","22");climate_apply_selected_target(ctx,ctx->target_tenths+10,false,true);fire();assert(commands.back()[0].second=="24.0");
 // A refused send restores the latest authoritative report.
 climate_apply_selected_target(ctx,250,false,true);accept=false;fire();assert(!ctx->pending_temp_send&&ctx->target_tenths==220);accept=true;
 // Disconnect or a whole disconnect/reconnect cycle retires pending work.
 auto count=commands.size();climate_apply_selected_target(ctx,250,false,true);t.connected=false;fire();assert(commands.size()==count&&!ctx->pending_temp_send);t.connected=true;
 climate_apply_selected_target(ctx,250,false,true);coordinator.invalidate_retained_state();fire();assert(commands.size()==count&&!ctx->pending_temp_send);
 publish("target_temp_low","18");publish("target_temp_high","24");publish("supported_features","2");
 ctx->edit_high=false;climate_apply_selected_target(ctx,190,false,true);publish("target_temp_low","18");publish("target_temp_high","24");fire();assert(commands.back()[0].second=="19.0"&&commands.back()[1].second=="24.0");
 ctx->edit_high=true;climate_apply_selected_target(ctx,260,false,true);publish("target_temp_high","24");fire();assert(commands.back()[1].second=="26.0");
 // Changes to an untouched endpoint must accompany the user's edit.
 ctx->edit_high=false;climate_apply_selected_target(ctx,200,false,true);publish("target_temp_low","19");publish("target_temp_high","27");fire();assert(commands.back()[0].second=="20.0"&&commands.back()[1].second=="27.0");
 ctx->edit_high=true;climate_apply_selected_target(ctx,290,false,true);publish("target_temp_high","27");publish("target_temp_low","21");fire();assert(commands.back()[0].second=="21.0"&&commands.back()[1].second=="29.0");
 // If both endpoints were edited before sending, both edits are protected.
 ctx->edit_high=false;climate_apply_selected_target(ctx,220,false,true);ctx->edit_high=true;climate_apply_selected_target(ctx,300,false,true);publish("target_temp_low","21");publish("target_temp_high","29");fire();assert(commands.back()[0].second=="22.0"&&commands.back()[1].second=="30.0");
 assert(!ctx->pending_low_edit&&!ctx->pending_high_edit);
 climate_apply_selected_target(ctx,270,false,true);publish("","off");assert(!ctx->pending_temp_send);
 climate_apply_selected_target(ctx,270,false,true);publish("target_temp_high","unavailable");assert(!ctx->pending_temp_send);
 remove_card(ctx);assert(coordinator.subscription_count()==0);
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror","-Wno-unused-function","-Wno-unused-const-variable","-Wno-unused-private-field","-I"+str(ROOT/"components/espcontrol"),"-I"+str(ROOT/"tests/firmware/stubs"),str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Climate pending temperature regression passed.")
