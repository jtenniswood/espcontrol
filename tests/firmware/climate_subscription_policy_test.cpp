#include <cassert>
#include <fstream>
#include <iterator>
#include <string>

#include "climate_subscription_policy.h"

size_t count_occurrences(const std::string &text, const std::string &needle) {
  size_t count = 0;
  size_t position = 0;
  while ((position = text.find(needle, position)) != std::string::npos) {
    count++;
    position += needle.size();
  }
  return count;
}

int main(int argc, char **argv) {
  using namespace espcontrol::climate;

  constexpr auto default_tabs = "temperature|mode|preset|fan|swing";
  static_assert(configured_climate_tab_mask(default_tabs) ==
                (CLIMATE_TAB_TEMPERATURE | CLIMATE_TAB_MODE |
                 CLIMATE_TAB_PRESET | CLIMATE_TAB_FAN | CLIMATE_TAB_SWING));
  static_assert(initial_subscription_count(default_tabs) == 18);
  static_assert(initial_subscription_count("temperature") == 15);
  static_assert(50 + 5 * initial_subscription_count("temperature") == 125);

  assert(argc == 2);
  std::ifstream fixture_stream(argv[1]);
  assert(fixture_stream.good());
  std::string fixture((std::istreambuf_iterator<char>(fixture_stream)),
                      std::istreambuf_iterator<char>());
  assert(count_occurrences(fixture, "\"type\": \"climate_control\"") == 5);
  assert(count_occurrences(fixture, "\"climate_tabs\": \"temperature\"") == 5);
  assert(fixture.find("\"other_subscription_channels\": 50") != std::string::npos);
  assert(fixture.find("\"expected_initial_subscription_channels\": 125") !=
         std::string::npos);

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
  assert(state.take_required(OPTIONAL_SUBSCRIPTION_FAN) ==
         OPTIONAL_SUBSCRIPTION_FAN);
  state.mark_subscribed(OPTIONAL_SUBSCRIPTION_FAN);
  assert(!state.mark_required(OPTIONAL_SUBSCRIPTION_FAN));
  assert(state.take_required(OPTIONAL_SUBSCRIPTION_FAN) == 0);

  state.mark_required(OPTIONAL_SUBSCRIPTION_SWING);
  state.clear_pending();  // Context deletion/rebuild drops delayed work.
  assert(state.pending == 0);
  state = OptionalSubscriptionState();  // A rebuilt card starts clean.
  assert(state.subscribed == 0 && state.pending == 0);
  return 0;
}
