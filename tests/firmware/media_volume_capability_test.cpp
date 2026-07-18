#include <cassert>

#include "media_volume_capability.h"

int main() {
  using espcontrol::media::SUPPORT_VOLUME_SET;
  using espcontrol::media::SUPPORT_VOLUME_STEP;
  using espcontrol::media::VolumeCommandKind;
  using espcontrol::media::VolumeControlMode;
  using espcontrol::media::volume_arc_interactive;
  using espcontrol::media::volume_command;
  using espcontrol::media::volume_control_mode;
  using espcontrol::media::volume_decrease_enabled;
  using espcontrol::media::volume_display_value;
  using espcontrol::media::volume_increase_enabled;

  // Preserve the existing absolute-volume behaviour until Home Assistant
  // publishes the entity's supported feature flags.
  assert(volume_control_mode(false, 0) == VolumeControlMode::ABSOLUTE);
  assert(volume_control_mode(true, SUPPORT_VOLUME_SET) ==
         VolumeControlMode::ABSOLUTE);
  assert(volume_control_mode(true, SUPPORT_VOLUME_STEP) ==
         VolumeControlMode::STEP);
  assert(volume_control_mode(true, SUPPORT_VOLUME_SET | SUPPORT_VOLUME_STEP) ==
         VolumeControlMode::ABSOLUTE);
  assert(volume_control_mode(true, 0) == VolumeControlMode::READ_ONLY);

  const auto absolute = volume_command(VolumeControlMode::ABSOLUTE, 30, 45, 100);
  assert(absolute.kind == VolumeCommandKind::SET_ABSOLUTE);
  assert(absolute.value == 45);

  const auto capped = volume_command(VolumeControlMode::ABSOLUTE, 30, 80, 40);
  assert(capped.kind == VolumeCommandKind::SET_ABSOLUTE);
  assert(capped.value == 40);

  assert(volume_command(VolumeControlMode::STEP, 30, 31, 100).kind ==
         VolumeCommandKind::STEP_UP);
  assert(volume_command(VolumeControlMode::STEP, 30, 29, 100).kind ==
         VolumeCommandKind::STEP_DOWN);
  assert(volume_command(VolumeControlMode::STEP, 40, 41, 40).kind ==
         VolumeCommandKind::NONE);
  assert(volume_command(VolumeControlMode::STEP, 45, 44, 40).kind ==
         VolumeCommandKind::STEP_DOWN);
  assert(volume_command(VolumeControlMode::STEP, 0, -1, 100).kind ==
         VolumeCommandKind::NONE);
  assert(volume_command(VolumeControlMode::STEP, 0, -1, 100, false).kind ==
         VolumeCommandKind::STEP_DOWN);
  assert(volume_command(VolumeControlMode::STEP, 0, 1, 40, false).kind ==
         VolumeCommandKind::STEP_UP);
  assert(volume_command(VolumeControlMode::READ_ONLY, 30, 31, 100).kind ==
         VolumeCommandKind::NONE);

  assert(volume_arc_interactive(VolumeControlMode::ABSOLUTE));
  assert(!volume_arc_interactive(VolumeControlMode::STEP));
  assert(!volume_arc_interactive(VolumeControlMode::READ_ONLY));
  assert(volume_display_value(VolumeControlMode::ABSOLUTE, 65, 40) == 40);
  assert(volume_display_value(VolumeControlMode::STEP, 65, 40) == 65);
  assert(volume_display_value(VolumeControlMode::READ_ONLY, 65, 40) == 65);
  assert(volume_decrease_enabled(VolumeControlMode::STEP, 30));
  assert(volume_decrease_enabled(VolumeControlMode::STEP, 0, false));
  assert(!volume_decrease_enabled(VolumeControlMode::ABSOLUTE, 0, false));
  assert(!volume_decrease_enabled(VolumeControlMode::READ_ONLY, 30));
  assert(volume_increase_enabled(VolumeControlMode::STEP, 39, 40));
  assert(!volume_increase_enabled(VolumeControlMode::STEP, 40, 40));

  return 0;
}
