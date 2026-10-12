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
lv_font_t font{16}, icons{32}, small_font{12}, summary_font{24};
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
using lv_text_align_t=int;
constexpr int LV_TEXT_ALIGN_LEFT=1, LV_TEXT_ALIGN_RIGHT=2;
constexpr int LV_ALIGN_TOP_MID=0, LV_ALIGN_TOP_LEFT=0, LV_ALIGN_BOTTOM_RIGHT=0, LV_ALIGN_BOTTOM_LEFT=0;
constexpr int LV_OPA_10=25, LV_OPA_70=70, LV_OPA_COVER=255;
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
void lv_obj_set_height(lv_obj_t *obj,int h){obj->height=h;}
int lv_obj_get_content_width(lv_obj_t *obj){return obj->width;}
int lv_obj_get_content_height(lv_obj_t *obj){return obj->height;}
int lv_font_get_line_height(const lv_font_t *f){return f->line_height;}
void lv_text_get_size(lv_point_t *point,const char *text,const lv_font_t *,int,int,int,int){point->x=std::strlen(text)*8;point->y=16;}
void lv_label_set_display_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_label_set_text(lv_obj_t *obj,const char *text){obj->text=text;}
void lv_obj_set_style_text_font(lv_obj_t *obj,const lv_font_t *f,int){obj->font=f;}
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *obj,int){return obj->font;}
#define NOOP(name) template<class... T> void name(T...) {}
NOOP(lv_obj_set_style_text_color) NOOP(lv_label_set_long_mode) NOOP(lv_obj_set_style_text_opa)
void lv_obj_set_style_text_align(lv_obj_t *obj,int align,int){obj->text_align=align;}
void lv_obj_set_pos(lv_obj_t *obj,int x,int y){obj->x=x;obj->y=y;}
void lv_obj_set_y(lv_obj_t *obj,int y){obj->y=y;}
void lv_obj_add_flag(lv_obj_t *obj,int flag){obj->flags|=flag;}
void lv_obj_clear_flag(lv_obj_t *obj,int flag){obj->flags&=~flag;}
NOOP(lv_obj_align) NOOP(lv_obj_set_style_bg_color) NOOP(lv_obj_center)
NOOP(lv_obj_set_style_radius) NOOP(lv_obj_set_style_bg_opa) NOOP(lv_obj_set_style_border_width) NOOP(lv_obj_set_style_pad_all)
NOOP(lv_obj_add_state) NOOP(lv_obj_clear_state) NOOP(apply_text_width_compensation)
NOOP(control_modal_apply_pressed_fill) NOOP(set_clock_bar_modal_label)
int lv_color_hex(int c){return c;} int lv_pct(int n){return n;}
struct Theme { int text_primary=0, surface_secondary=0; } theme;
const Theme &current_theme(){return theme;}
std::string espcontrol_i18n(const std::string &s){return s;}
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
struct ControlModalShell{lv_obj_t *overlay,*panel;long content_w=300;struct{long back_size=40,inset=10,panel_h=400,sw=480,sh=480;}layout;};
void weather_daily_hide_modal();
void control_modal_force_close_active(){weather_daily_hide_modal();}
void control_modal_delete_overlay(ControlModalKind,lv_obj_t *&overlay){if(overlay)lv_obj_del(overlay);overlay=nullptr;}
ControlModalShell control_modal_open_shell(ControlModalKind,lv_obj_t *,int,const lv_font_t *,void(*)()) {
  weather_daily_hide_modal();auto *overlay=lv_obj_create(nullptr);return {overlay,lv_obj_create(overlay),modal_width,{40,10,screen_height,screen_width,screen_height}};
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
    assert(weather_daily_modal().condition->text=="Loading forecast");
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
    assert(card->columns.front().day->width==103);
    assert(card->columns[1].day->x==119); // Visible columns share the available width after gaps.
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
      auto &modal=weather_daily_modal();
      assert(modal.columns.size()==4 && ha_bindings.size()==4);
      publish_weather("","rainy");publish_weather("friendly_name","Home");publish_weather("temperature","15.2");
      assert(modal.temperature->text=="--°C"); // Do not guess the source unit while attributes arrive.
      publish_weather("temperature_unit","°C");
      assert(modal.condition->text=="rainy" && modal.name->text=="Home" && modal.temperature->text=="15°C");
      assert(modal.range->text=="18° / 10°");
      const auto &column=modal.columns[0];
      assert(column.day->text=="Today" && column.high->text=="18°" && column.low->text=="10°");
      assert(column.day->y < column.icon->y && column.icon->y < column.high->y && column.high->y < column.low->y);
      assert(modal.name->y>modal.condition->y && modal.range->y>modal.temperature->y);
      assert(modal.temperature->text_align==LV_TEXT_ALIGN_RIGHT && modal.name->text_align==LV_TEXT_ALIGN_LEFT);
      assert(modal.details->text=="Today - sunny");
      assert(modal.metric_tiles.size()==3);
      assert(modal.metric_tiles[0].label->text=="Rain chance" && modal.metric_tiles[0].value->text=="0%");
      assert(modal.metric_tiles[0].unit->text=="0 mm");
      assert(modal.metric_tiles[1].label->text=="Wind" && modal.metric_tiles[1].value->text=="10" && modal.metric_tiles[1].unit->text=="km/h");
      assert(modal.metric_tiles[2].value->text=="75%");
      assert(modal.metric_tiles.back().tile->x+modal.metric_tiles.back().tile->width==modal.row_width);
      const int modal_live=lv_obj_t::live;
      for(int refresh=0;refresh<20;++refresh)weather_daily_refresh_modal();
      assert(lv_obj_t::live==modal_live && ha_bindings.size()==4);

      emit(modal.columns[1].button,LV_EVENT_CLICKED);assert(modal.day==1);
      assert(modal.details->text=="Mon - rainy" && modal.metric_tiles[0].value->text=="80%" && modal.metric_tiles[0].unit->text=="2 mm");
      assert(lv_obj_t::live==modal_live); // Selecting another complete day reuses all tiles.
      emit(modal.columns[2].button,LV_EVENT_CLICKED);assert(modal.metric_tiles.empty());
      assert((modal.details->flags & LV_OBJ_FLAG_HIDDEN) && (modal.metrics->flags & LV_OBJ_FLAG_HIDDEN));
      modal.source->stale=true;weather_daily_refresh_modal();
      assert(modal.status->text=="Last forecast" && !(modal.status->flags & LV_OBJ_FLAG_HIDDEN));
      modal.source->stale=false;
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

  // Add days at the minimum-width boundaries and fill the width even after all seven fit.
  auto *wide_owner=lv_obj_create(nullptr);
  BtnSlot wide_slot{wide_owner,lv_label_create(wide_owner),lv_label_create(wide_owner)};
  weather_daily_setup(wide_slot,{"weather.week",""},{});
  auto *wide_card=weather_daily_card_for(wide_owner);
  std::string week="WF1|2026-10-11|2026-10-12|°C|mm|km/h\n";
  for(int day=11;day<18;++day)week+="2026-10-"+std::to_string(day)+"|sunny|18|10||||\n";
  assert(espcontrol::weather::parse(week,wide_card->source->forecast));
  const int boundaries[][2]={{100,1},{159,1},{160,2},{247,2},{248,3},{335,3},{336,4},
    {423,4},{424,5},{511,5},{512,6},{599,6},{600,7},{901,7}};
  for(const auto &test:boundaries){
    lv_obj_set_size(wide_owner,test[0],160);
    const auto &columns=wide_card->columns;
    assert(columns.size()==static_cast<size_t>(test[1]));
    assert(columns.front().day->x==0);
    assert(columns.back().day->x+columns.back().day->width==test[0]);
    int smallest=test[0],largest=0;
    for(size_t i=0;i<columns.size();++i){
      smallest=std::min(smallest,columns[i].day->width);
      largest=std::max(largest,columns[i].day->width);
      assert(columns[i].day->width>=72);
      if(i)assert(columns[i].day->x-(columns[i-1].day->x+columns[i-1].day->width)==16);
    }
    assert(largest-smallest<=1);
    const int live=lv_obj_t::live;
    for(int i=0;i<20;++i)weather_daily_refresh_visuals();
    assert(lv_obj_t::live==live);
  }
  weather_daily_open_modal(wide_card);
  auto &paged=weather_daily_modal();
  assert(paged.day_limit==5 && paged.columns.size()==5);
  assert(paged.columns_per_row==4); // A narrow row wraps all five days into 3+2.
  assert(paged.columns[2].button->y==0 && paged.columns[3].button->y>0);
  assert(paged.columns[2].button->x+paged.columns[2].button->width==paged.row_width);
  assert(paged.columns[4].button->x+paged.columns[4].button->width==paged.row_width);
  emit(paged.columns[4].button,LV_EVENT_CLICKED);assert(paged.day==4);
  publish_weather("","sunny");publish_weather("temperature","68");publish_weather("temperature_unit","°F");
  assert(paged.temperature->text=="20°C");
  use_fahrenheit=true;weather_daily_refresh_modal();
  assert(paged.temperature->text=="68°F" && paged.range->text=="64° / 50°");
  assert(paged.columns[0].high->text=="64°" && paged.columns[0].low->text=="50°");
  use_fahrenheit=false;weather_daily_refresh_modal();
  publish_weather("temperature","99999");assert(paged.temperature->text=="--°C");
  publish_weather("temperature","68");publish_weather("","unavailable");assert(paged.temperature->text=="--°C");
  publish_weather("friendly_name",std::string(200,'x'));assert(std::strlen(paged.friendly_name)==64);
  const auto saved_week=paged.source->forecast;
  paged.source->forecast=espcontrol::weather::Forecast{};paged.source->loading=true;
  weather_daily_refresh_modal();
  assert(paged.columns.empty() && paged.condition->text=="Loading forecast");
  assert(paged.icon->text=="\U000F18F6" && (paged.days->flags & LV_OBJ_FLAG_HIDDEN));
  paged.source->forecast=saved_week;weather_daily_refresh_modal();assert(paged.columns.size()==5);
  const size_t old_capacity=paged.columns_per_row;
  paged.source->forecast.count=1;weather_daily_refresh_modal();
  assert(paged.columns.size()==1 && paged.day==0);
  assert(paged.columns_per_row==old_capacity && paged.columns[0].button->width==paged.row_width);
  const auto old_callback=ha_bindings[0].callback;
  weather_daily_hide_modal();assert(ha_bindings.empty());
  old_callback("rainy");assert(!weather_daily_modal().overlay);
  wide_card->source->forecast=saved_week;
  const long screens[][3]={{480,480,5},{720,720,5},{480,800,5},{800,480,5},
    {1024,600,7},{600,1024,7},{1280,800,7},{800,1280,7}};
  for(const auto &screen:screens){
    screen_width=screen[0];screen_height=screen[1];modal_width=screen_width-44;
    weather_daily_open_modal(wide_card);
    auto &modal=weather_daily_modal();
    assert(modal.day_limit==static_cast<size_t>(screen[2]) && modal.columns.size()==modal.day_limit);
    for(size_t i=0;i<modal.columns.size();++i){
      const auto *button=modal.columns[i].button;
      assert(button->width>=weather_daily_minimum_width(modal.font,modal.icon_font,12));
      if(i && button->y==modal.columns[i-1].button->y){
        const auto *previous=modal.columns[i-1].button;
        assert(button->x==previous->x+previous->width+12);
      } else assert(button->x==0);
      if(i+1==modal.columns.size() || button->y!=modal.columns[i+1].button->y)
        assert(button->x+button->width==modal.row_width);
    }
    emit(modal.columns.back().button,LV_EVENT_CLICKED);assert(modal.day==modal.day_limit-1);
    weather_daily_hide_modal();assert(ha_bindings.empty());
  }
  screen_width=480;screen_height=480;modal_width=436;
  auto &rain_day=wide_card->source->forecast.days[0];
  rain_day.precipitation=2.4f;rain_day.wind=0;rain_day.humidity=82.4f;
  weather_daily_open_modal(wide_card);
  auto &metrics=weather_daily_modal();
  assert(metrics.metric_tiles.size()==3 && metrics.metric_tiles[0].label->text=="Rainfall");
  assert(metrics.metric_tiles[0].value->text=="2.4" && metrics.metric_tiles[0].unit->text=="mm");
  assert(metrics.metric_tiles[1].value->text=="0" && metrics.metric_tiles[2].value->text=="82%");
  const auto first_tile=metrics.metric_tiles[0].tile;
  rain_day.rain_probability=79.8f;weather_daily_refresh_modal();
  assert(metrics.metric_tiles[0].tile==first_tile && metrics.metric_tiles[0].label->text=="Rain chance");
  assert(metrics.metric_tiles[0].value->text=="80%" && metrics.metric_tiles[0].unit->text=="2.4 mm");
  rain_day.wind=espcontrol::weather::MISSING;rain_day.humidity=espcontrol::weather::MISSING;
  weather_daily_refresh_modal();
  assert(metrics.metric_tiles.size()==1 && metrics.metric_tiles[0].tile->width==metrics.row_width);
  weather_daily_hide_modal();
  modal_width=300;
  weather_daily_cleanup(wide_owner);lv_obj_del(wide_owner);
  assert(weather_daily_sources().empty() && lv_obj_t::live==0);

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
