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
  PHOTOS,
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
         transition.target_mode == DisplayMode::CLOCK ||
         transition.target_mode == DisplayMode::PHOTOS;
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
               mode == DisplayMode::PHOTOS || mode == DisplayMode::DISPLAY_OFF;
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

inline const char *display_mode_name(DisplayMode mode) {
  switch (mode) {
    case DisplayMode::ACTIVE: return "active";
    case DisplayMode::SETUP_DIMMED: return "setup_dimmed";
    case DisplayMode::DIMMED: return "dimmed";
    case DisplayMode::CLOCK: return "clock";
    case DisplayMode::PHOTOS: return "photos";
    case DisplayMode::COVER_ART: return "cover_art";
    case DisplayMode::DISPLAY_OFF: return "display_off";
  }
  return "unknown";
}

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
