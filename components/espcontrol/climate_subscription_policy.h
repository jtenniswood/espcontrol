#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace espcontrol::climate {

constexpr uint8_t OPTIONAL_SUBSCRIPTION_PRESET = 1u << 0;
constexpr uint8_t OPTIONAL_SUBSCRIPTION_FAN = 1u << 1;
constexpr uint8_t OPTIONAL_SUBSCRIPTION_SWING = 1u << 2;
constexpr uint8_t CLIMATE_TAB_TEMPERATURE = 1u << 0;
constexpr uint8_t CLIMATE_TAB_MODE = 1u << 1;
constexpr uint8_t CLIMATE_TAB_PRESET = 1u << 2;
constexpr uint8_t CLIMATE_TAB_FAN = 1u << 3;
constexpr uint8_t CLIMATE_TAB_SWING = 1u << 4;

struct SubscriptionCapabilities {
  bool temperature = false;
  bool hvac = false;
  bool preset = false;
  bool fan = false;
  bool swing = false;
};

constexpr bool subscription_tab_present(std::string_view tabs,
                                        std::string_view wanted) {
  size_t start = 0;
  while (start <= tabs.size()) {
    size_t end = tabs.find('|', start);
    if (end == std::string_view::npos) end = tabs.size();
    if (tabs.substr(start, end - start) == wanted) return true;
    if (end == tabs.size()) break;
    start = end + 1;
  }
  return false;
}

constexpr uint8_t configured_optional_subscription_mask(
    std::string_view tabs) {
  uint8_t mask = 0;
  if (subscription_tab_present(tabs, "preset")) {
    mask |= OPTIONAL_SUBSCRIPTION_PRESET;
  }
  if (subscription_tab_present(tabs, "fan")) {
    mask |= OPTIONAL_SUBSCRIPTION_FAN;
  }
  if (subscription_tab_present(tabs, "swing")) {
    mask |= OPTIONAL_SUBSCRIPTION_SWING;
  }
  return mask;
}

constexpr uint8_t configured_climate_tab_mask(std::string_view tabs) {
  uint8_t mask = 0;
  if (subscription_tab_present(tabs, "temperature")) mask |= CLIMATE_TAB_TEMPERATURE;
  if (subscription_tab_present(tabs, "mode")) mask |= CLIMATE_TAB_MODE;
  if (subscription_tab_present(tabs, "preset")) mask |= CLIMATE_TAB_PRESET;
  if (subscription_tab_present(tabs, "fan")) mask |= CLIMATE_TAB_FAN;
  if (subscription_tab_present(tabs, "swing")) mask |= CLIMATE_TAB_SWING;
  return mask;
}

constexpr uint8_t configured_optional_subscription_mask(uint8_t tabs) {
  uint8_t mask = 0;
  if ((tabs & CLIMATE_TAB_PRESET) != 0) mask |= OPTIONAL_SUBSCRIPTION_PRESET;
  if ((tabs & CLIMATE_TAB_FAN) != 0) mask |= OPTIONAL_SUBSCRIPTION_FAN;
  if ((tabs & CLIMATE_TAB_SWING) != 0) mask |= OPTIONAL_SUBSCRIPTION_SWING;
  return mask;
}

constexpr bool configured_tab_is_supported(
    uint8_t tabs, const SubscriptionCapabilities &capabilities) {
  return (capabilities.temperature && (tabs & CLIMATE_TAB_TEMPERATURE) != 0) ||
         (capabilities.hvac && (tabs & CLIMATE_TAB_MODE) != 0) ||
         (capabilities.preset && (tabs & CLIMATE_TAB_PRESET) != 0) ||
         (capabilities.fan && (tabs & CLIMATE_TAB_FAN) != 0) ||
         (capabilities.swing && (tabs & CLIMATE_TAB_SWING) != 0);
}

// Configured controls receive their current-value subscription immediately.
// If none of those controls is supported, mirror the modal's fallback order
// and activate the current value required by the selected fallback control.
constexpr uint8_t required_optional_subscription_mask(
    uint8_t tabs, const SubscriptionCapabilities &capabilities) {
  uint8_t mask = configured_optional_subscription_mask(tabs);
  if (configured_tab_is_supported(tabs, capabilities)) return mask;
  if (capabilities.temperature || capabilities.hvac) return mask;
  if (capabilities.preset) return mask | OPTIONAL_SUBSCRIPTION_PRESET;
  if (capabilities.fan) return mask | OPTIONAL_SUBSCRIPTION_FAN;
  if (capabilities.swing) return mask | OPTIONAL_SUBSCRIPTION_SWING;
  return mask;
}

constexpr uint8_t required_optional_subscription_mask(
    std::string_view tabs, const SubscriptionCapabilities &capabilities) {
  return required_optional_subscription_mask(
      configured_climate_tab_mask(tabs), capabilities);
}

struct OptionalSubscriptionState {
  uint8_t subscribed = 0;
  uint8_t pending = 0;

  bool mark_required(uint8_t required) {
    uint8_t missing = required & ~subscribed;
    pending |= missing;
    return missing != 0;
  }

  uint8_t update_required(uint8_t required) {
    pending = required & ~subscribed;
    return pending;
  }

  void mark_subscribed(uint8_t fields) {
    subscribed |= fields;
    pending &= ~fields;
  }

  void clear_pending() { pending = 0; }
};

}  // namespace espcontrol::climate
