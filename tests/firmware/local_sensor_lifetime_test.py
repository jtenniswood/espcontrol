"""Exercise local sensor detach/rebind and owner deletion using production code."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
local = (ROOT / "components/espcontrol/button_grid_local_controls.h").read_text()
local = local[local.index("struct LocalSensorControl {"):local.index("#ifdef USE_WEBSERVER")]
driver = (ROOT / "components/espcontrol/button_grid_sensor_driver.h").read_text()
def definition(name):
    match = re.search(r"^inline [^\n]*\b" + name + r"\([^;{]*\)\s*\{.*?^}", driver, re.M | re.S)
    assert match, name
    return match[0]
source = r'''#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
struct lv_event_t;
struct lv_obj_t { std::string text; std::vector<void(*)(lv_event_t*)> callbacks; };
struct lv_event_t { lv_obj_t *target; };
void *lv_event_get_target(lv_event_t *e) { return e->target; }
constexpr int LV_EVENT_DELETE=1;
void lv_obj_remove_event_cb(lv_obj_t *o,void(*cb)(lv_event_t*)) {
  o->callbacks.erase(std::remove(o->callbacks.begin(),o->callbacks.end(),cb),o->callbacks.end());
}
void lv_obj_add_event_cb(lv_obj_t *o,void(*cb)(lv_event_t*),int,void*) {o->callbacks.push_back(cb);}
void destroy(lv_obj_t *o) {lv_event_t e{o};auto callbacks=o->callbacks;for(auto cb:callbacks) cb(&e);delete o;}
void lv_label_set_display_text(lv_obj_t*o,const char*t){o->text=t;}
void set_wrapped_button_label_text(lv_obj_t*o,const std::string&t){o->text=t;}
struct BtnSlot {lv_obj_t *btn,*sensor_lbl,*text_lbl;};
struct ParsedCfg {std::string entity,precision;};
struct Context {};
bool sensor_driver_matches(const Context&){return false;}
''' + local + definition("sensor_driver_cleanup") + definition("sensor_driver_register_local_value") + r'''
int main(){
  auto *owner=new lv_obj_t;auto *value=new lv_obj_t;auto *text=new lv_obj_t;
  BtnSlot slot{owner,value,text};
  sensor_driver_register_local_value(slot,{"temperature","1"});
  sensor_driver_register_local_value(slot,{"temperature","1"});
  assert(owner->callbacks.size()==1 && local_sensor_registry().size()==1);
  assert(local_sensor_apply_value("temperature",21.5f) && value->text=="21.5");
  assert(!sensor_driver_cleanup(slot,{},{})); // New card is a different family.
  assert(!local_sensor_apply_value("temperature",22));
  sensor_driver_register_local_value(slot,{"status","text"});
  assert(local_sensor_apply_text("status","ready") && text->text=="ready");
  destroy(owner);delete value;delete text;
  assert(!local_sensor_apply_text("status","deleted"));
  assert(!local_sensor_apply_value("temperature",23));
  assert(local_sensor_registry().empty());
  // Reusing the same owner address after detach must not resurrect an old key.
  lv_obj_t reused,label;BtnSlot again{&reused,&label,&label};
  sensor_driver_register_local_value(again,{"old","0"});
  sensor_driver_cleanup(again,{},{});
  sensor_driver_register_local_value(again,{"new","0"});
  assert(!local_sensor_apply_value("old",1));assert(local_sensor_apply_value("new",2));
  assert(!local_sensor_callback_registered("temperature",false));
  assert(local_sensor_callback_registered("temperature",false));
  sensor_driver_cleanup(again,{},{});
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/"test.cpp";exe=Path(temp)/"test";cpp.write_text(source)
    subprocess.run([os.environ.get("CXX","c++"),"-std=c++17","-Wall","-Wextra","-Werror","-fsanitize=address",str(cpp),"-o",str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print("Local sensor lifetime regression passed (AddressSanitizer).")
