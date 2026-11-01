#include <cassert>
#include <functional>
#include <map>
#include <string>
#include <cstdint>
#include <vector>

struct lv_font_t { int line_height = 16; };
struct lv_obj_t { std::string text; bool valid = true; int flags = 1, y = 0, padding = 16, text_color = 0, text_opa = 255; const lv_font_t *font = nullptr; };
struct BtnSlot { lv_obj_t *btn, *icon_lbl, *text_lbl, *sensor_container, *sensor_lbl, *unit_lbl; };
struct DisplayProfile {};
constexpr int LV_PART_MAIN=0, LV_OPA_70=178, LV_OBJ_FLAG_HIDDEN=1, LV_ALIGN_BOTTOM_LEFT=0, LV_ALIGN_TOP_LEFT=1;
constexpr size_t HA_SHORT_STATE_MAX_LEN=64;
lv_font_t font;
int padding_captures=0;
void capture_card_padding(lv_obj_t *btn) { ++padding_captures; if (!btn->padding) btn->padding=16; }
const lv_font_t *display_small_text_font(const DisplayProfile &, const lv_font_t *fallback) { return fallback; }
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *, int) { return &font; }
void lv_obj_set_style_text_font(lv_obj_t *obj, const lv_font_t *value, int) { obj->font=value; }
void lv_obj_set_style_text_opa(lv_obj_t *obj, int value, int) { obj->text_opa=value; }
void lv_obj_set_style_text_color(lv_obj_t *obj, int value, int) { obj->text_color=value; }
int lv_obj_get_style_text_color(lv_obj_t *obj, int) { return obj->text_color; }
void lv_obj_set_style_pad_bottom(lv_obj_t *, int, int) {}
void lv_obj_clear_flag(lv_obj_t *o, int flag) { o->flags &= ~flag; }
void lv_obj_add_flag(lv_obj_t *o, int flag) { o->flags |= flag; }
void lv_obj_align(lv_obj_t *o, int, int, int y) { o->y=y; }
bool lv_obj_is_valid(lv_obj_t *o) { return o && o->valid; }
void lv_label_set_display_text(lv_obj_t *o, const char *text) { o->text=text; }
const char *find_icon(const char *name) { return name; }
const char *espcontrol_i18n(const char *text) { return text; }
uint32_t generation=1;
uint32_t ha_subscription_generation() { return generation; }
void notify_dashboard_content_changed() {}
namespace esphome { using StringRef = std::string; }
std::string string_ref_limited(const std::string &s, size_t n) { return s.substr(0,n); }
using Callback=std::function<void(esphome::StringRef)>;
std::map<std::string, Callback> subscriptions;
std::map<std::string, std::vector<Callback>> channels;
void ha_subscribe_state(const std::string &entity, Callback cb) { subscriptions["state"]=cb; channels[entity].push_back(cb); }
void ha_subscribe_attribute(const std::string &entity, const std::string &attribute, Callback cb) { subscriptions[attribute]=cb; channels[entity+":"+attribute].push_back(cb); }
void emit(const std::string &channel, const std::string &value) { for (auto &cb:channels.at(channel)) cb(value); }
#include "button_grid_rainfall.h"
int main() {
  using espcontrol::rainfall::amount;
  using espcontrol::rainfall::value;
  assert(amount(0,"mm")=="0 mm");
  assert(amount(2.4,"mm")=="2.4 mm");
  assert(amount(0.001,"in")=="0.001 in");
  assert(amount(2.3456,"mm")=="2.346 mm");
  for (auto text:{"", "unknown", "unavailable", "nan", "inf", "2mm", "-1"})
    assert(amount(value(text),"mm")=="--");
  assert(amount(3,"unknown")=="3");
  lv_obj_t button,icon,label,container,reading,unit;
  button.padding=0; button.text_color=0x212121;
  BtnSlot slot{&button,&icon,&label,&container,&reading,&unit};
  rainfall_setup(slot, {});
  assert(button.padding==16 && label.text=="Rainfall" && icon.text=="Weather Rainy");
  assert(reading.font==&font && reading.text_opa==LV_OPA_70 && reading.text_color==button.text_color);
  assert(unit.flags & LV_OBJ_FLAG_HIDDEN);
  assert(label.y==-(font.line_height+2));
  subscribe_rainfall_sensor(slot,"sensor.rainfall_today");
  emit("sensor.rainfall_today:unit_of_measurement","mm");
  assert(reading.text=="--");
  emit("sensor.rainfall_today","0"); assert(reading.text=="0 mm");
  emit("sensor.rainfall_today","2.40"); assert(reading.text=="2.4 mm");
  emit("sensor.rainfall_today:unit_of_measurement","in"); assert(reading.text=="2.4 in");
  emit("sensor.rainfall_today","unavailable"); assert(reading.text=="--");
  emit("sensor.rainfall_today","0.025"); assert(reading.text=="0.025 in");
  ++generation;
  emit("sensor.rainfall_today","10"); assert(reading.text=="0.025 in");
  --generation; reading.valid=false;
  emit("sensor.rainfall_today","20"); assert(reading.text=="0.025 in");
}
