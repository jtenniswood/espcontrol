#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace esphome { using StringRef = std::string; }
struct lv_obj_t { std::string text; int flags = 0; unsigned color = 0; };
using lv_style_selector_t = int;
constexpr int LV_PART_MAIN = 0, LV_STATE_DEFAULT = 0;
constexpr int LV_OBJ_FLAG_HIDDEN = 1, LV_OBJ_FLAG_CLICKABLE = 2;
void lv_label_set_display_text(lv_obj_t *o, const char *s) { o->text = s; }
void lv_obj_add_flag(lv_obj_t *o, int f) { o->flags |= f; }
void lv_obj_clear_flag(lv_obj_t *o, int f) { o->flags &= ~f; }
unsigned lv_color_hex(unsigned c) { return c; }
void lv_obj_set_style_bg_color(lv_obj_t *o, unsigned c, int) { o->color = c; }
const char *find_icon(const char *name) { return name; }
constexpr size_t HA_SHORT_STATE_MAX_LEN = 64;
std::string string_ref_limited(esphome::StringRef s, size_t n) { return s.substr(0, n); }
int notifications = 0;
void notify_dashboard_content_changed() { ++notifications; }
uint32_t generation = 1;
uint32_t ha_subscription_generation() { return generation; }
using Callback = std::function<void(esphome::StringRef)>;
std::vector<Callback> callbacks;
std::string subscribed_entity;
void ha_subscribe_state(const std::string &entity, Callback callback) {
  subscribed_entity = entity;
  callbacks.push_back(callback);
}
struct BtnSlot { lv_obj_t *btn, *icon_lbl, *sensor_container, *text_lbl; };
struct ParsedCfg { std::string entity; };
struct CardPalette { bool has_sensor_color; unsigned sensor_val; };
namespace espcontrol::card_runtime { enum class CardDriverId { BATTERY, OTHER }; }
namespace espcontrol::cards {
struct Context { struct { card_runtime::CardDriverId driver; } runtime; };
}
#include "button_grid_battery_driver.h"

int main() {
  using namespace espcontrol::cards;
  Context context{{espcontrol::card_runtime::CardDriverId::BATTERY}};
  ParsedCfg config{"sensor.phone_battery"};
  CardPalette palette{true, 0x123456};
  lv_obj_t button, icon, sensor, label;
  button.flags = LV_OBJ_FLAG_CLICKABLE;
  BtnSlot slot{&button, &icon, &sensor, &label};
  assert(battery_driver_setup_visual(slot, config, context, palette));
  assert(icon.text == "Battery Unknown" && label.text == "--%");
  assert(button.color == palette.sensor_val && (sensor.flags & LV_OBJ_FLAG_HIDDEN));
  assert(battery_driver_attach_interaction(slot, config, context));
  assert(!(button.flags & LV_OBJ_FLAG_CLICKABLE));
  assert(battery_driver_bind_data(slot, config, context));
  assert(subscribed_entity == config.entity && callbacks.size() == 1);
  callbacks[0]("79.6");
  assert(label.text == "80%" && icon.text == "Battery 80%");
  for (const auto &state : {"", "unknown", "unavailable", "NaN", "inf", "1e100", "80junk", "80%"}) {
    callbacks[0](state);
    assert(icon.text == "Battery Unknown" && label.text == "--%");
  }
  callbacks[0]("-12");
  assert(label.text == "0%" && icon.text == "Battery Alert");
  callbacks[0]("1e30");
  assert(label.text == "100%" && icon.text == "Battery");
  callbacks[0]("94.5");
  assert(label.text == "95%" && icon.text == "Battery");
  assert(std::string(battery_icon_for_soc(5)) == "Battery Alert");
  assert(std::string(battery_icon_for_soc(6)) == "Battery 10%");
  assert(std::string(battery_icon_for_soc(14)) == "Battery 10%");
  assert(std::string(battery_icon_for_soc(15)) == "Battery 20%");
  assert(std::string(battery_icon_for_soc(94)) == "Battery 90%");
  // The shared grid lifecycle handles both surfaces and rejects old callbacks.
  ++generation;
  const int before = notifications;
  callbacks[0]("1");
  assert(label.text == "95%" && notifications == before);
  assert(battery_driver_cleanup(slot, config, context));
  assert(battery_driver_setup_visual(slot, config, context, palette));
  assert(battery_driver_bind_data(slot, config, context));
  callbacks.back()("20");
  assert(label.text == "20%" && notifications > before);
  assert(battery_driver_refresh_layout(slot, config, context));
  config.entity.clear();
  assert(battery_driver_bind_data(slot, config, context));
  assert(callbacks.size() == 2);
  context.runtime.driver = espcontrol::card_runtime::CardDriverId::OTHER;
  assert(!battery_driver_setup_visual(slot, config, context, palette));
  assert(!battery_driver_bind_data(slot, config, context));
}
