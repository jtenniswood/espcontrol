#pragma once

#include "power_flow_model.h"

namespace espcontrol::power {
// One card-owned snapshot. Separate state/unit arrivals never assume watts or
// turn missing data into zero. Retained HA subscriptions supply both channels.
struct CardData {
  Model model;
  std::array<std::string, 5> states{};
  std::array<std::string, 5> units{};
  void apply(size_t index) {
    if (index < 4) model.set_power(static_cast<Node>(index), states[index], units[index]);
    else model.set_soc(states[index], units[index]);
  }
  void state(size_t index, std::string_view value) {
    if (index >= states.size()) return;
    states[index] = value.size() <= 64 ? std::string(value) : "invalid";
    apply(index);
  }
  void unit(size_t index, std::string_view value) {
    if (index >= units.size()) return;
    units[index] = value.size() <= 8 ? std::string(value) : "invalid";
    apply(index);
  }
  void disconnect() {
    states = {};
    units = {};
    model.invalidate();
  }
};
} // namespace espcontrol::power
