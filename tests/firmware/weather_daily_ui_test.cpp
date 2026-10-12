// Exercise the production widget ownership and resizing against a small LVGL
// object/event model. Physical font rendering remains a device test.
#include <cassert>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include "button_grid_weather_daily_data.h"
#include "icons.h"
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *data; };
using Callback = void (*)(lv_event_t *);
struct Binding { Callback callback; int code; void *data; };
struct lv_font_t { int line_height; };
lv_font_t font{16}, icons{32};
#define LV_FONT_DEFAULT (&font)
struct lv_obj_t {
  static int live;
  lv_obj_t *parent = nullptr;
  std::vector<lv_obj_t *> children;
  std::vector<Binding> events;
  const lv_font_t *font = &::font;
  int width = 460, height = 160, flags = 0, x = 0, y = 0, text_align = 0;
  std::string text;
  explicit lv_obj_t(lv_obj_t *parent) : parent(parent) { ++live; if (parent) parent->children.push_back(this); }
  ~lv_obj_t() { --live; }
};
int lv_obj_t::live = 0;
constexpr int LV_PART_MAIN=0, LV_STATE_DISABLED=1, LV_TEXT_ALIGN_CENTER=0, LV_LABEL_LONG_DOT=0, LV_LABEL_LONG_WRAP=1;
constexpr int LV_TEXT_ALIGN_LEFT=1;
constexpr int LV_ALIGN_TOP_MID=0, LV_ALIGN_TOP_LEFT=0, LV_ALIGN_BOTTOM_RIGHT=0, LV_ALIGN_BOTTOM_LEFT=0;
constexpr int LV_OPA_TRANSP=0, LV_OBJ_FLAG_SCROLLABLE=1, LV_OBJ_FLAG_CLICKABLE=2, LV_COORD_MAX=32767, LV_TEXT_FLAG_NONE=0;
constexpr int LV_OBJ_FLAG_HIDDEN=4;
constexpr int LV_EVENT_DELETE=1, LV_EVENT_SIZE_CHANGED=2, LV_EVENT_CLICKED=3;
struct lv_point_t { int x,y; };
void *lv_event_get_user_data(lv_event_t *e) { return e->data; }
void *lv_event_get_target(lv_event_t *e) { return e->target; }
void lv_obj_add_event_cb(lv_obj_t *obj, Callback cb, int code, void *data) { obj->events.push_back({cb,code,data}); }
void lv_obj_remove_event_cb(lv_obj_t *obj, Callback cb) { auto &e=obj->events; e.erase(std::remove_if(e.begin(),e.end(),[cb](auto &b){return b.callback==cb;}),e.end()); }
void emit(lv_obj_t *obj, int code) { const auto bindings=obj->events; for(auto &b:bindings) if(b.code==code){lv_event_t event{obj,b.data}; b.callback(&event);} }
lv_obj_t *lv_obj_create(lv_obj_t *parent) { return new lv_obj_t(parent); }
lv_obj_t *lv_label_create(lv_obj_t *parent) { return lv_obj_create(parent); }
lv_obj_t *lv_btn_create(lv_obj_t *parent) { return lv_obj_create(parent); }
void lv_obj_del(lv_obj_t *obj);
void lv_obj_clean(lv_obj_t *obj) { while(!obj->children.empty())lv_obj_del(obj->children.back()); }
void lv_obj_del(lv_obj_t *obj) { emit(obj,LV_EVENT_DELETE); lv_obj_clean(obj); if(obj->parent){auto &c=obj->parent->children;c.erase(std::remove(c.begin(),c.end(),obj),c.end());} delete obj; }
void lv_obj_set_size(lv_obj_t *obj,int w,int h){obj->width=w;obj->height=h;emit(obj,LV_EVENT_SIZE_CHANGED);}
void lv_obj_set_width(lv_obj_t *obj,int w){obj->width=w;}
int lv_obj_get_content_width(lv_obj_t *obj){return obj->width;}
int lv_obj_get_content_height(lv_obj_t *obj){return obj->height;}
int lv_font_get_line_height(const lv_font_t *f){return f->line_height;}
void lv_text_get_size(lv_point_t *point,const char *text,const lv_font_t *,int,int,int,int){point->x=std::strlen(text)*8;point->y=16;}
void lv_label_set_display_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_label_set_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_obj_set_style_text_font(lv_obj_t *obj,const lv_font_t *f,int){obj->font=f;}
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *obj,int){return obj->font;}
#define NOOP(name) template<class... T> void name(T...) {}
NOOP(lv_obj_set_style_text_color) NOOP(lv_label_set_long_mode)
void lv_obj_set_style_text_align(lv_obj_t *obj,int align,int){obj->text_align=align;}
void lv_obj_set_pos(lv_obj_t *obj,int x,int y){obj->x=x;obj->y=y;}
void lv_obj_set_y(lv_obj_t *obj,int y){obj->y=y;}
void lv_obj_add_flag(lv_obj_t *obj,int flag){obj->flags|=flag;}
void lv_obj_clear_flag(lv_obj_t *obj,int flag){obj->flags&=~flag;}
NOOP(lv_obj_align) NOOP(lv_obj_set_style_bg_color) NOOP(lv_obj_center)
NOOP(lv_obj_set_style_bg_opa) NOOP(lv_obj_set_style_border_width) NOOP(lv_obj_set_style_pad_all)
NOOP(lv_obj_add_state) NOOP(lv_obj_clear_state) NOOP(apply_text_width_compensation)
NOOP(control_modal_apply_pressed_fill) NOOP(set_clock_bar_modal_label)
int lv_color_hex(int c){return c;} int lv_pct(int n){return n;}
struct Theme { int text_primary=0, surface_secondary=0; } theme;
const Theme &current_theme(){return theme;}
std::string espcontrol_i18n(const std::string &s){return s;}
const char *display_temperature_unit_symbol(){return "°C";}
bool use_fahrenheit=false;
float convert_temperature_value_for_display_float(float v,const char *unit){
  const bool fahrenheit=std::string(unit)=="°F";
  if(use_fahrenheit && !fahrenheit)return v*9.0f/5.0f+32.0f;
  if(!use_fahrenheit && fahrenheit)return (v-32.0f)*5.0f/9.0f;
  return v;
}
const char *weather_icon_for_state(const std::string &state){return state=="cloudy"?"cloudy-icon":state=="unavailable"?"unavailable-icon":"icon";}
std::string weather_label_for_state(const std::string &s){return s;}
void notify_dashboard_content_changed(){}
struct DisplayProfile{};
const lv_font_t *display_icon_font(const DisplayProfile &){return &icons;}
struct ParsedCfg{std::string entity,label;std::string precision="daily",type="weather";};
struct BtnSlot{lv_obj_t *btn,*text_lbl,*icon_lbl;};
enum class ControlModalKind{WEATHER_FORECAST};
struct ControlModalShell{lv_obj_t *overlay,*panel;int content_w=300;struct{int back_size=40,inset=10,panel_h=400;}layout;};
void weather_daily_hide_modal();
void control_modal_force_close_active(){weather_daily_hide_modal();}
void control_modal_delete_overlay(ControlModalKind,lv_obj_t *&overlay){if(overlay)lv_obj_del(overlay);overlay=nullptr;}
ControlModalShell control_modal_open_shell(ControlModalKind,lv_obj_t *,int,const lv_font_t *,void(*)()) {
  weather_daily_hide_modal();auto *overlay=lv_obj_create(nullptr);return {overlay,lv_obj_create(overlay),300,{40,10,400}};
}
lv_obj_t *control_modal_create_scroll_list(lv_obj_t *parent,int,int,int){return lv_obj_create(parent);}
#define ESP_LOGW(...) ((void)0)
#include "button_grid_weather_daily_ui.h"
int main(){
  assert(weather_daily_temperature(17.8f,"°C")=="18");
  assert(weather_daily_temperature(-2.6f,"°C")=="-3");
  assert(weather_daily_temperature(-0.2f,"°C")=="0");
  assert(weather_daily_temperature(espcontrol::weather::MISSING,"°C")=="--");
  use_fahrenheit=true;
  assert(weather_daily_temperature(17.8f,"°C")=="64");
  use_fahrenheit=false;
  assert(weather_daily_temperature(64.0f,"°F")=="18");
  for(int cycle=0;cycle<30;++cycle){
    auto *owner=lv_obj_create(nullptr);auto *caption=lv_label_create(owner);
    auto *placeholder_icon=lv_label_create(owner);BtnSlot slot{owner,caption,placeholder_icon};
    const int baseline=lv_obj_t::live;
    weather_daily_setup(slot,{"weather.home","Garden"},{});
    auto *card=weather_daily_card_for(owner);assert(card && card->source->users==1);
    assert(card->columns.empty() && card->root->children.empty());
    assert(card->root->flags & LV_OBJ_FLAG_HIDDEN);
    assert(!(placeholder_icon->flags & LV_OBJ_FLAG_HIDDEN));
    assert(placeholder_icon->text=="\U000F18F6" && caption->text=="Loading forecast");
    weather_daily_open_modal(card);
    assert(weather_daily_modal().date->text=="Loading forecast");
    assert(weather_daily_modal().icon->text=="\U000F18F6");
    weather_daily_hide_modal();

    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    lv_obj_set_size(owner,100,160);lv_obj_set_size(owner,460,160);
    assert(lv_obj_t::live==baseline+1 && card->columns.empty());
    weather_daily_set_unavailable("weather.home");weather_daily_render_card(card);
    assert(caption->text=="Forecast unavailable" && placeholder_icon->text=="unavailable-icon");
    assert(card->columns.empty());
    weather_daily_set_loading("weather.home");weather_daily_render_card(card);assert(caption->text=="Loading forecast");
    auto &f=card->source->forecast;
    assert(espcontrol::weather::parse("WF1|2026-10-11|2026-10-12|°C|mm|km/h\n2026-10-11|sunny|18.8|10|0|0|10|75\n",f));
    weather_daily_render_card(card);assert(card->columns.size()==1);
    assert(placeholder_icon->flags & LV_OBJ_FLAG_HIDDEN);
    assert(!(card->root->flags & LV_OBJ_FLAG_HIDDEN));
    assert(card->columns.front().day->text=="Today" && card->columns.front().high->text=="19°");
    assert(espcontrol::weather::parse("WF1|2026-10-11|2026-10-12|°C|mm|km/h\n2026-10-11|sunny|18|10|0|0|10|75\n2026-10-12|rainy|17|11|80|2|20|80\n2026-10-13|cloudy|16|9||||\n2026-10-14|sunny|18|10||||\n",f));
    weather_daily_render_card(card);assert(card->columns.size()==4);
    assert(card->columns.front().day->width==80);
    assert(card->columns[1].day->x==96); // Compact columns keep the gap without stretching across the card.
    for(const auto &col:card->columns){
      assert(col.icon->text_align==LV_TEXT_ALIGN_LEFT && col.day->text_align==LV_TEXT_ALIGN_LEFT && col.high->text_align==LV_TEXT_ALIGN_LEFT);
      assert(col.icon->x==col.day->x && col.day->x==col.high->x);
      assert(col.icon->y==0 && col.day->y==126 && col.high->y==144);
    }
    int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live); // Refreshes do not grow the widget tree.
    weather_daily_set_unavailable("weather.home");weather_daily_render_card(card);
    assert(card->columns.size()==4 && caption->text=="Last forecast" && lv_obj_t::live==live);
    assert(card->columns.front().high->y==126); // Keep stale status below the temperatures.
    card->source->stale=false;
    lv_obj_set_size(owner,460,240);
    assert(card->columns.front().icon->y==0 && card->columns.front().day->y==206 && card->columns.front().high->y==224);
    assert(lv_obj_t::live==live);
    lv_obj_set_size(owner,100,160);assert(card->columns.size()==1);
    lv_obj_set_size(owner,460,160);assert(card->columns.size()==4 && lv_obj_t::live==live);
    const auto saved_forecast=f;
    f=espcontrol::weather::Forecast{};card->source->loading=true;
    weather_daily_render_card(card);
    assert(card->columns.empty() && card->root->children.empty() && lv_obj_t::live==baseline+1);
    assert(!(placeholder_icon->flags & LV_OBJ_FLAG_HIDDEN) && caption->text=="Loading forecast");
    f=saved_forecast;weather_daily_render_card(card);
    assert(card->columns.size()==4 && lv_obj_t::live==live);
    for(int i=0;i<10;++i){
      weather_daily_open_modal(card);assert(card->source->users==2);
      emit(weather_daily_modal().next,LV_EVENT_CLICKED);assert(weather_daily_modal().day==1);
      weather_daily_hide_modal();assert(card->source->users==1 && lv_obj_t::live==live);
    }
    weather_daily_open_modal(card);weather_daily_cleanup(owner);
    assert(!weather_daily_modal().overlay && weather_daily_cards().empty() && weather_daily_sources().empty());
    assert(lv_obj_t::live==baseline && owner->events.empty());
    weather_daily_setup(slot,{"weather.home","Garden"},{}); // Reusing a persistent slot does not accumulate callbacks.
    weather_daily_cleanup(owner);assert(owner->events.empty());
    lv_obj_del(owner);assert(lv_obj_t::live==0);
  }

  // Single-day cards select dates exactly and share the multi-day source.
  auto *daily_owner=lv_obj_create(nullptr);
  BtnSlot daily_slot{daily_owner,lv_label_create(daily_owner),lv_label_create(daily_owner)};
  weather_daily_setup(daily_slot,{"weather.home",""},{});
  auto *daily=weather_daily_card_for(daily_owner);
  const auto payload="WF1|2026-10-11|2026-10-12|°C|mm|km/h\n2026-10-11|sunny|18.8|10||||\n2026-10-12|rainy|16.6|9||||\n2026-10-13|cloudy|14|8||||\n";
  assert(espcontrol::weather::parse(payload,daily->source->forecast));
  daily->source->loading=false;weather_daily_refresh_visuals();
  for(const auto &mode:{"today","tomorrow"}) {
    auto *owner=lv_obj_create(nullptr);
    BtnSlot slot{owner,lv_label_create(owner),lv_label_create(owner)};
    weather_daily_setup(slot,{"weather.home","Old label",mode},{});
    auto *card=weather_daily_card_for(owner);
    const bool tomorrow=std::string(mode)=="tomorrow";
    assert(card->source==daily->source && weather_daily_sources().size()==1 && card->source->users==2);
    assert(card->columns.size()==1 && card->columns[0].day->text==(tomorrow?"Tomorrow":"Today"));
    assert(card->columns[0].high->text==(tomorrow?"17°":"19°"));
    assert(card->columns[0].icon->x==0 && card->columns[0].icon->y==0);
    assert(card->columns[0].day->text_align==LV_TEXT_ALIGN_LEFT && card->columns[0].high->y==144);
    lv_obj_set_size(owner,900,240);assert(card->columns.size()==1 && card->columns[0].high->y==224);
    const int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live); // Single-day refreshes also reuse widgets.
    weather_daily_open_modal(card);
    assert(weather_daily_modal().day==(tomorrow?1u:0u));
    weather_daily_hide_modal();
    // Omit the requested day while other dates are still available.
    auto &f=card->source->forecast;
    if(tomorrow){f.days[1]=f.days[2];f.count=2;}
    else{f.days[0]=f.days[1];f.days[1]=f.days[2];f.count=2;}
    weather_daily_refresh_visuals();
    assert(card->columns.empty() && slot.text_lbl->text=="Forecast unavailable");
    assert(slot.icon_lbl->text=="unavailable-icon");
    weather_daily_open_modal(card);assert(!weather_daily_modal().overlay);
    weather_daily_set_loading("weather.home");weather_daily_refresh_visuals();
    assert(card->columns.empty() && slot.text_lbl->text=="Loading forecast");
    assert(slot.icon_lbl->text=="\U000F18F6");
    card->source->loading=false;

    assert(espcontrol::weather::parse(payload,f));weather_daily_refresh_visuals();
    assert(card->columns.size()==1 && card->columns[0].day->text==(tomorrow?"Tomorrow":"Today"));
    weather_daily_cleanup(owner);lv_obj_del(owner);assert(daily->source->users==1);
  }
  weather_daily_cleanup(daily_owner);lv_obj_del(daily_owner);
  assert(weather_daily_sources().empty() && lv_obj_t::live==0);
}
