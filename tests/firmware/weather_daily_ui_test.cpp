// Exercise the production widget ownership and resizing against a small LVGL
// object/event model. Physical font rendering remains a device test.
#include <cassert>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include "button_grid_weather_daily_data.h"
#include "icons.h"
#include "button_grid_modal_layout.h"
// Inject the two setup allocation failures without changing the production allocator.
bool fail_nothrow_allocation=false, fail_lvgl_allocation=false;
void *operator new(std::size_t size, const std::nothrow_t &) noexcept {
  if (fail_nothrow_allocation) { fail_nothrow_allocation=false; return nullptr; }
  try { return ::operator new(size); } catch (...) { return nullptr; }
}
void operator delete(void *ptr, const std::nothrow_t &) noexcept { ::operator delete(ptr); }
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *data; int code = 0; };
using Callback = void (*)(lv_event_t *);
struct Binding { Callback callback; int code; void *data; };
struct lv_font_t { int line_height; };
lv_font_t font{16}, icons{32}, small_font{12}, summary_font{24};
int measured_font_pixels=0;
#define LV_FONT_DEFAULT (&font)
struct lv_obj_t {
  static int live;
  lv_obj_t *parent = nullptr;
  std::vector<lv_obj_t *> children;
  std::vector<Binding> events;
  const lv_font_t *font = &::font;
  int width = 460, height = 160, flags = 0, x = 0, y = 0, text_align = 0, text_color = 0;
  std::string text;
  explicit lv_obj_t(lv_obj_t *parent) : parent(parent) { ++live; if (parent) parent->children.push_back(this); }
  ~lv_obj_t() { --live; }
};
int lv_obj_t::live = 0;
constexpr int LV_PART_MAIN=0, LV_STATE_DISABLED=1, LV_TEXT_ALIGN_CENTER=0, LV_LABEL_LONG_DOT=0, LV_LABEL_LONG_WRAP=1;
using lv_text_align_t=int;
constexpr int LV_TEXT_ALIGN_LEFT=1, LV_TEXT_ALIGN_RIGHT=2;
constexpr int LV_ALIGN_TOP_MID=0, LV_ALIGN_TOP_LEFT=0, LV_ALIGN_BOTTOM_RIGHT=0, LV_ALIGN_BOTTOM_LEFT=0;
constexpr int LV_OPA_10=25, LV_OPA_70=70, LV_OPA_COVER=255;
constexpr int LV_OPA_TRANSP=0, LV_OBJ_FLAG_SCROLLABLE=1, LV_OBJ_FLAG_CLICKABLE=2, LV_COORD_MAX=32767, LV_TEXT_FLAG_NONE=0;
constexpr int LV_OBJ_FLAG_HIDDEN=4, LV_OBJ_FLAG_USER_2=1<<28;
constexpr int LV_EVENT_ALL=0, LV_EVENT_DELETE=1, LV_EVENT_SIZE_CHANGED=2, LV_EVENT_CLICKED=3, LV_EVENT_STYLE_CHANGED=4;
int lv_event_get_code(lv_event_t *e){return e->code;}
struct lv_point_t { int x,y; };
void *lv_event_get_user_data(lv_event_t *e) { return e->data; }
void *lv_event_get_target(lv_event_t *e) { return e->target; }
void lv_obj_add_event_cb(lv_obj_t *obj, Callback cb, int code, void *data) { obj->events.push_back({cb,code,data}); }
void lv_obj_remove_event_cb(lv_obj_t *obj, Callback cb) { auto &e=obj->events; e.erase(std::remove_if(e.begin(),e.end(),[cb](auto &b){return b.callback==cb;}),e.end()); }
void emit(lv_obj_t *obj, int code) { const auto bindings=obj->events; for(auto &b:bindings) if(b.code==code || b.code==LV_EVENT_ALL){lv_event_t event{obj,b.data,code}; b.callback(&event);} }
lv_obj_t *lv_obj_create(lv_obj_t *parent) { if(fail_lvgl_allocation){fail_lvgl_allocation=false;return nullptr;} return new lv_obj_t(parent); }
lv_obj_t *lv_label_create(lv_obj_t *parent) { return lv_obj_create(parent); }
lv_obj_t *lv_btn_create(lv_obj_t *parent) { return lv_obj_create(parent); }
void lv_obj_del(lv_obj_t *obj);
void lv_obj_clean(lv_obj_t *obj) { while(!obj->children.empty())lv_obj_del(obj->children.back()); }
void lv_obj_del(lv_obj_t *obj) { emit(obj,LV_EVENT_DELETE); lv_obj_clean(obj); if(obj->parent){auto &c=obj->parent->children;c.erase(std::remove(c.begin(),c.end(),obj),c.end());} delete obj; }
void lv_obj_set_size(lv_obj_t *obj,int w,int h){obj->width=w;obj->height=h;emit(obj,LV_EVENT_SIZE_CHANGED);}
void lv_obj_set_width(lv_obj_t *obj,int w){obj->width=w;}
void lv_obj_set_height(lv_obj_t *obj,int h){obj->height=h;}
int lv_obj_get_content_width(lv_obj_t *obj){return obj->width;}
int lv_obj_get_width(lv_obj_t *obj){return obj->width;}
int lv_obj_get_content_height(lv_obj_t *obj){return obj->height;}
int lv_font_get_line_height(const lv_font_t *f){return f->line_height;}
void lv_text_get_size(lv_point_t *point,const char *text,const lv_font_t *measured_font,int,int,int,int){
  if(!measured_font_pixels){point->x=std::strlen(text)*8;point->y=measured_font->line_height;return;}
  const double pixels=measured_font_pixels*static_cast<double>(measured_font->line_height)/font.line_height;
  double advance=0;
  for(const auto *ch=reinterpret_cast<const unsigned char *>(text);*ch;++ch){
    if((*ch & 0xc0)==0x80)continue;
    const double width=(*ch=='/' || *ch=='-' || *ch=='.' || *ch==0xc2)?0.3:(*ch>='0' && *ch<='9')?0.55:0.6;
    advance+=pixels*width;
  }
  point->x=static_cast<int>(std::ceil(advance));point->y=measured_font->line_height;
}
void lv_label_set_display_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_label_set_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_obj_set_style_text_font(lv_obj_t *obj,const lv_font_t *f,int){obj->font=f;obj->height=f->line_height;}
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *obj,int){return obj->font;}
#define NOOP(name) template<class... T> void name(T...) {}
void lv_obj_set_style_text_color(lv_obj_t *obj,int color,int){obj->text_color=color;}
void theme_set_content_foreground(lv_obj_t *obj){obj->flags|=LV_OBJ_FLAG_USER_2;}
NOOP(lv_label_set_long_mode) NOOP(lv_obj_set_style_text_opa)
void lv_obj_set_style_text_align(lv_obj_t *obj,int align,int){obj->text_align=align;}
void lv_obj_set_pos(lv_obj_t *obj,int x,int y){obj->x=x;obj->y=y;}
void lv_obj_set_y(lv_obj_t *obj,int y){obj->y=y;}
void lv_obj_add_flag(lv_obj_t *obj,int flag){obj->flags|=flag;}
void lv_obj_clear_flag(lv_obj_t *obj,int flag){obj->flags&=~flag;}
void lv_obj_align(lv_obj_t *obj,int,int x,int y){obj->x=(obj->parent?obj->parent->width-obj->width:0)/2+x;obj->y=y;}
NOOP(lv_obj_set_style_bg_color) NOOP(lv_obj_center)
NOOP(lv_obj_set_style_radius) NOOP(lv_obj_set_style_bg_opa) NOOP(lv_obj_set_style_border_width) NOOP(lv_obj_set_style_pad_all)
NOOP(lv_obj_add_state) NOOP(lv_obj_clear_state) NOOP(apply_text_width_compensation)
NOOP(control_modal_apply_pressed_fill) NOOP(set_clock_bar_modal_label)
int lv_color_hex(int c){return c;} int lv_pct(int n){return n;}
struct Theme { int text_primary=0, surface_secondary=0, surface_card=0, text_muted=0xB0B0B0; } theme;
const Theme &current_theme(){return theme;}
std::string translated_today;
std::string espcontrol_i18n(const std::string &s){return s=="Today" && !translated_today.empty()?translated_today:s;}
bool use_fahrenheit=false;
const char *display_temperature_unit_symbol(){return use_fahrenheit?"°F":"°C";}
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
const lv_font_t *display_small_text_font(const DisplayProfile &, const lv_font_t *){return &small_font;}
const lv_font_t *display_icon_font(const DisplayProfile &){return &icons;}
const lv_font_t *display_media_title_font(const DisplayProfile &){return &summary_font;}
const lv_font_t *display_volume_label_font(const DisplayProfile &, const lv_font_t *){return &small_font;}
namespace esphome { using StringRef = std::string; }
std::string string_ref_limited(const std::string &state,size_t size){return state.substr(0,size);}
struct HaBinding{void *owner;std::string key;std::function<void(esphome::StringRef)> callback;};
std::vector<HaBinding> ha_bindings;
void *ha_owner=nullptr;
struct HaCallbackOwnerScope{
  void *previous;
  explicit HaCallbackOwnerScope(void *owner):previous(ha_owner){ha_owner=owner;}
  ~HaCallbackOwnerScope(){ha_owner=previous;}
};
void ha_release_callbacks_for_owner(void *owner){
  ha_bindings.erase(std::remove_if(ha_bindings.begin(),ha_bindings.end(),[owner](const auto &binding){return binding.owner==owner;}),ha_bindings.end());
}
bool ha_subscribe_state(const std::string &,std::function<void(esphome::StringRef)> cb){ha_bindings.push_back({ha_owner,"",std::move(cb)});return true;}
bool ha_subscribe_attribute(const std::string &,const std::string &key,std::function<void(esphome::StringRef)> cb){ha_bindings.push_back({ha_owner,key,std::move(cb)});return true;}
void ha_reannounce_state_subscriptions(){}
void publish_weather(const std::string &key,const std::string &value){
  const auto bindings=ha_bindings;
  for(const auto &binding:bindings)if(binding.key==key)binding.callback(value);
}
struct ParsedCfg{std::string entity,label;std::string precision="daily",type="weather";};
struct BtnSlot{lv_obj_t *btn,*text_lbl,*icon_lbl;};
enum class ControlModalKind{WEATHER_FORECAST};
long modal_width=300, screen_width=480, screen_height=480;
long modal_back=40, modal_inset=10, modal_top_padding=0, modal_bottom_padding=0;
int scroll_list_creations=0;
struct ControlModalShell{lv_obj_t *overlay,*panel;long content_w=300;struct{long back_size=40,inset=10,panel_h=400,sw=480,sh=480;}layout;};
void weather_daily_hide_modal();
void control_modal_force_close_active(){weather_daily_hide_modal();}
void control_modal_delete_overlay(ControlModalKind,lv_obj_t *&overlay){if(overlay)lv_obj_del(overlay);overlay=nullptr;}
ControlModalShell control_modal_open_shell(ControlModalKind,lv_obj_t *,int,const lv_font_t *,void(*)()) {
  weather_daily_hide_modal();auto *overlay=lv_obj_create(nullptr);auto *panel=lv_obj_create(overlay);
  lv_obj_set_size(panel,modal_width+modal_inset*2,screen_height-modal_top_padding-modal_bottom_padding);
  return {overlay,panel,modal_width,{modal_back,modal_inset,screen_height-modal_top_padding-modal_bottom_padding,screen_width,screen_height}};
}
lv_obj_t *control_modal_create_scroll_list(lv_obj_t *parent,int,int,int){++scroll_list_creations;return lv_obj_create(parent);}
#define ESP_LOGW(...) ((void)0)
#include "button_grid_weather_daily_ui.h"
void check_card_fit(const WeatherDailyCard *card){
  const int height=card->height-(card->source->stale?lv_font_get_line_height(card->font)+2:0);
  assert(card->columns.size()<=8 && card->layout.count==card->columns.size());
  for(const auto &column:card->columns){
    for(const auto *label:{column.icon,column.day,column.high}){
      assert(label->x>=0 && label->y>=0 && label->x+label->width<=card->width && label->y+label->height<=height);
    }
    assert(column.day->y+column.day->height<=column.high->y);
    if(card->layout.list)assert(column.icon->x+column.icon->width<=column.day->x);
    else assert(column.icon->y+column.icon->height<=column.day->y);
  }
}
void check_modal_fit(){
  const auto &ui=weather_daily_modal();
  assert(ui.body && !(ui.body->flags & LV_OBJ_FLAG_SCROLLABLE) && scroll_list_creations==0);
  assert(ha_bindings.empty()); // Forecast-only modals have no live current-condition subscriptions.
  auto within=[](const lv_obj_t *parent,const lv_obj_t *child){
    if(child->flags & LV_OBJ_FLAG_HIDDEN)return;
    assert(child->x>=0 && child->y>=0 && child->x+child->width<=parent->width && child->y+child->height<=parent->height);
    assert(!(child->flags & LV_OBJ_FLAG_SCROLLABLE));
  };
  within(ui.body->parent,ui.body);
  assert(ui.day_gap==weather_daily_day_gap(ui.font) && ui.columns_per_row==3 && ui.day_limit==6);
  for(const auto *child:ui.body->children)within(ui.body,child);
  for(size_t i=0;i<ui.columns.size();++i){
    const auto &column=ui.columns[i];
    within(ui.days,column.button);
    assert(column.button->y==static_cast<int>(i/3)*(ui.row_height+ui.gap));
    for(const auto *child:column.button->children)within(column.button,child);
    assert(column.icon->y==ui.day_vertical_padding && column.icon->x==ui.day_padding);
    assert(column.day->x==column.temperature->x && column.conditions->x==ui.day_padding);
    assert(column.conditions->y+column.conditions->height+ui.day_vertical_padding<=column.button->height);
    if(ui.side_header){
      assert(column.day->x>=column.icon->x+column.icon->width);
      assert(column.conditions->y>=std::max(column.icon->y+column.icon->height,column.temperature->y+column.temperature->height));
    } else {
      assert(column.icon->y+column.icon->height+ui.text_gap==column.day->y);
      assert(column.day->y+column.day->height<=column.temperature->y);
      assert(column.temperature->y+column.temperature->height<=column.conditions->y);
    }
    assert(column.icon->text_align==LV_TEXT_ALIGN_LEFT && column.day->text_align==LV_TEXT_ALIGN_LEFT && column.temperature->text_align==LV_TEXT_ALIGN_LEFT && column.conditions->text_align==LV_TEXT_ALIGN_LEFT);
  }
  assert(ui.days->y+ui.days->height<=ui.status->y);
  assert(ui.body->children.size()==4); // No selected-day rainfall/humidity footer.
  for(const auto &column:ui.columns){
    assert(!(column.button->flags & LV_OBJ_FLAG_CLICKABLE) && column.button->events.empty());
    assert(column.day->text_color==0xFFFFFF && (column.day->flags & LV_OBJ_FLAG_USER_2));
    assert(column.icon->text_color==theme.text_muted && column.temperature->text_color==theme.text_muted && column.conditions->text_color==theme.text_muted);
    assert(column.conditions->text.find("Humidity")==std::string::npos);
  }
}
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
    assert(weather_daily_modal().message->text=="Loading forecast");
    assert(weather_daily_modal().placeholder_icon->text=="\U000F18F6");
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
    assert(card->columns.front().day->text=="Today" && card->columns.front().high->text=="19°/10°");
    assert(espcontrol::weather::parse("WF1|2026-10-11|2026-10-12|°C|mm|km/h\n2026-10-11|sunny|18|10|0|0|10|75\n2026-10-12|rainy|17|11|80|2|20|80\n2026-10-13|cloudy|16|9||||\n2026-10-14|sunny|18|10||||\n",f));
    weather_daily_render_card(card);assert(card->columns.size()==4);
    assert(card->columns.front().day->width==103);
    assert(card->columns[1].day->x==119); // Visible columns share the available width after gaps.
    for(const auto &col:card->columns){
      assert(col.icon->text_align==LV_TEXT_ALIGN_LEFT && col.day->text_align==LV_TEXT_ALIGN_LEFT && col.high->text_align==LV_TEXT_ALIGN_LEFT);
      assert(col.icon->x==col.day->x && col.day->x==col.high->x);
      assert(col.high->text_color==theme.text_muted && (col.high->flags & LV_OBJ_FLAG_USER_2));
      assert(col.icon->y==0 && col.day->y==130 && col.high->y==148);
    }
    theme.text_muted=0x606060;emit(owner,LV_EVENT_STYLE_CHANGED);
    assert(card->columns.front().high->text_color==theme.text_muted);
    theme.text_muted=0xB0B0B0;emit(owner,LV_EVENT_STYLE_CHANGED);
    assert(card->columns.front().high->text_color==theme.text_muted);
    int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live); // Refreshes do not grow the widget tree.
    weather_daily_set_unavailable("weather.home");weather_daily_render_card(card);
    assert(card->columns.size()==4 && caption->text=="Last forecast" && lv_obj_t::live==live);
    assert(card->columns.front().high->y==130); // Keep stale status below the temperatures.
    card->source->stale=false;
    lv_obj_set_size(owner,460,240);
    assert(card->layout.columns==2 && card->layout.rows==2);check_card_fit(card);
    assert(lv_obj_t::live==live);
    lv_obj_set_size(owner,100,160);assert(card->columns.size()==2);check_card_fit(card);
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
      auto &modal=weather_daily_modal();
      assert(modal.columns.size()==4 && ha_bindings.empty());
      check_modal_fit();
      const auto &column=modal.columns[0];
      assert(column.day->text=="Today" && column.temperature->text=="18°/10°");
      assert(column.icon->y < column.day->y && column.day->y < column.temperature->y);
      assert(column.conditions->text.find("0 mm")!=std::string::npos);
      assert(column.conditions->text.find("0%")!=std::string::npos && column.conditions->text.find("10 km/h")!=std::string::npos);
      assert(modal.columns[1].conditions->text.find("80%")!=std::string::npos && modal.columns[1].conditions->text.find("20 km/h")!=std::string::npos);
      assert(modal.columns[2].conditions->flags & LV_OBJ_FLAG_HIDDEN);
      const int modal_live=lv_obj_t::live;
      for(int refresh=0;refresh<20;++refresh)weather_daily_refresh_modal();
      assert(lv_obj_t::live==modal_live && ha_bindings.empty());

      const auto second_day_text=modal.columns[1].conditions->text;
      emit(modal.columns[1].button,LV_EVENT_CLICKED);
      assert(modal.columns[1].conditions->text==second_day_text && second_day_text.find("2 mm")!=std::string::npos);
      assert(lv_obj_t::live==modal_live); // Tapping a day has no selection or allocation effect.
      const auto rainy_day=f.days[1];
      f.days[1].rain_probability=f.days[1].precipitation=f.days[1].wind=f.days[1].humidity=espcontrol::weather::MISSING;
      weather_daily_refresh_modal();assert(modal.columns[1].conditions->flags & LV_OBJ_FLAG_HIDDEN);
      modal.source->stale=true;weather_daily_refresh_modal();
      assert(modal.status->text=="Last forecast" && !(modal.status->flags & LV_OBJ_FLAG_HIDDEN));
      check_modal_fit();
      modal.source->stale=false;f.days[1]=rainy_day;
      emit(modal.columns[0].button,LV_EVENT_CLICKED);assert(lv_obj_t::live==modal_live);
      weather_daily_hide_modal();assert(card->source->users==1 && lv_obj_t::live==live && ha_bindings.empty());
    }
    weather_daily_open_modal(card);weather_daily_cleanup(owner);
    assert(!weather_daily_modal().overlay && weather_daily_cards().empty() && weather_daily_sources().empty());
    assert(lv_obj_t::live==baseline && owner->events.empty());
    weather_daily_setup(slot,{"weather.home","Garden"},{}); // Reusing a persistent slot does not accumulate callbacks.
    weather_daily_cleanup(owner);assert(owner->events.empty());
    lv_obj_del(owner);assert(lv_obj_t::live==0);
  }

  // Add days at the minimum-width boundaries and fill the width even after all eight fit.
  auto *wide_owner=lv_obj_create(nullptr);
  BtnSlot wide_slot{wide_owner,lv_label_create(wide_owner),lv_label_create(wide_owner)};
  weather_daily_setup(wide_slot,{"weather.week",""},{});
  auto *wide_card=weather_daily_card_for(wide_owner);
  std::string week="WF1|2026-10-11|2026-10-12|°C|mm|km/h\n";
  for(int day=11;day<19;++day)week+="2026-10-"+std::to_string(day)+"|sunny|18|10||||\n";
  assert(espcontrol::weather::parse(week,wide_card->source->forecast));
  const int boundaries[][2]={{100,1},{207,1},{208,2},{319,2},{320,3},{431,3},{432,4},
    {543,4},{544,5},{655,5},{656,6},{767,6},{768,7},{879,7},{880,8},{901,8}};
  for(const auto &test:boundaries){
    lv_obj_set_size(wide_owner,test[0],90);
    const auto &columns=wide_card->columns;
    assert(columns.size()==static_cast<size_t>(test[1]));
    assert(columns.front().day->x==0);
    assert(columns.back().day->x+columns.back().day->width==test[0]);
    int smallest=test[0],largest=0;
    for(size_t i=0;i<columns.size();++i){
      smallest=std::min(smallest,columns[i].day->width);
      largest=std::max(largest,columns[i].day->width);
      assert(columns[i].day->width>=96);
      if(i)assert(columns[i].day->x-(columns[i-1].day->x+columns[i-1].day->width)==16);
    }
    assert(largest-smallest<=1);check_card_fit(wide_card);
    const int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live);
  }
  // S3's native 3x3 grid, using conservative line heights and measured glyphs.
  font.line_height=26;small_font.line_height=22;icons.line_height=44;measured_font_pixels=22;
  const int spans[][5]={{1,1,1,1,0},{2,1,2,1,0},{3,1,4,1,0},
    {1,2,1,4,1},{2,2,2,2,0},{3,2,4,2,0},
    {1,3,1,6,1},{2,3,2,3,0},{3,3,3,3,0}};
  for(const auto &span:spans){
    const int width=150*span[0]+10*(span[0]-1)-28;
    const int height=138*span[1]+10*(span[1]-1)-28;
    lv_obj_set_size(wide_owner,width,height);
    assert(wide_card->layout.columns==static_cast<size_t>(span[2]) && wide_card->layout.rows==static_cast<size_t>(span[3]) && wide_card->layout.list==static_cast<bool>(span[4]));
    const auto expected=std::min(8,span[2]*span[3]);
    assert(wide_card->columns.size()==static_cast<size_t>(expected));check_card_fit(wide_card);
    const int widgets=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==widgets);
    wide_card->source->stale=true;weather_daily_refresh_visuals();check_card_fit(wide_card);
    wide_card->source->stale=false;weather_daily_refresh_visuals();check_card_fit(wide_card);
  }
  // Fewer provider days rebalance the grid without creating empty widgets.
  wide_card->source->forecast.count=6;lv_obj_set_size(wide_owner,442,258);
  assert(wide_card->layout.columns==3 && wide_card->layout.rows==2 && wide_card->columns.size()==6);check_card_fit(wide_card);
  wide_card->source->forecast.count=8;
  font.line_height=16;small_font.line_height=12;icons.line_height=32;measured_font_pixels=0;
  weather_daily_open_modal(wide_card);
  auto &paged=weather_daily_modal();
  assert(paged.day_limit==6 && paged.columns.size()==6);
  assert(paged.columns_per_row==3); // Six day panels in exactly two rows.
  assert(paged.columns[2].button->y==0 && paged.columns[3].button->y>0);
  assert(paged.columns[2].button->x+paged.columns[2].button->width==paged.row_width);
  assert(paged.columns[5].button->x+paged.columns[5].button->width==paged.row_width);
  emit(paged.columns[5].button,LV_EVENT_CLICKED);check_modal_fit();
  use_fahrenheit=true;weather_daily_refresh_modal();
  assert(paged.columns[0].temperature->text=="64°/50°");
  use_fahrenheit=false;weather_daily_refresh_modal();
  const auto saved_week=paged.source->forecast;
  paged.source->forecast=espcontrol::weather::Forecast{};paged.source->loading=true;
  weather_daily_refresh_modal();
  assert(paged.columns.empty() && paged.message->text=="Loading forecast");
  assert(paged.placeholder_icon->text=="\U000F18F6" && (paged.days->flags & LV_OBJ_FLAG_HIDDEN));
  paged.source->forecast=saved_week;weather_daily_refresh_modal();assert(paged.columns.size()==6);
  const size_t old_capacity=paged.columns_per_row;
  paged.source->forecast.count=1;weather_daily_refresh_modal();
  assert(paged.columns.size()==1);
  assert(paged.columns_per_row==old_capacity && paged.columns[0].button->width<paged.row_width);
  weather_daily_hide_modal();assert(ha_bindings.empty());
  wide_card->source->forecast=saved_week;
  const long screens[][3]={{480,480,6},{720,720,6},{480,800,6},{800,480,6},
    {1024,600,6},{600,1024,6},{1280,800,6},{800,1280,6}};
  for(const auto &screen:screens){
    screen_width=screen[0];screen_height=screen[1];modal_width=screen_width-44;
    weather_daily_open_modal(wide_card);
    auto &modal=weather_daily_modal();
    assert(modal.day_limit==static_cast<size_t>(screen[2]) && modal.columns.size()==6);
    for(size_t i=0;i<modal.columns.size();++i){
      const auto *button=modal.columns[i].button;
      assert(button->width>modal.day_padding*2);
      if(i && button->y==modal.columns[i-1].button->y){
        const auto *previous=modal.columns[i-1].button;
        assert(button->x==previous->x+previous->width+modal.day_gap);
      } else assert(button->x==0);
      if((i+1)%modal.columns_per_row==0)assert(button->x+button->width==modal.row_width);
    }
    emit(modal.columns.back().button,LV_EVENT_CLICKED);check_modal_fit();
    weather_daily_hide_modal();assert(ha_bindings.empty());
  }
  // Supported resolutions, both rotations, conservative real font line heights,
  // and the production shell geometry including the clock-bar/page padding.
  const int displays[][7]={{480,480,22,26,44,41,40},{720,720,33,40,66,63,60},
    {480,800,29,35,62,68,50},{1024,600,22,26,55,52,42},{1280,800,24,29,56,56,46}};
  for(const auto &display:displays)for(bool rotated:{false,true})for(const char *today_label:{"Today","Aujourd'hui"}) {
    translated_today=today_label;
    small_font.line_height=static_cast<int>(std::ceil(display[3]*0.82));
    font.line_height=display[3];icons.line_height=display[4];summary_font.line_height=display[5];
    measured_font_pixels=display[2];
    screen_width=display[rotated?1:0];screen_height=display[rotated?0:1];
    modal_top_padding=display[6];modal_bottom_padding=4;
    espcontrol::modal::FrameRequest request;
    request.screen_width=screen_width;request.screen_height=screen_height;
    request.panel_left=5;request.panel_right=5;request.panel_top=modal_top_padding;request.panel_bottom=modal_bottom_padding;
    const auto frame=espcontrol::modal::calculate_frame({},request);
    modal_back=frame.back_size;modal_inset=frame.inset;modal_width=frame.panel_width-frame.inset*2;
    auto &forecast=wide_card->source->forecast;forecast=saved_week;
    for(auto &day:forecast.days){day.rain_probability=80;day.precipitation=2.4f;day.wind=12;day.humidity=82;}
    for(size_t available:{7u,1u,3u}){
      forecast.count=available;wide_card->source->loading=false;wide_card->source->stale=true;
      weather_daily_open_modal(wide_card);
      auto &ui=weather_daily_modal();
      assert(ui.columns.size()==std::min(ui.day_limit,available));
      assert(ui.message->flags & LV_OBJ_FLAG_HIDDEN);
      check_modal_fit();
      const int widgets=lv_obj_t::live;
      for(size_t i=0;i<ui.columns.size();++i){
        emit(ui.columns[i].button,LV_EVENT_CLICKED);check_modal_fit();assert(lv_obj_t::live==widgets);
      }
      forecast.count=0;wide_card->source->loading=true;weather_daily_refresh_modal();
      assert(ui.message->text=="Loading forecast" && ui.placeholder_icon->text=="\U000F18F6");
      check_modal_fit();
      forecast.count=available;weather_daily_refresh_modal();check_modal_fit();
      weather_daily_hide_modal();assert(ha_bindings.empty());
    }
  }
  translated_today.clear();
  small_font.line_height=12;font.line_height=16;icons.line_height=32;summary_font.line_height=24;measured_font_pixels=0;
  modal_top_padding=0;modal_bottom_padding=0;modal_back=40;modal_inset=10;
  wide_card->source->forecast=saved_week;wide_card->source->stale=false;
  screen_width=480;screen_height=480;modal_width=436;
  auto &rain_day=wide_card->source->forecast.days[0];
  rain_day.precipitation=2.4f;rain_day.wind=0;rain_day.humidity=82.4f;
  weather_daily_open_modal(wide_card);
  auto &metrics=weather_daily_modal();
  assert(metrics.columns[0].conditions->text.find("2.4 mm")!=std::string::npos);
  assert(metrics.columns[0].conditions->text.find("0 km/h")!=std::string::npos);
  assert(metrics.columns[0].conditions->text.find("2.4 mm")!=std::string::npos);
  const auto first_tile=metrics.columns[0].button;
  rain_day.rain_probability=79.8f;weather_daily_refresh_modal();
  assert(metrics.columns[0].button==first_tile && metrics.columns[0].conditions->text.find("80%")!=std::string::npos);
  assert(metrics.columns[0].conditions->text.find("2.4 mm")!=std::string::npos);
  rain_day.wind=espcontrol::weather::MISSING;rain_day.humidity=espcontrol::weather::MISSING;
  weather_daily_refresh_modal();
  assert(metrics.columns[0].conditions->text.find("km/h")==std::string::npos && metrics.columns[0].conditions->text.find("2.4 mm")!=std::string::npos);
  const auto mm_text=metrics.columns[0].conditions->text;
  std::strcpy(metrics.source->forecast.precipitation_unit,"in");
  weather_daily_refresh_modal();
  assert(metrics.columns[0].conditions->text.find("2.4 in")!=std::string::npos); // Never mislabel a provider amount as mm.
  std::strcpy(metrics.source->forecast.precipitation_unit,"mm");
  theme.text_muted=0x888888;weather_daily_refresh_modal();
  check_modal_fit();
  assert(metrics.columns[0].conditions->text==mm_text);
  theme.text_muted=0xB0B0B0;
  weather_daily_hide_modal();
  modal_width=300;
  weather_daily_cleanup(wide_owner);lv_obj_del(wide_owner);
  assert(weather_daily_sources().empty() && lv_obj_t::live==0);

  // Imported/unsafe entity IDs fail visibly without allocating a loading source.
  for(const auto *entity:{"", "sensor.temperature", "weather.", "weather.bad.id", "weather.bad'", "weather.bad name"}) {
    auto *owner=lv_obj_create(nullptr);BtnSlot slot{owner,lv_label_create(owner),lv_label_create(owner)};
    lv_obj_add_flag(slot.icon_lbl,LV_OBJ_FLAG_HIDDEN);
    weather_daily_setup(slot,{entity,""},{});
    assert(slot.text_lbl->text=="Forecast unavailable" && slot.icon_lbl->text=="unavailable-icon");
    assert(!(slot.icon_lbl->flags & LV_OBJ_FLAG_HIDDEN) && !(owner->flags & LV_OBJ_FLAG_CLICKABLE));
    assert(weather_daily_cards().empty() && weather_daily_sources().empty());
    lv_obj_del(owner);assert(lv_obj_t::live==0);
  }
  for(bool root_failure:{false,true}) {
    auto *owner=lv_obj_create(nullptr);BtnSlot slot{owner,lv_label_create(owner),lv_label_create(owner)};
    auto *source=weather_daily_acquire_source("weather.allocation"); // The next nothrow new belongs to the card.
    assert(source && source->users==1);
    lv_obj_add_flag(slot.icon_lbl,LV_OBJ_FLAG_HIDDEN);
    if(root_failure)fail_lvgl_allocation=true;else fail_nothrow_allocation=true;
    weather_daily_setup(slot,{"weather.allocation",""},{});
    assert(!weather_daily_card_for(owner) && source->users==1);
    assert(slot.text_lbl->text=="Forecast unavailable" && !(slot.icon_lbl->flags & LV_OBJ_FLAG_HIDDEN));
    weather_daily_release_source(source);lv_obj_del(owner);
    assert(lv_obj_t::live==0 && weather_daily_sources().empty());
  }
  // Replacing a full subpage with new main-screen entities must free the old cap first.
  // All forecast modes share the eight-source cap; a ninth entity is unavailable.
  for(int rebuild=0;rebuild<2;++rebuild) {
    std::vector<lv_obj_t *> owners;
    for(int i=0;i<9;++i) {
      auto *owner=lv_obj_create(nullptr);owners.push_back(owner);
      BtnSlot slot{owner,lv_label_create(owner),lv_label_create(owner)};
      const std::string mode=i%3==0?"today":i%3==1?"tomorrow":"daily";
      weather_daily_setup(slot,{"weather.rebuild_"+std::to_string(rebuild)+"_"+std::to_string(i),"",mode},{});
      assert((weather_daily_card_for(owner)!=nullptr)==(i<8));
      if(i==8)assert(slot.text_lbl->text=="Forecast unavailable");
    }
    assert(weather_daily_sources().size()==8 && weather_daily_cards().size()==8);
    weather_daily_open_modal(weather_daily_card_for(owners[2])); // Reset also releases an open modal's reference.
    weather_daily_reset_cards();
    assert(weather_daily_sources().empty() && weather_daily_cards().empty() && ha_bindings.empty());
    for(auto *owner:owners){assert(owner->events.empty());lv_obj_del(owner);}
    assert(lv_obj_t::live==0);
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
    assert(card->columns[0].high->text==(tomorrow?"17°/9°":"19°/10°"));
    assert(card->columns[0].icon->x==0 && card->columns[0].icon->y==0);
    assert(card->columns[0].day->text_align==LV_TEXT_ALIGN_LEFT && card->columns[0].high->y==148);
    lv_obj_set_size(owner,900,240);assert(card->columns.size()==1 && card->columns[0].high->y==228);
    const int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live); // Single-day refreshes also reuse widgets.
    weather_daily_open_modal(card);
    check_modal_fit(); // Single-day cards open the same non-selectable forecast grid.
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
