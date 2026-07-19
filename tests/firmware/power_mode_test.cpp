#include <cstdlib>

#include "power_mode.h"

using espcontrol::NetworkPowerAction;
using espcontrol::PowerMode;

int main() {
  if (espcontrol::parse_power_mode("Battery Saver") != PowerMode::BATTERY_SAVER) {
    return EXIT_FAILURE;
  }
  if (espcontrol::parse_power_mode("Normal") != PowerMode::NORMAL ||
      espcontrol::parse_power_mode("unexpected") != PowerMode::NORMAL) {
    return EXIT_FAILURE;
  }

  if (espcontrol::power_mode_network_action(PowerMode::NORMAL, false) !=
      NetworkPowerAction::REQUEST_HIGH_PERFORMANCE) {
    return EXIT_FAILURE;
  }
  if (espcontrol::power_mode_network_action(PowerMode::NORMAL, true) !=
      NetworkPowerAction::NONE) {
    return EXIT_FAILURE;
  }
  if (espcontrol::power_mode_network_action(PowerMode::BATTERY_SAVER, true) !=
      NetworkPowerAction::RELEASE_HIGH_PERFORMANCE) {
    return EXIT_FAILURE;
  }
  if (espcontrol::power_mode_network_action(PowerMode::BATTERY_SAVER, false) !=
      NetworkPowerAction::NONE) {
    return EXIT_FAILURE;
  }

  if (!espcontrol::power_mode_defer_display_work(PowerMode::BATTERY_SAVER, true) ||
      espcontrol::power_mode_defer_display_work(PowerMode::BATTERY_SAVER, false) ||
      espcontrol::power_mode_defer_display_work(PowerMode::NORMAL, true)) {
    return EXIT_FAILURE;
  }
  if (!espcontrol::power_mode_flush_deferred_work(PowerMode::NORMAL, true) ||
      espcontrol::power_mode_flush_deferred_work(PowerMode::BATTERY_SAVER, true)) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
