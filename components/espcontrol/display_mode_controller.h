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
  CAMERA,
  COVER_ART,
  DISPLAY_OFF,
};

enum class DisplayRequestSource : uint8_t {
  ONBOARDING,
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
         transition.target_mode == DisplayMode::CLOCK ||
         transition.target_mode == DisplayMode::CAMERA;
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

    // First-time setup must stay fully visible regardless of saved display
    // policy or the normal setup-screen burn-in timeout.
    if (request_active(DisplayRequestSource::ONBOARDING)) {
      if (apply_source(DisplayRequestSource::ONBOARDING, result)) return result;
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

  // Presentation effects may span several LVGL loop iterations. Track the
  // effect separately from the resolved policy so periodic reconciliation can
  // leave an unchanged transition running instead of restarting its fade.
  bool start_transition(const DisplayTransition &transition,
                        uint32_t started_ms) {
    if (!transition_is_current(transition.generation,
                               transition.target_mode)) {
      return false;
    }
    const DisplayTransition current = resolve();
    if (!same_transition(transition, current)) return false;
    if (transition_in_progress(transition)) return false;
    if (!transition_required(current) && !presentation_incomplete_) return false;

    in_flight_ = InFlightTransition{transition, started_ms, false};
    cancelled_generation_ = 0;
    presentation_incomplete_ = true;
    return true;
  }

  bool transition_in_progress(const DisplayTransition &transition) const {
    return in_flight_.has_value() &&
           same_transition(in_flight_->transition, transition);
  }

  bool has_transition_in_progress() const { return in_flight_.has_value(); }

  const DisplayTransition *in_flight_transition() const {
    return in_flight_ ? &in_flight_->transition : nullptr;
  }

  bool cancel_transition() {
    if (!in_flight_) return false;
    cancelled_generation_ = in_flight_->transition.generation;
    in_flight_.reset();
    // A stopped effect can be retried without any request changing. Advance
    // the policy generation in that case so a delayed completion from the
    // cancelled attempt can never match the retry.
    if (generation_ == cancelled_generation_) advance_generation();
    return true;
  }

  void require_presentation_cleanup() { presentation_incomplete_ = true; }
  bool presentation_incomplete() const { return presentation_incomplete_; }

  uint32_t transition_elapsed_ms(uint32_t now_ms) const {
    return in_flight_ ? now_ms - in_flight_->started_ms : 0;
  }

  bool transition_warning_due(uint32_t now_ms, uint32_t threshold_ms) {
    if (!in_flight_ || in_flight_->warning_emitted ||
        transition_elapsed_ms(now_ms) < threshold_ms) {
      return false;
    }
    in_flight_->warning_emitted = true;
    return true;
  }

  uint32_t last_completed_generation() const {
    return last_completed_generation_;
  }
  uint32_t last_transition_elapsed_ms() const {
    return last_transition_elapsed_ms_;
  }

  bool complete_transition(const DisplayTransition &transition) {
    return complete_transition(transition, 0);
  }

  bool complete_transition(const DisplayTransition &transition,
                           uint32_t completed_ms) {
    if (!generation_is_current(transition.generation)) return false;
    if (transition.generation == cancelled_generation_) return false;
    if (in_flight_ && !same_transition(in_flight_->transition, transition)) {
      return false;
    }
    const DisplayTransition current = resolve();
    if (!same_transition(transition, current)) return false;
    current_mode_ = transition.target_mode;
    current_source_ = transition.winning_source;
    current_takeover_ = transition.winning_takeover;
    last_completed_generation_ = transition.generation;
    last_transition_elapsed_ms_ =
        in_flight_ && completed_ms != 0
            ? completed_ms - in_flight_->started_ms
            : 0;
    in_flight_.reset();
    presentation_incomplete_ = false;
    return true;
  }

  bool transition_is_current(uint32_t generation, DisplayMode target_mode) const {
    if (!generation_is_current(generation)) return false;
    return resolve().target_mode == target_mode;
  }

  bool complete_transition(uint32_t generation, DisplayMode target_mode) {
    return complete_transition(generation, target_mode, 0);
  }

  bool complete_transition(uint32_t generation, DisplayMode target_mode,
                           uint32_t completed_ms) {
    if (!transition_is_current(generation, target_mode)) return false;
    return complete_transition(resolve(), completed_ms);
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
  DisplayMode target_mode() const { return resolve().target_mode; }
  bool target_mode_is(DisplayMode mode) const { return target_mode() == mode; }
  DisplayMode current_mode() const { return current_mode_; }
  bool current_mode_is(DisplayMode mode) const { return current_mode_ == mode; }
  bool target_source_is(DisplayRequestSource source) const {
    return resolve().winning_source == source;
  }
  bool current_source_is(DisplayRequestSource source) const {
    return current_source_ == source;
  }
  bool target_schedule_inactive() const {
    const DisplayTransition transition = resolve();
    return transition.target_mode != DisplayMode::ACTIVE &&
        (transition.winning_source == DisplayRequestSource::BOOT_GUARD ||
         transition.winning_source == DisplayRequestSource::SCREEN_SCHEDULE);
  }
  const std::optional<DisplayRequestSource> &current_source() const {
    return current_source_;
  }
  const std::optional<DisplayTakeoverKind> &current_takeover() const {
    return current_takeover_;
  }

  static bool request_is_valid(DisplayRequestSource source, DisplayMode mode) {
    switch (source) {
      case DisplayRequestSource::ONBOARDING:
        return mode == DisplayMode::ACTIVE;
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
               mode == DisplayMode::CAMERA || mode == DisplayMode::DISPLAY_OFF;
    }
    return false;
  }

 private:
  struct InFlightTransition {
    DisplayTransition transition{};
    uint32_t started_ms{0};
    bool warning_emitted{false};
  };

  struct Request {
    DisplayMode mode{DisplayMode::ACTIVE};
    uint32_t sequence{0};
    bool active{false};
  };

  static constexpr std::size_t kRequestCount = 9;
  static constexpr std::size_t kTakeoverCount = 2;

  static constexpr std::size_t source_index(DisplayRequestSource source) {
    return static_cast<std::size_t>(source);
  }

  static constexpr std::size_t takeover_index(DisplayTakeoverKind kind) {
    return static_cast<std::size_t>(kind);
  }

  static bool same_transition(const DisplayTransition &first,
                              const DisplayTransition &second) {
    return first.generation == second.generation &&
           first.previous_mode == second.previous_mode &&
           first.target_mode == second.target_mode &&
           first.winning_source == second.winning_source &&
           first.winning_takeover == second.winning_takeover;
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
  std::optional<InFlightTransition> in_flight_;
  uint32_t cancelled_generation_{0};
  uint32_t last_completed_generation_{0};
  uint32_t last_transition_elapsed_ms_{0};
  bool presentation_incomplete_{false};
};

inline const char *display_mode_name(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::ACTIVE: return "active";
    case DisplayMode::SETUP_DIMMED: return "setup_dimmed";
    case DisplayMode::DIMMED: return "dimmed";
    case DisplayMode::CLOCK: return "clock";
    case DisplayMode::CAMERA: return "camera";
    case DisplayMode::COVER_ART: return "cover_art";
    case DisplayMode::DISPLAY_OFF: return "display_off";
  }
  return "unknown";
}

inline const char *display_request_source_name(
    const std::optional<DisplayRequestSource> &source) {
  if (!source) return "default";
  switch (*source) {
    case DisplayRequestSource::ONBOARDING: return "onboarding";
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
