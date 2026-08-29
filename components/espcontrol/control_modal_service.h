#pragma once

#include <cstdint>

enum class ControlModalKind {
  NONE,
  MEDIA_VOLUME,
  CLIMATE,
  SWITCH_CONFIRMATION,
  OPTION_SELECT,
  FAN_PRESET,
  FAN_CONTROL,
  NETWORK_STATUS,
  ALARM_PIN,
  ALARM_CONTROL,
  IMAGE_CARD,
  TODO_LIST,
  COVER_CONTROL,
  LIGHT_CONTROL,
  MEDIA_CONTROL,
};

using ControlModalCloseCallback = void (*)();

enum class ControlModalDismissPolicy {
  DISMISS,
  PRESERVE_DURING_DISPLAY_TAKEOVER,
};

template <typename Overlay>
struct ControlModalActiveState {
  ControlModalKind kind = ControlModalKind::NONE;
  Overlay *overlay = nullptr;
  ControlModalCloseCallback close_callback = nullptr;
  ControlModalDismissPolicy dismiss_policy = ControlModalDismissPolicy::DISMISS;
  uint32_t close_guard_until_ms = 0;
  bool closing = false;
};

template <typename Overlay>
struct ControlModalNestedActiveState {
  Overlay *overlay = nullptr;
  ControlModalCloseCallback close_callback = nullptr;
  bool closing = false;
};

// Owns modal lifecycle state separately from the LVGL widgets. The caller
// creates and deletes widgets, while this service makes opening, close guards,
// and re-entrant callbacks predictable and testable on the host.
template <typename Overlay>
class ControlModalStateService {
 public:
  using Active = ControlModalActiveState<Overlay>;
  using NestedActive = ControlModalNestedActiveState<Overlay>;

  Active &active() { return active_; }
  const Active &active() const { return active_; }
  NestedActive &nested_active() { return nested_active_; }
  const NestedActive &nested_active() const { return nested_active_; }

  void reset_active() { active_ = Active{}; }
  void clear_active(ControlModalKind kind) {
    if (active_.kind == kind) reset_active();
  }

  void set_active(ControlModalKind kind, Overlay *overlay,
                  ControlModalCloseCallback close_callback,
                  ControlModalDismissPolicy dismiss_policy) {
    active_.kind = kind;
    active_.overlay = overlay;
    active_.close_callback = close_callback;
    active_.dismiss_policy = dismiss_policy;
    active_.close_guard_until_ms = 0;
    active_.closing = false;
  }

  bool close_guard_active(uint32_t now_ms) const {
    return active_.close_guard_until_ms != 0 &&
           static_cast<int32_t>(now_ms - active_.close_guard_until_ms) < 0;
  }

  void block_close_for(uint32_t now_ms, uint32_t delay_ms) {
    if (active_.kind == ControlModalKind::NONE || delay_ms == 0) return;
    active_.close_guard_until_ms = now_ms + delay_ms;
  }

  bool begin_active_close(uint32_t now_ms, bool honor_close_guard,
                          ControlModalKind *closing_kind,
                          ControlModalCloseCallback *close_callback) {
    if (active_.kind == ControlModalKind::NONE || active_.closing ||
        (honor_close_guard && close_guard_active(now_ms))) {
      return false;
    }
    active_.closing = true;
    if (closing_kind != nullptr) *closing_kind = active_.kind;
    if (close_callback != nullptr) *close_callback = active_.close_callback;
    return true;
  }

  void reset_nested_menu() { nested_active_ = NestedActive{}; }
  void clear_nested_menu(Overlay *overlay) {
    if (nested_active_.overlay == overlay) reset_nested_menu();
  }

  bool begin_nested_close(Overlay **closing_overlay,
                          ControlModalCloseCallback *close_callback) {
    if (nested_active_.overlay == nullptr || nested_active_.closing) return false;
    nested_active_.closing = true;
    if (closing_overlay != nullptr) *closing_overlay = nested_active_.overlay;
    if (close_callback != nullptr) *close_callback = nested_active_.close_callback;
    return true;
  }

 private:
  Active active_{};
  NestedActive nested_active_{};
};
