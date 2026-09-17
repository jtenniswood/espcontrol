#include "photo_metadata.h"
#include <cassert>
#include <functional>
#include <string>
#include <cstdint>

namespace esphome { using StringRef = std::string; }
namespace espcontrol { enum class DisplayMode { NORMAL, CAMERA }; }
using espcontrol::DisplayMode;
struct App {
  DisplayMode target = DisplayMode::NORMAL;
  App &display() { return *this; }
  bool target_mode_is(DisplayMode mode) { return target == mode; }
  DisplayMode current_mode() { return target; }
} espcontrol_app;
struct Setting { std::string state; } screensaver_metadata_entity;
struct Apply { int calls = 0; void execute(int) { ++calls; } } clock_overlay_apply;
std::string photo_metadata_subscribed_entity, photo_metadata_value;
uint32_t photo_metadata_subscription_generation = 0, generation = 1;
bool connected = true, subscribe_ok = true;
int subscriptions = 0, announcements = 0;
std::function<void(esphome::StringRef)> state_callback;
bool ha_api_state_connected() { return connected; }
uint32_t ha_subscription_generation() { return generation; }
void ha_release_callbacks_for_owner(void *) { state_callback = {}; }
struct HaCallbackOwnerScope { explicit HaCallbackOwnerScope(void *) {} };
bool ha_subscribe_state(const std::string &, std::function<void(esphome::StringRef)> callback) {
  ++subscriptions;
  if (!subscribe_ok) return false;
  state_callback = std::move(callback);
  return true;
}
void ha_reannounce_state_subscriptions() { ++announcements; }
std::string string_ref_limited(esphome::StringRef value, size_t size) { return value.substr(0, size); }
#define id(x) x
void refresh() {
#include "metadata_subscription.inc"
}
int main() {
  screensaver_metadata_entity.state = "sensor.photo";
  refresh();
  assert(subscriptions == 0);
  espcontrol_app.target = DisplayMode::CAMERA;
  refresh();
  assert(subscriptions == 1 && announcements == 1);
  state_callback("Paris");
  assert(photo_metadata_value == "Paris");
  const int calls = clock_overlay_apply.calls;
  state_callback("Paris");
  refresh();
  assert(clock_overlay_apply.calls == calls && subscriptions == 1);
  state_callback("unknown");
  assert(photo_metadata_value.empty() && clock_overlay_apply.calls == calls + 1);
  state_callback("Paris");
  auto stale = state_callback;
  screensaver_metadata_entity.state = "sensor.new_photo";
  stale("Old photo");
  assert(photo_metadata_value == "Paris");
  refresh();
  assert(photo_metadata_value.empty() && subscriptions == 2);
  stale("Old photo");
  assert(photo_metadata_value.empty());
  state_callback("New photo");
  connected = false;
  state_callback("Late photo");
  assert(photo_metadata_value == "New photo");
  refresh();
  assert(photo_metadata_value.empty() && !state_callback);
  connected = true;
  refresh();
  assert(subscriptions == 3);
  stale = state_callback;
  ++generation;
  stale("Old generation");
  assert(photo_metadata_value.empty());
  refresh();
  assert(subscriptions == 4);
  state_callback("Current photo");
  espcontrol_app.target = DisplayMode::NORMAL;
  refresh();
  assert(!state_callback && photo_metadata_value.empty());
  espcontrol_app.target = DisplayMode::CAMERA;
  subscribe_ok = false;
  refresh();
  assert(photo_metadata_subscribed_entity.empty());
  subscribe_ok = true;
  refresh();
  assert(state_callback && subscriptions == 6);
  screensaver_metadata_entity.state.clear();
  refresh();
  assert(!state_callback && photo_metadata_value.empty());
}
