"""Run production image ownership functions against a checked LVGL buffer boundary."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
source = r'''
#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <vector>
constexpr int CARD_BACKGROUND_IMAGE_MAX_CONTEXTS=2, CARD_BACKGROUND_IMAGE_MAX_BINDINGS=2;
struct lv_obj_t { bool hidden=false; const void *source=nullptr; int width=80,height=80; };
std::vector<lv_obj_t *> widgets;
void image_card_clear_widget_source(lv_obj_t *widget) { if(widget){widget->source=nullptr;widget->hidden=true;} }
void lv_obj_move_background(lv_obj_t *) {}
int lv_obj_get_width(lv_obj_t *obj){return obj->width;}
int lv_obj_get_height(lv_obj_t *obj){return obj->height;}
struct Image {
 int width=80,height=80,releases=0,downloads=0;
 std::vector<std::function<void(bool)>> finished;
 std::vector<std::function<void()>> errors;
 void release(){for(auto *widget:widgets)assert(widget->source!=this);++releases;}
 int get_fixed_width(){return width;} int get_fixed_height(){return height;}
 void set_target_size(int w,int h){width=w;height=h;}
 void add_on_finished_callback(std::function<void(bool)> cb){finished.push_back(cb);}
 void add_on_error_callback(std::function<void()> cb){errors.push_back(cb);}
};
struct CardBackgroundImageCtx {
 Image *image=nullptr;std::string url;bool callbacks_bound=false;
 struct Binding {lv_obj_t *btn=nullptr,*widget=nullptr;bool active=false;} bindings[CARD_BACKGROUND_IMAGE_MAX_BINDINGS];
};
struct State {bool active=true,ready=true;std::string id="image";int width=80,height=80;};
struct Controller {int removals=0;void remove_binding(size_t){++removals;}} controller;
struct GridConfig {};
struct lv_timer_t {};
void lv_timer_del(lv_timer_t *) {}
struct CardBackgroundRuntimeState {
 std::shared_ptr<uint8_t> callback_lifetime=std::make_shared<uint8_t>(0);
 CardBackgroundImageCtx contexts[2];
 lv_timer_t *final_refresh_timer=nullptr,*cache_write_timer=nullptr,*next_download_timer=nullptr;
} runtime;
State states[2];
CardBackgroundRuntimeState &card_background_runtime(){return runtime;}
CardBackgroundImageCtx *card_background_image_contexts(){return runtime.contexts;}
int card_background_context_index(CardBackgroundImageCtx *ctx){return ctx-runtime.contexts;}
State &card_background_state(CardBackgroundImageCtx *ctx){return states[card_background_context_index(ctx)];}
Controller &card_background_controller(){return controller;}
CardBackgroundImageCtx *target=nullptr;
CardBackgroundImageCtx *acquire_card_background_image_context(const GridConfig &,const std::string &,int,int){return target;}
CardBackgroundImageCtx::Binding *card_background_add_binding(CardBackgroundImageCtx *ctx,lv_obj_t *btn,lv_obj_t *widget){
 for(auto &binding:ctx->bindings)if(!binding.active){binding={btn,widget,true};return &binding;}return nullptr;
}
void card_background_deactivate_if_unused(CardBackgroundImageCtx *ctx){
 for(auto &binding:ctx->bindings)if(binding.active)return;
 ctx->image->release();card_background_state(ctx).active=false;
}
void card_background_position_widget(lv_obj_t *,lv_obj_t *){}
void card_background_request_download(CardBackgroundImageCtx *ctx){++ctx->image->downloads;}
void card_background_sync_binding_image(CardBackgroundImageCtx *ctx,CardBackgroundImageCtx::Binding *binding){
 if(card_background_state(ctx).ready){binding->widget->source=ctx->image;binding->widget->hidden=false;}
 else card_background_request_download(ctx);
}
int completed=0,failed=0;
void card_background_apply_downloaded(CardBackgroundImageCtx *,bool){++completed;}
void card_background_handle_download_error(CardBackgroundImageCtx *){++failed;}
'''
text = (root / 'components/espcontrol/card_background_rendering.cpp').read_text()
text = re.sub(r'^(bool|void) (card_background_|refresh_card_background_image)', r'inline \1 \2', text, flags=re.M)
for name in ('card_background_runtime_shutdown', 'card_background_bind_callbacks', 'card_background_move_binding_to_size', 'refresh_card_background_image'):
    line, code = definition(text, name)
    source += f'\n#line {line} "card_background_rendering.cpp"\n{code}\n'
source += r'''
int main(){
 GridConfig config;
 for(bool ready:{false,true}){
  runtime=CardBackgroundRuntimeState{};states[0]=State{};states[1]=State{};states[1].ready=ready;
  Image old_image,new_image;lv_obj_t button,widget;
  widgets={&widget};widget.source=&old_image;
  auto *ctx=&runtime.contexts[0];ctx->image=&old_image;ctx->bindings[0]={&button,&widget,true};
  target=&runtime.contexts[1];target->image=&new_image;
  assert(card_background_move_binding_to_size(ctx,ctx->bindings[0],config,100,100));
  assert(old_image.releases==1 && !ctx->bindings[0].active);
  assert(ready ? widget.source==&new_image : widget.source==nullptr && widget.hidden);
  if(!ready)assert(new_image.downloads==1);
  // Shutdown must detach every remaining widget before freeing either image.
  card_background_runtime_shutdown(runtime);assert(widget.source==nullptr && widget.hidden);
 }
 {
  runtime=CardBackgroundRuntimeState{};states[0]=State{};states[1].active=false;
  Image image;image.width=40;image.height=40;lv_obj_t button,widget;
  widgets={&widget};widget.source=&image;
  auto *ctx=&runtime.contexts[0];ctx->image=&image;ctx->url="image";ctx->bindings[0]={&button,&widget,true};
  refresh_card_background_image(&button,config);
  assert(image.releases==1 && image.downloads==1 && widget.source==nullptr && widget.hidden);
 }
 {
  runtime=CardBackgroundRuntimeState{};states[0]=State{};Image image;lv_obj_t button,widget;
  widgets={&widget};widget.source=&image;auto *ctx=&runtime.contexts[0];ctx->image=&image;
  ctx->bindings[0]={&button,&widget,true};target=nullptr;
  assert(!card_background_move_binding_to_size(ctx,ctx->bindings[0],config,100,100));
  assert(widget.source==&image && image.releases==0 && ctx->bindings[0].active);
  card_background_bind_callbacks(ctx);card_background_bind_callbacks(ctx);
  assert(image.finished.size()==1 && image.errors.size()==1);
  image.finished[0](false);image.errors[0]();assert(completed==1 && failed==1);
  card_background_runtime_shutdown(runtime);
  runtime=CardBackgroundRuntimeState{};runtime.contexts[0].image=&image;
  card_background_bind_callbacks(&runtime.contexts[0]);
  for(auto &cb:image.finished)cb(false);for(auto &cb:image.errors)cb();
  assert(completed==2 && failed==2); // Old runtime callbacks are inert.
 }
}
'''
with tempfile.TemporaryDirectory(prefix='espcontrol-image-lifecycle-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    subprocess.run([sys.argv[1] if len(sys.argv)>1 else 'c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(path/'test.cpp'), '-o', str(path/'test')], check=True)
    subprocess.run([str(path/'test')], check=True)
print('Image lifecycle passed: resize, pending/ready replacements, unavailable contexts, teardown and service restart callbacks.')
