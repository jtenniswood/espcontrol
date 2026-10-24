#include <cassert>
#include <string>

#include "media_volume_capability.h"

int main() {
  using espcontrol::media::SUPPORT_VOLUME_SET;
  using espcontrol::media::SUPPORT_VOLUME_STEP;
  using espcontrol::media::SUPPORT_VOLUME_MUTE;
  using espcontrol::media::VolumeCommandKind;
  using espcontrol::media::VolumeControlMode;
  using espcontrol::media::volume_arc_interactive;
  using espcontrol::media::volume_command;
  using espcontrol::media::volume_control_mode;
  using espcontrol::media::volume_decrease_enabled;
  using espcontrol::media::volume_display_value;
  using espcontrol::media::volume_increase_enabled;
  using espcontrol::media::parse_volume_mute_state;
  using espcontrol::media::volume_mute_supported;

  assert(!volume_mute_supported(false, SUPPORT_VOLUME_MUTE));
  assert(!volume_mute_supported(true, 0));
  assert(volume_mute_supported(true, SUPPORT_VOLUME_MUTE));
  assert(volume_mute_supported(
    true, SUPPORT_VOLUME_SET | SUPPORT_VOLUME_MUTE));

  bool muted = false;
  assert(parse_volume_mute_state("true", muted) && muted);
  assert(parse_volume_mute_state(" FALSE ", muted) && !muted);
  // Home Assistant's ESPHome bridge sends boolean attributes as on/off.
  assert(parse_volume_mute_state("on", muted) && muted);
  assert(parse_volume_mute_state(" OFF ", muted) && !muted);
  muted = true;
  for (const std::string value : {
         "", "none", "null", "unknown", "unavailable", "0"}) {
    assert(!parse_volume_mute_state(value, muted));
    assert(muted);  // Invalid input must not overwrite the last confirmed value.
  }

  using espcontrol::media::volume_mute_card_icon_name;
  assert(std::string(volume_mute_card_icon_name("Speaker")) == "Speaker Off");
  assert(std::string(volume_mute_card_icon_name("Speaker Multiple")) ==
         "Speaker Off");
  assert(std::string(volume_mute_card_icon_name("Volume High")) == "Volume Off");
  assert(std::string(volume_mute_card_icon_name("Music")) == "Volume Off");
  assert(std::string(volume_mute_card_icon_name("")) == "Volume Off");

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
