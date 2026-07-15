#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace espcontrol {

enum class DisplayMode : uint8_t {
  ACTIVE,
  SETUP_DIMMED,
  DIMMED,
  CLOCK,
  COVER_ART,
  DISPLAY_OFF,
};

enum class DisplayRequestSource : uint8_t {
  BOOT_GUARD,
  IDLE_TIMER,
  PRESENCE_SENSOR,
  SCREEN_SCHEDULE,
  MANUAL_SLEEP,
  MEDIA_PLAYBACK,
  SETUP_TIMEOUT,
  USER_WAKE,
};

enum class DisplayTakeoverKind : uint8_t {
  INTERACTIVE,
  CRITICAL,
};

struct DisplayTransition {
  DisplayMode previous_mode{DisplayMode::ACTIVE};
  DisplayMode target_mode{DisplayMode::ACTIVE};
  std::optional<DisplayRequestSource> winning_source;
  std::optional<DisplayTakeoverKind> winning_takeover;
  uint32_t generation{0};
};

inline bool presence_can_wake_display(const DisplayTransition &transition) {
  if (!transition.winning_source.has_value()) return false;
  const DisplayRequestSource source = *transition.winning_source;
  const bool automatic_screensaver =
      source == DisplayRequestSource::IDLE_TIMER ||
      source == DisplayRequestSource::PRESENCE_SENSOR;
  if (!automatic_screensaver) return false;
  return transition.target_mode == DisplayMode::DISPLAY_OFF ||
         transition.target_mode == DisplayMode::DIMMED ||
         transition.target_mode == DisplayMode::CLOCK;
}

class DisplayModeController {
 public:
  DisplayModeController() = default;

  bool request(DisplayRequestSource source, DisplayMode mode) {
    if (!request_is_valid(source, mode)) return false;

    Request &manual_sleep = requests_[source_index(DisplayRequestSource::MANUAL_SLEEP)];
    if (source == DisplayRequestSource::USER_WAKE && manual_sleep.active) return false;

    bool changed = false;
    if (source == DisplayRequestSource::MANUAL_SLEEP) {
      Request &user_wake = requests_[source_index(DisplayRequestSource::USER_WAKE)];
      if (user_wake.active) {
        user_wake.active = false;
        user_wake.sequence = ++sequence_;
        changed = true;
      }
    }

    Request &entry = requests_[source_index(source)];
    if (entry.active && entry.mode == mode) {
      if (changed) advance_generation();
      return changed;
    }
    entry.active = true;
    entry.mode = mode;
    entry.sequence = ++sequence_;
    advance_generation();
    return true;
  }

  bool clear(DisplayRequestSource source) {
    Request &entry = requests_[source_index(source)];
    if (!entry.active) return false;
    entry.active = false;
    entry.sequence = ++sequence_;
    advance_generation();
    return true;
  }

  bool begin_takeover(DisplayTakeoverKind kind) {
    uint16_t &depth = takeover_depth_[takeover_index(kind)];
    if (depth == UINT16_MAX) return false;
    ++depth;
    advance_generation();
    return true;
  }

  bool end_takeover(DisplayTakeoverKind kind) {
    uint16_t &depth = takeover_depth_[takeover_index(kind)];
    if (depth == 0) return false;
    --depth;
    advance_generation();
    return true;
  }

  bool takeover_active(DisplayTakeoverKind kind) const {
    return takeover_depth_[takeover_index(kind)] != 0;
  }

  bool request_active(DisplayRequestSource source) const {
    return requests_[source_index(source)].active;
  }

  DisplayTransition resolve() const {
    DisplayTransition result;
    result.previous_mode = current_mode_;
    result.generation = generation_;

    if (takeover_active(DisplayTakeoverKind::CRITICAL)) {
      result.target_mode = DisplayMode::ACTIVE;
      result.winning_takeover = DisplayTakeoverKind::CRITICAL;
      return result;
    }

    if (apply_source(DisplayRequestSource::MANUAL_SLEEP, result)) return result;
    if (apply_source(DisplayRequestSource::USER_WAKE, result)) return result;
    if (apply_source(DisplayRequestSource::BOOT_GUARD, result)) return result;
    if (apply_source(DisplayRequestSource::SCREEN_SCHEDULE, result)) return result;

    if (takeover_active(DisplayTakeoverKind::INTERACTIVE)) {
      result.target_mode = DisplayMode::ACTIVE;
      result.winning_takeover = DisplayTakeoverKind::INTERACTIVE;
      return result;
    }

    if (apply_source(DisplayRequestSource::MEDIA_PLAYBACK, result)) return result;
    if (apply_most_recent(DisplayRequestSource::IDLE_TIMER,
                          DisplayRequestSource::PRESENCE_SENSOR, result)) {
      return result;
    }
    if (apply_source(DisplayRequestSource::SETUP_TIMEOUT, result)) return result;

    result.target_mode = DisplayMode::ACTIVE;
    return result;
  }

  bool complete_transition(const DisplayTransition &transition) {
    if (!generation_is_current(transition.generation)) return false;
    const DisplayTransition current = resolve();
    if (transition.target_mode != current.target_mode ||
        transition.winning_source != current.winning_source ||
        transition.winning_takeover != current.winning_takeover) {
      return false;
    }
    current_mode_ = transition.target_mode;
    current_source_ = transition.winning_source;
    current_takeover_ = transition.winning_takeover;
    return true;
  }

  bool transition_is_current(uint32_t generation, DisplayMode target_mode) const {
    if (!generation_is_current(generation)) return false;
    return resolve().target_mode == target_mode;
  }

  bool complete_transition(uint32_t generation, DisplayMode target_mode) {
    if (!transition_is_current(generation, target_mode)) return false;
    return complete_transition(resolve());
  }

  bool transition_required(const DisplayTransition &transition) const {
    return transition.target_mode != current_mode_ ||
           transition.winning_source != current_source_ ||
           transition.winning_takeover != current_takeover_;
  }

  bool generation_is_current(uint32_t generation) const {
    return generation != 0 && generation == generation_;
  }

  uint32_t generation() const { return generation_; }
  DisplayMode current_mode() const { return current_mode_; }
  const std::optional<DisplayRequestSource> &current_source() const {
    return current_source_;
  }
  const std::optional<DisplayTakeoverKind> &current_takeover() const {
    return current_takeover_;
  }

  static bool request_is_valid(DisplayRequestSource source, DisplayMode mode) {
    switch (source) {
      case DisplayRequestSource::BOOT_GUARD:
      case DisplayRequestSource::MANUAL_SLEEP:
        return mode == DisplayMode::DISPLAY_OFF;
      case DisplayRequestSource::USER_WAKE:
        return mode == DisplayMode::ACTIVE;
      case DisplayRequestSource::MEDIA_PLAYBACK:
        return mode == DisplayMode::COVER_ART;
      case DisplayRequestSource::SETUP_TIMEOUT:
        return mode == DisplayMode::SETUP_DIMMED;
      case DisplayRequestSource::SCREEN_SCHEDULE:
        return mode == DisplayMode::ACTIVE || mode == DisplayMode::CLOCK ||
               mode == DisplayMode::DISPLAY_OFF;
      case DisplayRequestSource::IDLE_TIMER:
      case DisplayRequestSource::PRESENCE_SENSOR:
        return mode == DisplayMode::DIMMED || mode == DisplayMode::CLOCK ||
               mode == DisplayMode::DISPLAY_OFF;
    }
    return false;
  }

 private:
  struct Request {
    DisplayMode mode{DisplayMode::ACTIVE};
    uint32_t sequence{0};
    bool active{false};
  };

  static constexpr std::size_t kRequestCount = 8;
  static constexpr std::size_t kTakeoverCount = 2;

  static constexpr std::size_t source_index(DisplayRequestSource source) {
    return static_cast<std::size_t>(source);
  }

  static constexpr std::size_t takeover_index(DisplayTakeoverKind kind) {
    return static_cast<std::size_t>(kind);
  }

  bool apply_source(DisplayRequestSource source, DisplayTransition &result) const {
    const Request &entry = requests_[source_index(source)];
    if (!entry.active) return false;
    result.target_mode = entry.mode;
    result.winning_source = source;
    return true;
  }

  bool apply_most_recent(DisplayRequestSource first,
                         DisplayRequestSource second,
                         DisplayTransition &result) const {
    const Request &a = requests_[source_index(first)];
    const Request &b = requests_[source_index(second)];
    if (!a.active && !b.active) return false;
    return apply_source(!b.active || (a.active && a.sequence >= b.sequence) ? first : second,
                        result);
  }

  void advance_generation() {
    ++generation_;
    if (generation_ == 0) ++generation_;
  }

  std::array<Request, kRequestCount> requests_{};
  std::array<uint16_t, kTakeoverCount> takeover_depth_{};
  uint32_t sequence_{0};
  uint32_t generation_{1};
  DisplayMode current_mode_{DisplayMode::ACTIVE};
  std::optional<DisplayRequestSource> current_source_;
  std::optional<DisplayTakeoverKind> current_takeover_;
};

struct LegacyDisplayState {
  bool display_asleep{false};
  bool screen_schedule_asleep{false};
  bool backlight_manual_off{false};
  bool temporary_user_wake{false};
  bool display_off_active{false};
  bool dimmed_active{false};
  bool clock_showing{false};
  bool cover_art_active{false};
  bool interactive_takeover{false};
  bool critical_takeover{false};
  bool setup_screen{false};
};

struct LegacyModeResult {
  DisplayMode mode{DisplayMode::ACTIVE};
  bool stable{true};
};

inline LegacyModeResult derive_legacy_display_mode(const LegacyDisplayState &state) {
  if (state.critical_takeover || state.interactive_takeover) {
    const bool clean = !state.display_asleep && !state.display_off_active &&
                       !state.dimmed_active && !state.clock_showing &&
                       !state.cover_art_active;
    return {DisplayMode::ACTIVE, clean};
  }

  const unsigned visible_count = static_cast<unsigned>(state.display_off_active) +
                                 static_cast<unsigned>(state.dimmed_active) +
                                 static_cast<unsigned>(state.clock_showing) +
                                 static_cast<unsigned>(state.cover_art_active);
  const bool presentation_consistent = visible_count <= 1 &&
      (visible_count == 0 || state.display_asleep);

  if (state.display_off_active) return {DisplayMode::DISPLAY_OFF, presentation_consistent};
  if (state.dimmed_active) return {DisplayMode::DIMMED, presentation_consistent};
  if (state.clock_showing) return {DisplayMode::CLOCK, presentation_consistent};
  if (state.cover_art_active) return {DisplayMode::COVER_ART, presentation_consistent};
  if (state.display_asleep && state.setup_screen) return {DisplayMode::SETUP_DIMMED, true};
  if (state.display_asleep) return {DisplayMode::SETUP_DIMMED, false};
  return {DisplayMode::ACTIVE, presentation_consistent};
}

inline const char *display_mode_name(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::ACTIVE: return "active";
    case DisplayMode::SETUP_DIMMED: return "setup_dimmed";
    case DisplayMode::DIMMED: return "dimmed";
    case DisplayMode::CLOCK: return "clock";
    case DisplayMode::COVER_ART: return "cover_art";
    case DisplayMode::DISPLAY_OFF: return "display_off";
  }
  return "unknown";
}

struct DisplayShadowObservation {
  DisplayTransition decision;
  LegacyModeResult legacy;
  bool decision_changed{false};
  bool mismatch{false};
  bool mismatch_changed{false};
};

class DisplayModeShadowObserver {
 public:
  DisplayShadowObservation observe(const LegacyDisplayState &state,
                                   bool presence_owned_sleep = false,
                                   bool boot_guard_active = false,
                                   bool schedule_active_ui = false) {
    set_takeover(DisplayTakeoverKind::CRITICAL, state.critical_takeover,
                 critical_takeover_active_);
    set_takeover(DisplayTakeoverKind::INTERACTIVE, state.interactive_takeover,
                 interactive_takeover_active_);

    set_request(DisplayRequestSource::MANUAL_SLEEP, state.backlight_manual_off,
                DisplayMode::DISPLAY_OFF);
    set_request(DisplayRequestSource::USER_WAKE,
                state.temporary_user_wake && !state.backlight_manual_off,
                DisplayMode::ACTIVE);
    set_request(DisplayRequestSource::BOOT_GUARD, boot_guard_active,
                DisplayMode::DISPLAY_OFF);

    const bool schedule_visible = state.screen_schedule_asleep &&
                                  (state.display_off_active || state.clock_showing);
    const DisplayMode schedule_mode = state.clock_showing ? DisplayMode::CLOCK
                                                          : DisplayMode::DISPLAY_OFF;
    set_request(DisplayRequestSource::SCREEN_SCHEDULE,
                schedule_visible || schedule_active_ui,
                schedule_active_ui ? DisplayMode::ACTIVE : schedule_mode);
    set_request(DisplayRequestSource::MEDIA_PLAYBACK, state.cover_art_active,
                DisplayMode::COVER_ART);

    const bool automatic_sleep = state.display_asleep && !state.screen_schedule_asleep &&
                                 !state.backlight_manual_off && !state.cover_art_active &&
                                 (state.display_off_active || state.dimmed_active ||
                                  state.clock_showing);
    DisplayMode automatic_mode = DisplayMode::DISPLAY_OFF;
    if (state.dimmed_active) automatic_mode = DisplayMode::DIMMED;
    if (state.clock_showing) automatic_mode = DisplayMode::CLOCK;
    set_request(DisplayRequestSource::PRESENCE_SENSOR,
                automatic_sleep && presence_owned_sleep, automatic_mode);
    set_request(DisplayRequestSource::IDLE_TIMER,
                automatic_sleep && !presence_owned_sleep, automatic_mode);
    set_request(DisplayRequestSource::SETUP_TIMEOUT,
                state.display_asleep && state.setup_screen,
                DisplayMode::SETUP_DIMMED);

    DisplayShadowObservation observation;
    observation.decision = controller_.resolve();
    observation.legacy = derive_legacy_display_mode(state);
    observation.mismatch = !observation.legacy.stable ||
                           observation.decision.target_mode != observation.legacy.mode;
    observation.decision_changed = !has_previous_decision_ ||
        observation.decision.target_mode != previous_mode_ ||
        observation.decision.winning_source != previous_source_ ||
        observation.decision.winning_takeover != previous_takeover_;
    observation.mismatch_changed = !has_previous_decision_ ||
        observation.mismatch != previous_mismatch_ ||
        observation.legacy.mode != previous_legacy_mode_ ||
        observation.legacy.stable != previous_legacy_stable_;

    previous_mode_ = observation.decision.target_mode;
    previous_source_ = observation.decision.winning_source;
    previous_takeover_ = observation.decision.winning_takeover;
    previous_mismatch_ = observation.mismatch;
    previous_legacy_mode_ = observation.legacy.mode;
    previous_legacy_stable_ = observation.legacy.stable;
    has_previous_decision_ = true;
    controller_.complete_transition(observation.decision);
    return observation;
  }

 private:
  void set_request(DisplayRequestSource source, bool active, DisplayMode mode) {
    if (active) {
      controller_.request(source, mode);
    } else {
      controller_.clear(source);
    }
  }

  void set_takeover(DisplayTakeoverKind kind, bool active, bool &tracked) {
    if (active == tracked) return;
    if (active) {
      controller_.begin_takeover(kind);
    } else {
      controller_.end_takeover(kind);
    }
    tracked = active;
  }

  DisplayModeController controller_;
  bool critical_takeover_active_{false};
  bool interactive_takeover_active_{false};
  bool has_previous_decision_{false};
  DisplayMode previous_mode_{DisplayMode::ACTIVE};
  std::optional<DisplayRequestSource> previous_source_;
  std::optional<DisplayTakeoverKind> previous_takeover_;
  bool previous_mismatch_{false};
  DisplayMode previous_legacy_mode_{DisplayMode::ACTIVE};
  bool previous_legacy_stable_{true};
};

inline const char *display_request_source_name(
    const std::optional<DisplayRequestSource> &source) {
  if (!source) return "default";
  switch (*source) {
    case DisplayRequestSource::BOOT_GUARD: return "boot_guard";
    case DisplayRequestSource::IDLE_TIMER: return "idle_timer";
    case DisplayRequestSource::PRESENCE_SENSOR: return "presence_sensor";
    case DisplayRequestSource::SCREEN_SCHEDULE: return "screen_schedule";
    case DisplayRequestSource::MANUAL_SLEEP: return "manual_sleep";
    case DisplayRequestSource::MEDIA_PLAYBACK: return "media_playback";
    case DisplayRequestSource::SETUP_TIMEOUT: return "setup_timeout";
    case DisplayRequestSource::USER_WAKE: return "user_wake";
  }
  return "unknown";
}

inline const char *display_takeover_name(
    const std::optional<DisplayTakeoverKind> &takeover) {
  if (!takeover) return "none";
  return *takeover == DisplayTakeoverKind::CRITICAL ? "critical" : "interactive";
}

}  // namespace espcontrol
