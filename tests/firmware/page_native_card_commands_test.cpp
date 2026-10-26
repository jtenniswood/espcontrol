#include <cassert>
#include <cstdlib>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "lvgl.h"
#include "page_card_commands.h"

extern "C" {
void lv_mem_init() {}
void lv_mem_deinit() {}
lv_mem_pool_t lv_mem_add_pool(void*, size_t) { return nullptr; }
void lv_mem_remove_pool(lv_mem_pool_t) {}
void* lv_malloc_core(size_t size) { return std::malloc(size); }
void lv_free_core(void* ptr) { std::free(ptr); }
void* lv_realloc_core(void* ptr, size_t size) { return std::realloc(ptr, size); }
void lv_mem_monitor_core(lv_mem_monitor_t* out) { *out = {}; }
lv_result_t lv_mem_test_core() { return LV_RESULT_OK; }
}

using namespace espcontrol::pages;
static bool online = true, api = true, busy = false, locked = false;
static std::string called, entity;
static bool screensaver = false;
static size_t calls = 0;
namespace esphome::api {
struct HomeassistantActionRequest {
  std::string service;
  bool is_event{false};
  std::vector<std::pair<std::string, std::string>> data;
};
}  // namespace esphome::api
static esphome::api::HomeassistantActionRequest event;
struct Coordinator {
  bool dispatching() const { return busy; }
};
Coordinator& ha_read_coordinator() {
  static Coordinator c;
  return c;
}
bool ha_api_available() { return api; }
bool ha_api_state_connected() { return online; }
namespace espcontrol {
struct Pin {
  bool locked() const { return ::locked; }
};
Pin& screensaver_pin() {
  static Pin pin;
  return pin;
}
}  // namespace espcontrol
void record(const char* command, const std::string& target) {
  called = command;
  entity = target;
  ++calls;
}
void send_toggle_action(const std::string& target) { record("toggle", target); }
void ha_send_entity_action(const std::string& target, const char* command) { record(command, target); }
void send_turn_on_action(const std::string& target) { record("on", target); }
void send_turn_off_action(const std::string& target) { record("off", target); }
void send_action_card_action(const ParsedCfg& p) { record("action", p.entity); }
void send_local_action(const std::string& target) { record("local", target); }
void send_internal_relay_action(const ParsedCfg& p) { record("relay", p.entity); }
void send_webhook_action(const ParsedCfg& p) { record("webhook", p.entity); }
std::function<void()>& force_screensaver_action() {
  static std::function<void()> action;
  return action;
}
void screen_lock_activate(bool value) {
  record("lock", {});
  screensaver = value;
}
bool ha_action_begin(esphome::api::HomeassistantActionRequest& r, const char* service, bool is_event,
                     size_t count) {
  if (!api) return false;
  r.service = service;
  r.is_event = is_event;
  r.data.reserve(count);
  return true;
}
void ha_action_add_data(esphome::api::HomeassistantActionRequest& r, const char* k, const char* v) {
  r.data.emplace_back(k, v);
}
bool ha_action_send(const esphome::api::HomeassistantActionRequest& r) {
  event = r;
  ++calls;
  return true;
}

#include "page_native_card_commands.h"

int main() {
  lv_init();
  auto* display = lv_display_create(480, 480);
  assert(display);
  bool valid = true;
  PageNativeCardCommandPort port(
      &valid, [](void* owner, PageBindingToken t) { return *static_cast<bool*>(owner) && t.binding == 1; });
  const PageBindingToken origin{0, 1, 0, 0, 0, false};
  assert(port.ready() && port.valid(origin) && port.connected() && port.service_available());
  locked = true;
  assert(!port.valid(origin));
  locked = false;
  valid = false;
  assert(!port.valid(origin));
  valid = true;
  online = false;
  api = false;
  assert(!port.connected() && !port.service_available());
  online = api = true;
  busy = true;
  assert(!port.ready());
  busy = false;
  auto* button = lv_button_create(lv_screen_active());
  bool tested = false;
  struct Probe {
    PageNativeCardCommandPort* port;
    bool* tested;
  } probe{&port, &tested};
  lv_obj_add_event_cb(
      button,
      [](lv_event_t* e) {
        auto& p = *static_cast<Probe*>(lv_event_get_user_data(e));
        assert(!p.port->ready());
        *p.tested = true;
      },
      LV_EVENT_CLICKED, &probe);
  lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
  assert(tested && port.ready());
  PageCardCommandSnapshot snapshot;
  snapshot.origin = origin;
  snapshot.config.entity = "switch.test";
  snapshot.slot = 6;
  for (auto item : {std::pair{PageCardCommandKind::TOGGLE, "toggle"},
                    {PageCardCommandKind::BUTTON, "button.press"},
                    {PageCardCommandKind::TURN_ON, "on"},
                    {PageCardCommandKind::TURN_OFF, "off"},
                    {PageCardCommandKind::OPEN, "cover.open_cover"},
                    {PageCardCommandKind::CLOSE, "cover.close_cover"},
                    {PageCardCommandKind::ACTION, "action"},
                    {PageCardCommandKind::LOCAL, "local"},
                    {PageCardCommandKind::RELAY, "relay"},
                    {PageCardCommandKind::WEBHOOK, "webhook"}}) {
    snapshot.kind = item.first;
    port.dispatch(snapshot);
    assert(called == item.second && entity == "switch.test");
  }
  snapshot.kind = PageCardCommandKind::SCREENSAVER;
  force_screensaver_action() = [] { record("screensaver", {}); };
  port.dispatch(snapshot);
  assert(called == "screensaver");
  snapshot.kind = PageCardCommandKind::SCREEN_LOCK;
  snapshot.config.options = "screensaver_on_lock";
  port.dispatch(snapshot);
  assert(called == "lock" && screensaver);
  snapshot.config.options = "screensaver_on_lock=1";
  port.dispatch(snapshot);
  assert(!screensaver);
  snapshot.kind = PageCardCommandKind::PUSH;
  set_espcontrol_language("fr");
  for (int i = 0; i < 1000; ++i) {
    snapshot.root = true;
    snapshot.config.label = {};
    port.dispatch(snapshot);
    assert(event.is_event && event.service == "esphome.push_button_pressed" && event.data.size() == 2);
    assert(event.data[0].second == "Push 6" && event.data[1].second == "6");
    snapshot.root = false;
    port.dispatch(snapshot);
    assert(event.data.size() == 1 && event.data[0].second == "Appuyer");
    snapshot.config.label = "Custom";
    port.dispatch(snapshot);
    assert(event.data[0].second == "Custom");
  }
  force_screensaver_action() = {};
  set_espcontrol_language("en");
  lv_display_delete(display);
  lv_deinit();
}
