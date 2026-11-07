#pragma once

#include "ha_state_limits.h"
#include "page_fan_mode.h"

namespace espcontrol::pages::cleaning {
enum class Mode : uint8_t {
  STATUS,
  START_STOP,
  START_DOCK,
  DOCK,
  PAUSE_RESUME,
  CLEAN_SPOT,
  LOCATE,
  CLEAN_AREA,
  START_MOWING
};
enum class Glyph : uint8_t { VACUUM, VACUUM_ALERT, VACUUM_OFF, VACUUM_DOCKED, MOWER, MOWER_OUTLINE, COUNT };
struct Options {
  Mode mode;
  bool mower, automatic_icon;
};
inline Mode mode(std::string_view value, bool mower) {
  if (value == "status") return Mode::STATUS;
  if (value == "dock" || (!mower && value == "vacuum.return_to_base")) return Mode::DOCK;
  if (value == "pause_resume") return Mode::PAUSE_RESUME;
  if (mower) return Mode::START_MOWING;
  if (value == "start_dock") return Mode::START_DOCK;
  if (value == "clean_spot") return Mode::CLEAN_SPOT;
  if (value == "locate") return Mode::LOCATE;
  if (value == "clean_area") return Mode::CLEAN_AREA;
  return Mode::START_STOP;
}
inline bool needs_state(Options options) {
  return options.mower || options.mode == Mode::STATUS || options.mode == Mode::START_STOP ||
         options.mode == Mode::START_DOCK || options.mode == Mode::PAUSE_RESUME;
}
inline bool automatic_icon(std::string_view icon, bool mower) {
  if (icon.empty() || icon == "Auto") return true;
  if (mower) return icon == "Lawnmower" || icon == "Robot Mower" || icon == "Robot Mower Outline";
  for (const auto name :
       {"Robot Vacuum", "Robot Vacuum Alert", "Robot Vacuum Off", "Robot Vacuum Variant",
        "Robot Vacuum Variant Alert", "Robot Vacuum Variant Off", "Vacuum", "Vacuum Outline"})
    if (icon == name) return true;
  return false;
}
inline const char* default_icon(Options options) {
  if (options.mower) return options.mode == Mode::DOCK ? "Robot Mower Outline" : "Robot Mower";
  return options.mode == Mode::DOCK         ? "Robot Vacuum Variant"
         : options.mode == Mode::CLEAN_SPOT ? "Vacuum"
         : options.mode == Mode::LOCATE     ? "Robot Vacuum Alert"
         : options.mode == Mode::CLEAN_AREA ? "Vacuum Outline"
                                            : "Robot Vacuum";
}
inline const char* default_label(Options options) {
  switch (options.mode) {
    case Mode::STATUS:
      return options.mower ? "Lawn Mower" : "Vacuum";
    case Mode::DOCK:
      return "Dock";
    case Mode::PAUSE_RESUME:
      return "Pause";
    case Mode::START_DOCK:
      return "Start / Dock";
    case Mode::CLEAN_SPOT:
      return "Spot Clean";
    case Mode::LOCATE:
      return "Locate";
    case Mode::CLEAN_AREA:
      return "Clean Area";
    default:
      return "Start";
  }
}
inline const char* glyph_name(Glyph glyph) {
  constexpr const char* names[] = {"Robot Vacuum",         "Robot Vacuum Alert", "Robot Vacuum Off",
                                   "Robot Vacuum Variant", "Robot Mower",        "Robot Mower Outline"};
  const auto index = static_cast<size_t>(glyph);
  return index < static_cast<size_t>(Glyph::COUNT) ? names[index] : names[0];
}
inline Glyph state_glyph(std::string_view state, bool mower) {
  if (mower)
    return state == "docked" || state == "returning" || state == "error" || state == "unavailable" ||
                   state == "unknown"
               ? Glyph::MOWER_OUTLINE
               : Glyph::MOWER;
  return state == "error"                               ? Glyph::VACUUM_ALERT
         : state == "unavailable" || state == "unknown" ? Glyph::VACUUM_OFF
         : state == "docked" || state == "returning"    ? Glyph::VACUUM_DOCKED
                                                        : Glyph::VACUUM;
}
inline const char* state_label(std::string_view state, bool mower) {
  if (state == "cleaning" && !mower) return "Cleaning";
  if (state == "mowing" && mower) return "Mowing";
  if (state == "docked") return "Docked";
  if (state == "error") return "Error";
  if (state == "idle" && !mower) return "Idle";
  if (state == "paused") return "Paused";
  if (state == "returning") return "Returning";
  if (state == "unavailable") return "Unavailable";
  if (state == "unknown") return "Unknown";
  return nullptr;
}
inline bool unavailable(std::string_view entity, std::string_view state) {
  state = state.substr(0, HA_SHORT_STATE_MAX_LEN);
  if (fan_mode_trim(state).empty() || fan_mode_equal(state, "unavailable")) return true;
  if (!fan_mode_equal(state, "unknown")) return false;
  for (const std::string_view prefix : {"button.", "input_button.", "select.", "input_select."})
    if (entity.size() > prefix.size() && entity.substr(0, prefix.size()) == prefix) return false;
  return true;
}
inline const char* service(Options options, std::string_view state) {
  if (options.mode == Mode::STATUS) return nullptr;
  if (options.mower) {
    if (state == "unknown" || state == "unavailable") return nullptr;
    return options.mode == Mode::DOCK                                ? "lawn_mower.dock"
           : options.mode == Mode::PAUSE_RESUME && state == "mowing" ? "lawn_mower.pause"
                                                                     : "lawn_mower.start_mowing";
  }
  switch (options.mode) {
    case Mode::START_STOP:
      return state == "cleaning" ? "vacuum.stop" : "vacuum.start";
    case Mode::START_DOCK:
      return state == "cleaning"                                         ? "vacuum.return_to_base"
             : state == "docked" || state == "idle" || state == "paused" ? "vacuum.start"
                                                                         : nullptr;
    case Mode::DOCK:
      return "vacuum.return_to_base";
    case Mode::PAUSE_RESUME:
      return state == "cleaning" ? "vacuum.pause" : state == "paused" ? "vacuum.start" : "vacuum.start_pause";
    case Mode::CLEAN_SPOT:
      return "vacuum.clean_spot";
    case Mode::LOCATE:
      return "vacuum.locate";
    case Mode::CLEAN_AREA:
      return "vacuum.clean_area";
    default:
      return nullptr;
  }
}
}  // namespace espcontrol::pages::cleaning
