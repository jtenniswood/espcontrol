#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <map>
#include <string>

namespace esphome {
using StringRef = std::string;
uint32_t now_ms = 100;
uint32_t millis() { return now_ms; }
}
struct lv_obj_t { std::string text; bool checked = false; };
struct lv_timer_t { void *data; };
struct lv_font_t {};
constexpr int LV_STATE_CHECKED = 1, LV_OBJ_FLAG_HIDDEN = 1, LV_PART_MAIN = 0;
int timers = 0;
lv_timer_t *lv_timer_create(void (*)(lv_timer_t *), uint32_t, void *p) { ++timers; return new lv_timer_t{p}; }
void lv_timer_del(lv_timer_t *t) { --timers; delete t; }
void lv_timer_set_repeat_count(lv_timer_t *, int) {}
void *lv_timer_get_user_data(lv_timer_t *t) { return t->data; }
void lv_label_set_display_text(lv_obj_t *o, const char *s) { o->text = s; }
const char *lv_label_get_text(lv_obj_t *o) { return o->text.c_str(); }
void lv_obj_add_state(lv_obj_t *o, int) { o->checked = true; }
void lv_obj_clear_state(lv_obj_t *o, int) { o->checked = false; }
int checked_state_updates = 0;
void set_card_checked_state(lv_obj_t *o, bool active) {
  ++checked_state_updates;
  o->checked = active;
}
void lv_obj_add_flag(lv_obj_t *, int) {}
void lv_obj_clear_flag(lv_obj_t *, int) {}
void lv_obj_set_style_text_font(lv_obj_t *, const lv_font_t *, int) {}
const char *espcontrol_i18n(const char *s) { return s; }
using Callback = std::function<void(esphome::StringRef)>;
std::map<std::string, Callback> callbacks;
void *released = nullptr;
struct HaCallbackOwnerScope { explicit HaCallbackOwnerScope(void *) {} };
void ha_release_callbacks_for_owner(void *p) { released = p; callbacks.clear(); }
void ha_subscribe_state(const std::string &, Callback cb) { callbacks["state"] = cb; }
void ha_subscribe_attribute(const std::string &, const std::string &attr, Callback cb) { callbacks[attr] = cb; }
bool connected = true;
bool ha_api_state_connected() { return connected; }
std::string action;
bool ha_send_entity_action(const std::string &, const char *s) { action = s; return connected; }
struct BtnSlot { lv_obj_t *icon_lbl, *sensor_container, *sensor_lbl, *unit_lbl, *text_lbl; };
struct ParsedCfg { std::string label; };
#include "button_grid_timer.h"

int main() {
  assert(timer_card_state_active_ref("active"));
  assert(!timer_card_state_active_ref("paused"));
  assert(!timer_card_state_active_ref("idle"));
  assert(!timer_card_state_active_ref("unavailable"));
  assert(parse_timer_hms("1:02:03") == 3723);
  assert(parse_timer_hms("1 day, 0:00:00") == 86400);
  assert(parse_timer_hms("2 days, 3:04:05") == 183845);
  assert(parse_timer_hms("-1 day, 0:00:00") == 0);
  assert(parse_timer_hms("1 week, 0:00:00") == 0);
  assert(parse_timer_hms("999999999 days, 0:00:00") == 0);
  assert(parse_timer_hms("999999999:00:00") == 0);
  assert(parse_timer_hms("1 day, 0:00:00junk") == 0);
  assert(parse_timer_hms("-1:00:00") == 0);
  assert(parse_timer_hms("0:99:00") == 0);
  assert(parse_timer_hms("0:00:10junk") == 0);
  assert(parse_iso8601_to_epoch("2026-05-08T12:34:56.123456-06:00") ==
         parse_iso8601_to_epoch("2026-05-08T18:34:56Z"));
  assert(parse_iso8601_to_epoch("2026-99-08T12:34:56Z") == 0);
  assert(parse_iso8601_to_epoch("2026-05-08T12:34:56") == 0);
  char text[16];
  format_timer_secs(3599, text, sizeof(text)); assert(std::string(text) == "59:59");
  format_timer_secs(3600, text, sizeof(text)); assert(std::string(text) == "1:00");
  assert(timer_parse_confirm_timeout("99") == 30);
  assert(timer_parse_confirm_timeout("0") == 3);
  lv_obj_t button, value, label;
  label.text = "Kitchen";
  auto *ctx = new TimerCardCtx;
  ctx->entity_id = "timer.kitchen"; ctx->btn = &button;
  ctx->value_lbl = &value; ctx->text_lbl = &label;
  subscribe_timer_card(ctx);
  callbacks["duration"]("0:05:00"); callbacks["state"]("idle");
  assert(value.text == "5:00");
  callbacks["duration"]("1 day, 0:00:00"); assert(value.text == "24:00");
  callbacks["remaining"]("2 days, 3:04:05"); callbacks["state"]("paused");
  assert(value.text == "51:04" && !button.checked);
  callbacks["duration"]("0:05:00"); callbacks["state"]("idle");
  handle_timer_card_click(ctx); assert(action == "timer.start");
  callbacks["remaining"]("0:05:00"); callbacks["state"]("active");
  esphome::now_ms += 2000; timer_card_refresh(ctx);
  assert(value.text == "4:58" && button.checked);
  assert(checked_state_updates > 0);
  // Reconnect: absolute finish time wins over the stale remaining attribute.
  ctx->finishes_at_epoch = ::time(nullptr) + 20;
  timer_card_refresh(ctx); assert(value.text == "0:20" || value.text == "0:19");
  callbacks["remaining"]("0:00:17"); callbacks["state"]("paused");
  esphome::now_ms += 2000; timer_card_refresh(ctx); assert(value.text == "0:17");
  action.clear(); handle_timer_card_click(ctx); assert(action == "timer.start");
  callbacks["state"]("active"); ctx->confirm_enabled = true;
  action.clear(); handle_timer_card_click(ctx);
  assert(action.empty() && label.text == "Confirm" && timers == 1);
  handle_timer_card_click(ctx);
  assert(action == "timer.cancel" && label.text == "Kitchen" && timers == 0);
  callbacks["state"]("idle"); assert(value.text == "5:00");
  callbacks["state"]("active");
  ctx->finishes_at_epoch = ::time(nullptr);
  callbacks["state"]("idle"); assert(value.text == "0:00");
  esphome::now_ms += 5001; timer_card_refresh(ctx); assert(value.text == "5:00");
  callbacks["state"]("unavailable"); action.clear(); handle_timer_card_click(ctx);
  assert(action.empty() && value.text == "--:--");
  callbacks["state"]("active"); connected = false;
  handle_timer_card_click(ctx); assert(action.empty());
  connected = true; handle_timer_card_click(ctx);
  ctx->tick_timer = lv_timer_create(timer_card_tick_cb, 250, ctx);
  assert(timers == 2);
  delete ctx;
  assert(released == ctx && callbacks.empty() && timers == 0);
}
