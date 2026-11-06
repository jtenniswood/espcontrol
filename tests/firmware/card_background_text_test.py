"""Exercise shared image text rendering, nested ownership and slider contrast."""
from pathlib import Path
import json
import re
import subprocess
import sys
import tempfile
from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
headers = root / "components/espcontrol"
source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
using lv_color_t = uint32_t;
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *param; int code; };
using Callback = void (*)(lv_event_t *);
struct Binding { Callback cb; int code; void *data; };
struct lv_area_t { int x1=0,y1=0,x2=99,y2=39; };
struct lv_obj_t {
  bool label=false, hidden=false;
  int flags=0, opacity=178, font=12, align=0, letter_space=0, line_space=0, offset=0;
  uint32_t color=0xFFFFFF, background=0;
  std::string text;
  lv_area_t area;
  lv_obj_t *parent=nullptr;
  std::vector<lv_obj_t *> children;
  std::vector<Binding> events;
};
int lv_label_class;
constexpr int LV_DRAW_TASK_TYPE_LABEL=1, LV_EVENT_DRAW_TASK_ADDED=2, LV_EVENT_CHILD_CREATED=3;
constexpr int LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS=16, LV_PART_MAIN=0, LV_OPA_COVER=255, LV_OPA_80=204;
struct lv_layer_t { lv_area_t _clip_area; };
struct lv_draw_label_dsc_t {
  struct { lv_layer_t *layer; lv_obj_t *obj; } base;
  uint32_t color;
  int opa, text_local=0, font=12, align=0, letter_space=0, line_space=0, offset=0;
  const char *text;
};
struct lv_draw_task_t {
  int type=LV_DRAW_TASK_TYPE_LABEL;
  lv_area_t area, _real_area, clip_area;
  lv_draw_label_dsc_t descriptor;
  std::string owned_text;
  lv_draw_task_t *next=nullptr;
};
std::vector<std::unique_ptr<lv_draw_task_t>> tasks;
int lv_draw_task_get_type(lv_draw_task_t *task){return task->type;}
void *lv_draw_task_get_draw_dsc(lv_draw_task_t *task){return &task->descriptor;}
uint32_t lv_color_hex(uint32_t color){return color;}
uint32_t lv_color_to_int(uint32_t color){return color;}
bool lv_color_eq(uint32_t a,uint32_t b){return a==b;}
uint32_t display_text_color_for_bg(uint32_t color){return color==0xFFFFFF?0:0xFFFFFF;}
int lv_obj_get_width(lv_obj_t *obj){return obj->area.x2-obj->area.x1+1;}
int lv_obj_get_height(lv_obj_t *obj){return obj->area.y2-obj->area.y1+1;}
lv_obj_t *lv_obj_get_parent(lv_obj_t *obj){return obj->parent;}
uint32_t lv_obj_get_style_text_color(lv_obj_t *obj,int){return obj->color;}
uint32_t lv_obj_get_style_bg_color(lv_obj_t *obj,int){return obj->background;}
void lv_obj_get_coords(lv_obj_t *obj,lv_area_t *area){*area=obj->area;}
bool lv_area_intersect(lv_area_t *result,const lv_area_t *a,const lv_area_t *b){
 *result={std::max(a->x1,b->x1),std::max(a->y1,b->y1),std::min(a->x2,b->x2),std::min(a->y2,b->y2)};
 return result->x1<=result->x2 && result->y1<=result->y2;
}
void *lv_event_get_param(lv_event_t *event){return event->param;}
void *event_data=nullptr;
void *lv_event_get_user_data(lv_event_t *){return event_data;}
bool lv_obj_check_type(lv_obj_t *obj,int *){return obj->label;}
void lv_obj_add_flag(lv_obj_t *obj,int flag){obj->flags|=flag;}
void lv_obj_remove_event_cb(lv_obj_t *obj,Callback cb){auto &v=obj->events;v.erase(std::remove_if(v.begin(),v.end(),[cb](auto &e){return e.cb==cb;}),v.end());}
void lv_obj_add_event_cb(lv_obj_t *obj,Callback cb,int code,void *data){obj->events.push_back({cb,code,data});}
unsigned lv_obj_get_event_count(lv_obj_t *obj){return obj->events.size();}
Binding *lv_obj_get_event_dsc(lv_obj_t *obj,unsigned i){return &obj->events[i];}
Callback lv_event_dsc_get_cb(Binding *binding){return binding->cb;}
unsigned lv_obj_get_child_cnt(lv_obj_t *obj){return obj->children.size();}
lv_obj_t *lv_obj_get_child(lv_obj_t *obj,unsigned i){return obj->children[i];}
void emit(lv_obj_t *obj,int code,void *param){
 const auto events=obj->events;
 for(auto &binding:events)if(binding.code==code){event_data=binding.data;lv_event_t e{obj,param,code};binding.cb(&e);}
}
void attach(lv_obj_t *obj,lv_obj_t *parent,bool label){
 obj->parent=parent;obj->label=label;parent->children.push_back(obj);
 // Match LVGL: CHILD_CREATED always bubbles through nested containers.
 for(auto *p=parent;p;p=p->parent)emit(p,LV_EVENT_CHILD_CREATED,obj);
}
void lv_draw_label(lv_layer_t *layer,const lv_draw_label_dsc_t *descriptor,const lv_area_t *area){
 if(!descriptor->text || !*descriptor->text)return;
 auto task=std::make_unique<lv_draw_task_t>();
 task->descriptor=*descriptor;task->area=task->_real_area=*area;task->clip_area=layer->_clip_area;
 task->owned_text=descriptor->text;task->descriptor.text=task->owned_text.c_str();
 if(!tasks.empty())tasks.back()->next=task.get();
 tasks.push_back(std::move(task));
}
#include "card_background_text.h"
namespace espcontrol::card_background {
struct Ref {lv_obj_t *btn;};
struct CardBackgroundRuntimeState {std::vector<Ref> widget_refs;};
CardBackgroundRuntimeState runtime;
auto &refs=runtime.widget_refs;
}
namespace espcontrol {
bool assets_live=true, runtime_live=true;
struct Assets {
 template<typename T> const T *card_background_runtime_if_present() const {
  return runtime_live?&card_background::runtime:nullptr;
 }
} assets;
Assets *card_asset_service(){return assets_live?&assets:nullptr;}
}
'''
for filename, namespace, names in [
    ("button_grid_slider_contrast.h", "", ("slider_text_contrast_event_cb", "slider_bind_text_contrast")),
    ("card_background_rendering.cpp", "espcontrol::card_background", ("card_background_text_on_image", "card_background_bind_text_tree", "card_background_text_child_created", "card_background_unbind_text_tree")),
]:
    if namespace:
        source += "\nnamespace " + namespace + " {\n"
    text = (headers / filename).read_text()
    if filename.endswith(".cpp"):
        text = re.sub(r"^(bool|void) (card_background_)", r"inline \1 \2", text, flags=re.M)
    for name in names:
        line, code = definition(text, name)
        source += f'\n#line {line} "{filename}"\n{code}\n'
    if namespace:
        source += "\n}\n"
source += r'''
using namespace espcontrol::card_background;
void enable(lv_obj_t *owner){
 refs={{owner}};card_background_text_query()=card_background_text_on_image;
 card_background_bind_text_tree(owner);
 lv_obj_remove_event_cb(owner,card_background_text_child_created);
 lv_obj_add_event_cb(owner,card_background_text_child_created,LV_EVENT_CHILD_CREATED,nullptr);
}
void render(lv_obj_t *label){
 tasks.clear();
 for(auto *p=label;p;p=p->parent)if(p->hidden)return;
 if(label->text.empty())return;
 static lv_layer_t layer{{0,0,999,999}};
 lv_draw_label_dsc_t descriptor{{&layer,label},label->color,label->opacity,0,label->font,label->align,label->letter_space,label->line_space,label->offset,label->text.c_str()};
 lv_draw_label(&layer,&descriptor,&label->area);
 emit(label,LV_EVENT_DRAW_TASK_ADDED,tasks.front().get());
}
void check_pair(lv_obj_t *label){
 render(label);assert(tasks.size()==2);
 const auto &shadow=*tasks[0],&foreground=*tasks[1];
 assert(shadow.owned_text==label->text && foreground.owned_text==label->text);
 assert(shadow.descriptor.color==0 && shadow.descriptor.opa==LV_OPA_80);
 assert(foreground.descriptor.color==0xFFFFFF && foreground.descriptor.opa==LV_OPA_COVER);
 assert(shadow.area.x1==label->area.x1+1 && shadow.area.y1==label->area.y1+1);
 assert(shadow._real_area.x2==shadow.area.x2 && shadow._real_area.y2==shadow.area.y2);
 assert(foreground.area.x1==label->area.x1 && foreground.area.y1==label->area.y1);
 assert(foreground.area.x2==label->area.x2 && foreground.area.y2==label->area.y2);
 for(const auto &t:tasks){
  assert(t->descriptor.font==label->font && t->descriptor.align==label->align);
  assert(t->descriptor.letter_space==label->letter_space && t->descriptor.line_space==label->line_space);
  assert(t->descriptor.offset==label->offset);
 }
 assert(label->opacity==178); // No style override to leak after image removal.
}
int main(){
'''
contract = (headers / "button_grid_contract_generated.h").read_text()
types = re.search(r"CARD_CONTRACT_BACKGROUND_IMAGE_TYPES\[\] = (\{.*?\});", contract).group(1)
source += "const std::vector<std::string> types=" + types + ";\n"
source += r'''
 for(const auto &type:types){
  (void)type;
  lv_obj_t owner,container,title,value,unit,late,icon;
  attach(&container,&owner,false);attach(&title,&owner,true);
  attach(&value,&container,true);attach(&unit,&container,true);attach(&icon,&owner,true);
  title.text="Loading";value.text="--";unit.text="°C";icon.text="weather-icon";
  enable(&owner);enable(&owner);
  assert(owner.events.size()==1 && title.events.size()==1 && value.events.size()==1);
  for(auto *label:{&title,&value,&unit,&icon})check_pair(label);
  // Direct text replacement, units, fonts, wrapping and scroll offsets all use
  // the live descriptor; no special text setter or per-card shadow update.
  title.text="Updated\nsecond line";value.text="11°/6°";unit.text="%";
  title.font=32;title.align=2;title.line_space=4;title.letter_space=1;title.offset=-17;
  title.color=0; // A saved pale card colour must not make image text black.
  title.area={10,12,219,80};
  for(auto *label:{&title,&value,&unit})check_pair(label);
  attach(&late,&container,true);late.text="Late forecast column";check_pair(&late);
  const auto children=container.children.size();
  for(int i=0;i<30;++i){value.text=std::to_string(i);check_pair(&value);}
  assert(container.children.size()==children && value.events.size()==1);
  value.text.clear();render(&value);assert(tasks.empty());
  value.text="Unavailable";value.hidden=true;render(&value);assert(tasks.empty());
  value.hidden=false;container.hidden=true;render(&value);assert(tasks.empty());
  container.hidden=false;check_pair(&value);
  refs.clear();card_background_unbind_text_tree(&owner);
  assert(owner.events.empty() && value.events.empty() && late.events.empty());
  render(&value);assert(tasks.size()==1 && tasks.front()->descriptor.opa==178);
  enable(&owner);espcontrol::assets_live=false;render(&value);
  assert(tasks.size()==1);espcontrol::assets_live=true;
  espcontrol::runtime_live=false;render(&value);assert(tasks.size()==1);espcontrol::runtime_live=true;
 }
 // Preserve the existing contrast on each side of a slider fill, even at 0%.
 for(bool horizontal:{false,true})for(int percent:{0,1,50,99,100}){
  lv_obj_t owner,fill,label;
  owner.color=0;
  owner.area={0,0,99,99};fill.area=horizontal?lv_area_t{0,0,percent-1,99}:lv_area_t{0,100-percent,99,99};
  fill.background=0xFFFFFF;fill.parent=&owner;
  attach(&label,&owner,true);label.area=owner.area;label.text="50 %";
  slider_bind_text_contrast(&label,&fill);enable(&owner);
  assert(label.events.size()==1); // No duplicate generic shadow callback.
  render(&label);
  assert(tasks.size()>=2 && tasks[0]->descriptor.opa==LV_OPA_80);
  for(int y=0;y<100;++y)for(int x=0;x<100;++x){
   int matches=0;
   const bool filled=x>=fill.area.x1 && x<=fill.area.x2 && y>=fill.area.y1 && y<=fill.area.y2;
   for(size_t i=1;i<tasks.size();++i){const auto &t=*tasks[i];const auto &c=t.clip_area;
    if(x>=c.x1 && x<=c.x2 && y>=c.y1 && y<=c.y2){
     ++matches;assert(t.descriptor.opa==LV_OPA_COVER && t.descriptor.color==(filled?0u:0xFFFFFFu));
    }
   }
   assert(matches==1);
  }
  slider_bind_text_contrast(&label,nullptr);assert(label.events.size()==1);check_pair(&label);
  refs.clear();card_background_unbind_text_tree(&owner);render(&label);assert(tasks.size()==1);
 }
 refs.clear();tasks.clear();
}
'''
with tempfile.TemporaryDirectory(prefix="espcontrol-image-text-") as directory:
    path = Path(directory)
    (path / "test.cpp").write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(headers), str(path / "test.cpp"), "-o", str(path / "test")], check=True)
    subprocess.run([str(path / "test")], check=True)
print(f"Image text rendering passed for {len(json.loads(types.replace('{','[').replace('}',']')))} supported card types: live text, secondary opacity, nested labels, visibility, layout, reuse and slider contrast.")
