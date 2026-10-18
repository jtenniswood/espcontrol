#pragma once
#include "page_alarm_state_runtime.h"
namespace espcontrol::pages {
struct AlarmCardOptions {
  alarm::Action mode;
  bool panel, status_icon, status_label, pin_arm, pin_disarm, theme_off;
};
enum class AlarmCardGlyph : uint8_t { ALARM, HOME, AWAY, NIGHT, VACATION, DISARM, TRIGGERED, COUNT };
inline const char* alarm_card_glyph_name(AlarmCardGlyph glyph) {
  switch (glyph) {
    case AlarmCardGlyph::HOME:
      return "Shield Home";
    case AlarmCardGlyph::AWAY:
      return "Shield Lock";
    case AlarmCardGlyph::NIGHT:
      return "Weather Night";
    case AlarmCardGlyph::VACATION:
      return "Airplane";
    case AlarmCardGlyph::DISARM:
      return "Shield Off";
    case AlarmCardGlyph::TRIGGERED:
      return "Alarm Light";
    default:
      return "Alarm";
  }
}
inline AlarmCardGlyph alarm_card_glyph(std::string_view state, std::string_view mode) {
  const auto effective = alarm::effective(state, mode);
  return effective == "armed_home"                                         ? AlarmCardGlyph::HOME
         : effective == "armed_away" || effective == "armed_custom_bypass" ? AlarmCardGlyph::AWAY
         : effective == "armed_night"                                      ? AlarmCardGlyph::NIGHT
         : effective == "armed_vacation"                                   ? AlarmCardGlyph::VACATION
         : effective == "disarmed"                                         ? AlarmCardGlyph::DISARM
         : effective == "triggered"                                        ? AlarmCardGlyph::TRIGGERED
                                                                           : AlarmCardGlyph::ALARM;
}
inline const char* alarm_card_action_label(alarm::Action mode) {
  switch (mode) {
    case alarm::Action::AWAY:
      return "Arm Away";
    case alarm::Action::HOME:
      return "Arm Home";
    case alarm::Action::NIGHT:
      return "Arm Night";
    case alarm::Action::VACATION:
      return "Arm Vacation";
    case alarm::Action::DISARM:
      return "Disarm";
    default:
      return "Alarm";
  }
}
inline std::string_view alarm_card_visible_caption(std::string_view history) {
  return history.substr(0, history.find('\0'));
}
inline uint32_t alarm_card_on_color(bool panel, bool triggered, uint32_t configured_on) {
  return panel && triggered ? 0xC62828 : configured_on;
}
}  // namespace espcontrol::pages
