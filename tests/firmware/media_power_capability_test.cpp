#include <cassert>

#include "media_power_capability.h"

int main() {
  using espcontrol::media::PowerCommand;
  using espcontrol::media::SUPPORT_TURN_OFF;
  using espcontrol::media::SUPPORT_TURN_ON;
  using espcontrol::media::power_command;
  using espcontrol::media::power_toggle_supported;
  using espcontrol::media::media_control_tab_count;

  assert(!power_toggle_supported(false, 0));
  assert(!power_toggle_supported(true, 0));
  assert(!power_toggle_supported(true, SUPPORT_TURN_ON));
  assert(!power_toggle_supported(true, SUPPORT_TURN_OFF));
  assert(power_toggle_supported(true, SUPPORT_TURN_ON | SUPPORT_TURN_OFF));
  assert(power_toggle_supported(
    true, SUPPORT_TURN_ON | SUPPORT_TURN_OFF | 4 | 1024));

  assert(media_control_tab_count(false, false) == 2);
  assert(media_control_tab_count(true, false) == 3);
  assert(media_control_tab_count(false, true) == 3);
  assert(media_control_tab_count(true, true) == 4);
  assert(media_control_tab_count(false, false, true) == 3);
  assert(media_control_tab_count(true, true, true) == 5);

  const int power_features = SUPPORT_TURN_ON | SUPPORT_TURN_OFF;
  assert(power_command(true, power_features, true, true, "off") ==
         PowerCommand::TURN_ON);
  assert(power_command(true, power_features, true, true, "on") ==
         PowerCommand::TURN_OFF);
  assert(power_command(true, power_features, true, true, "idle") ==
         PowerCommand::TURN_OFF);
  assert(power_command(true, power_features, true, true, "playing") ==
         PowerCommand::TURN_OFF);
  assert(power_command(true, power_features, true, true, "paused") ==
         PowerCommand::TURN_OFF);
  assert(power_command(true, power_features, false, true, "unknown") ==
         PowerCommand::NONE);
  assert(power_command(true, power_features, true, false, "unavailable") ==
         PowerCommand::NONE);
  assert(power_command(true, SUPPORT_TURN_ON, true, true, "off") ==
         PowerCommand::NONE);

  return 0;
}
