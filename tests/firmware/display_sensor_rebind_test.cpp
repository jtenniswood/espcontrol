#include <cassert>
#include <cmath>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <string_view>

#define ESP_LOGI(...) ((void) 0)
using lv_obj_t = int;
namespace esphome { using StringRef = std::string_view; }
constexpr uint32_t HA_SUBSCRIPTION_SCOPE_PHASE3 = 3;
using Callback = std::function<void(esphome::StringRef)>;
std::map<std::string, Callback> subscriptions;
std::map<std::string, std::string> retained;
void ha_reset_subscription_callbacks(uint32_t scope) {
  assert(scope == HA_SUBSCRIPTION_SCOPE_PHASE3);
  subscriptions.clear();
}
void ha_subscribe_state(const std::string &name, Callback callback, uint32_t) {
  subscriptions[name] = callback;
  if (retained.count(name)) callback(retained.at(name));
}
void lv_disp_trig_activity(void *) {}
bool parse_float_ref(esphome::StringRef, float &) { return false; }
template<typename... Args> bool configure_clock_bar_temperature_entities(Args...) { return false; }
template<typename... Args> void refresh_clock_bar_temperature_label_values(Args...) {}
#include "display_sensor_binding.h"

int main() {
  bool presence = true, schedule = true, playing = true;
  int schedule_changes = 0;
  auto rebind = [&](const std::string &prefix) {
    grid_phase3(false, false, "", "", "", nullptr, nullptr, nullptr, 0, nullptr,
                prefix.empty() ? "" : prefix + ".presence", &presence,
                prefix.empty() ? "" : prefix + ".schedule", &schedule,
                prefix.empty() ? "" : prefix + ".media", &playing,
                nullptr, nullptr, nullptr, [&]() { ++schedule_changes; });
  };
  // Clearing all entities must also clear their values and reevaluate schedule.
  subscriptions["old.presence"] = [&](esphome::StringRef) { presence = true; };
  rebind("");
  assert(!presence && !schedule && !playing && subscriptions.empty());
  assert(schedule_changes == 1);

  // Replacement states may arrive later. Old values/callbacks cannot survive.
  presence = schedule = playing = true;
  rebind("new");
  assert(!presence && !schedule && !playing);
  assert(subscriptions.size() == 3 && !subscriptions.count("old.presence"));
  assert(schedule_changes == 2);
  subscriptions.at("new.presence")("on");
  subscriptions.at("new.schedule")("on");
  subscriptions.at("new.media")("playing");
  assert(presence && schedule && playing);

  // Reset must precede subscriptions, which can immediately replay fresh state.
  retained = {{"new.presence", "on"}, {"new.schedule", "on"}, {"new.media", "playing"}};
  rebind("new");
  assert(presence && schedule && playing);
  retained.clear();
  rebind("");
  assert(!presence && !schedule && !playing);
}
