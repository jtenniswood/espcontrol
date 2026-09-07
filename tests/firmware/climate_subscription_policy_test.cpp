#include <cassert>

#include "climate_subscription_policy.h"

int main() {
  using namespace espcontrol::climate;

  constexpr auto default_tabs = "temperature|mode|preset|fan|swing";
  static_assert(configured_climate_tab_mask(default_tabs) ==
                (CLIMATE_TAB_TEMPERATURE | CLIMATE_TAB_MODE |
                 CLIMATE_TAB_PRESET | CLIMATE_TAB_FAN | CLIMATE_TAB_SWING));
  // A supported temperature fallback needs no optional current value, even
  // when temperature was not one of the configured tabs.
  SubscriptionCapabilities temperature_fallback{true, false, true, false, false};
  assert(required_optional_subscription_mask("fan", temperature_fallback) ==
         OPTIONAL_SUBSCRIPTION_FAN);

  SubscriptionCapabilities temperature_only{true, false, false, false, false};
  assert(required_optional_subscription_mask("temperature", temperature_only) == 0);

  SubscriptionCapabilities preset_fallback{false, false, true, true, true};
  assert(required_optional_subscription_mask("temperature", preset_fallback) ==
         OPTIONAL_SUBSCRIPTION_PRESET);

  SubscriptionCapabilities fan_fallback{false, false, false, true, true};
  assert(required_optional_subscription_mask("temperature", fan_fallback) ==
         OPTIONAL_SUBSCRIPTION_FAN);

  SubscriptionCapabilities swing_fallback{false, false, false, false, true};
  assert(required_optional_subscription_mask("temperature", swing_fallback) ==
         OPTIONAL_SUBSCRIPTION_SWING);

  SubscriptionCapabilities mode_fallback{false, true, true, true, true};
  assert(required_optional_subscription_mask("temperature", mode_fallback) == 0);

  SubscriptionCapabilities unsupported_configured_fan{
      false, false, true, false, false};
  assert(required_optional_subscription_mask(
             "temperature|fan", unsupported_configured_fan) ==
         (OPTIONAL_SUBSCRIPTION_FAN | OPTIONAL_SUBSCRIPTION_PRESET));

  SubscriptionCapabilities supported_configured_fan{
      false, false, true, true, false};
  assert(required_optional_subscription_mask(
             "temperature|fan", supported_configured_fan) ==
         OPTIONAL_SUBSCRIPTION_FAN);

  assert(configured_optional_subscription_mask("fan|fan|swing") ==
         (OPTIONAL_SUBSCRIPTION_FAN | OPTIONAL_SUBSCRIPTION_SWING));

  OptionalSubscriptionState state;
  assert(state.mark_required(OPTIONAL_SUBSCRIPTION_FAN));
  assert(state.pending == OPTIONAL_SUBSCRIPTION_FAN);
  assert(state.mark_required(OPTIONAL_SUBSCRIPTION_FAN));
  assert(state.pending == OPTIONAL_SUBSCRIPTION_FAN);
  assert(state.update_required(OPTIONAL_SUBSCRIPTION_FAN) ==
         OPTIONAL_SUBSCRIPTION_FAN);
  state.mark_subscribed(OPTIONAL_SUBSCRIPTION_FAN);
  assert(!state.mark_required(OPTIONAL_SUBSCRIPTION_FAN));
  assert(state.update_required(OPTIONAL_SUBSCRIPTION_FAN) == 0);

  state.mark_required(OPTIONAL_SUBSCRIPTION_SWING);
  state.clear_pending();  // Context deletion/rebuild drops delayed work.
  assert(state.pending == 0);
  state = OptionalSubscriptionState();  // A rebuilt card starts clean.
  assert(state.subscribed == 0 && state.pending == 0);
  return 0;
}
