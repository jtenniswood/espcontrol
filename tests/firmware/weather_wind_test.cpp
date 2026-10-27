#include <cassert>
#include <functional>
#include <map>
#include <string>
#include <cstdint>
#include <vector>

struct lv_font_t { int line_height = 16; };
struct lv_obj_t { std::string text; bool valid = true; int flags = 1, y = 0, padding = 16; };
struct BtnSlot { lv_obj_t *btn, *icon_lbl, *text_lbl, *sensor_container, *sensor_lbl, *unit_lbl; };
struct DisplayProfile {};
constexpr int LV_PART_MAIN=0, LV_OBJ_FLAG_HIDDEN=1, LV_ALIGN_BOTTOM_LEFT=0, LV_ALIGN_TOP_LEFT=1;
constexpr size_t HA_SHORT_STATE_MAX_LEN=64;
lv_font_t font;
int padding_captures=0;
void capture_card_padding(lv_obj_t *btn) { ++padding_captures; if (!btn->padding) btn->padding=16; }
const lv_font_t *display_small_text_font(const DisplayProfile &, const lv_font_t *fallback) { return fallback; }
const lv_font_t *lv_obj_get_style_text_font(lv_obj_t *, int) { return &font; }
void lv_obj_set_style_text_font(lv_obj_t *, const lv_font_t *, int) {}
void lv_obj_set_style_text_opa(lv_obj_t *, int, int) {}
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
#include "button_grid_weather_wind.h"

int main() {
  using espcontrol::wind::direction;
  assert(direction("0")==0 && direction("360")==0 && direction("359")==0);
  assert(direction("22.49")==0 && direction("22.5")==1);
  for (int i=0;i<8;++i) assert(direction(std::to_string(i*45))==i);
  assert(direction("North West")==7 && direction("south-east")==3);
  assert(direction(" ne ")==1 && direction("SSW")==5 && direction("NNW")==0);
  for (auto text : {"", "unknown", "unavailable", "nan", "inf", "12x", "-1", "361"}) assert(direction(text)==-1);
  assert(espcontrol::wind::speed("0")=="0");
  assert(espcontrol::wind::speed("12.56")=="12.6");
  for (auto text : {"", "unknown", "nan", "inf", "-2", "20mph"}) assert(espcontrol::wind::speed(text)=="--");
  lv_obj_t button,icon,label,container,speed,unit;
  button.padding=0;
  BtnSlot slot{&button,&icon,&label,&container,&speed,&unit};
  weather_wind_layout(slot, {});
  assert(!(container.flags & LV_OBJ_FLAG_HIDDEN) && label.y < container.y);
  assert(unit.flags & LV_OBJ_FLAG_HIDDEN);
  assert(padding_captures==1 && button.padding==16);
  button.padding=24; weather_wind_layout(slot, {});
  assert(button.padding==24); // Preserve a subpage's explicit inset.
  subscribe_weather_wind(slot,"weather.home");
  assert(label.text=="--" && speed.text=="--");
  subscriptions["wind_bearing"]("225");
  assert(label.text=="South West" && icon.text=="Wind arrow-bottom-left");
  subscriptions["wind_speed"]("12.5");
  subscriptions["wind_speed_unit"]("km/h");
  assert(speed.text=="12.5 km/h");
  subscriptions["wind_speed_unit"]("mph");
  assert(speed.text=="12.5 mph");
  subscriptions["state"]("unavailable");
  assert(speed.text=="--" && label.text=="--" && icon.text=="Weather Windy");
  subscriptions["state"]("sunny");
  subscriptions["wind_speed"]("0");
  assert(speed.text=="0 mph");
  subscriptions["wind_bearing"]("unknown");
  assert(label.text=="--" && speed.text=="0 mph");
  ++generation;
  subscriptions["wind_bearing"]("N");
  assert(label.text=="--");
  --generation;
  label.valid=false;
  subscriptions["wind_bearing"]("E");
  assert(label.text=="--");
  label.valid=true;
  channels.clear();
  subscribe_weather_wind(slot, "sensor.direction", "sensor.speed", true);
  assert(channels.size()==3 && !channels.count("sensor.direction:wind_bearing"));
  emit("sensor.direction", "SE");
  emit("sensor.speed", "8.2");
  emit("sensor.speed:unit_of_measurement", "m/s");
  assert(label.text=="South East" && speed.text=="8.2 m/s");
  emit("sensor.direction", "unavailable");
  assert(label.text=="--" && speed.text=="8.2 m/s");
  emit("sensor.direction", "270");
  emit("sensor.speed", "unknown");
  assert(label.text=="West" && speed.text=="--");
  emit("sensor.speed", "0");
  assert(label.text=="West" && speed.text=="0 m/s");
  emit("sensor.speed:unit_of_measurement", "mph");
  assert(speed.text=="0 mph");
  ++generation;
  emit("sensor.direction", "NE");
  emit("sensor.speed", "15");
  assert(label.text=="West" && speed.text=="0 mph");
}
