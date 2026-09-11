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
bool parse_float_ref(esphome::StringRef state, float &value) {
  if (state == "unknown" || state == "unavailable") return false;
  value = std::stof(std::string(state));
  return true;
}
template<typename... Args> bool configure_clock_bar_temperature_entities(Args...) { return false; }
float rendered_indoor = 0, rendered_outdoor = 0;
void refresh_clock_bar_temperature_label_values(lv_obj_t *, bool, bool, bool, float indoor, float outdoor) {
  rendered_indoor = indoor;
  rendered_outdoor = outdoor;
}
#include "display_sensor_binding.h"

int main() {
  bool presence = true, schedule = true, playing = true;
  int schedule_changes = 0;
  float indoor = 21, outdoor = 12;
  auto rebind = [&](const std::string &prefix) {
    grid_phase3(true, true, prefix.empty() ? "" : prefix + ".indoor",
                prefix.empty() ? "" : prefix + ".outdoor", "", &indoor, &outdoor, nullptr, 0, nullptr,
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
  assert(std::isnan(indoor) && std::isnan(outdoor));
  assert(std::isnan(rendered_indoor) && std::isnan(rendered_outdoor));

  // Replacement states may arrive later. Old values/callbacks cannot survive.
  presence = schedule = playing = true;
  indoor = 21; outdoor = 12;
  rebind("new");
  assert(!presence && !schedule && !playing);
  assert(subscriptions.size() == 5 && !subscriptions.count("old.presence"));
  assert(schedule_changes == 2);
  assert(std::isnan(indoor) && std::isnan(outdoor));
  assert(std::isnan(rendered_indoor) && std::isnan(rendered_outdoor));
  subscriptions.at("new.indoor")("unknown");
  subscriptions.at("new.outdoor")("unavailable");
  assert(std::isnan(indoor) && std::isnan(outdoor));
  subscriptions.at("new.indoor")("23");
  subscriptions.at("new.outdoor")("14");
  assert(indoor == 23 && outdoor == 14 && rendered_indoor == 23 && rendered_outdoor == 14);
  subscriptions.at("new.presence")("on");
  subscriptions.at("new.schedule")("on");
  subscriptions.at("new.media")("playing");
  assert(presence && schedule && playing);

  // Reset must precede subscriptions, which can immediately replay fresh state.
  retained = {{"new.presence", "on"}, {"new.schedule", "on"}, {"new.media", "playing"},
              {"new.indoor", "24"}, {"new.outdoor", "15"}};
  rebind("new");
  assert(presence && schedule && playing);
  assert(indoor == 24 && outdoor == 15 && rendered_indoor == 24 && rendered_outdoor == 15);
  retained.clear();
  rebind("");
  assert(!presence && !schedule && !playing);
  assert(std::isnan(indoor) && std::isnan(outdoor));
  assert(std::isnan(rendered_indoor) && std::isnan(rendered_outdoor));
}
