#pragma once

#include <cstdint>

namespace espcontrol {

inline constexpr uint32_t DISPLAY_OFF_FADE_OUT_MS = 400;
inline constexpr uint32_t BACKLIGHT_FADE_SAMPLE_MS = 16;

// Keep the light's current brightness aligned with direct PWM samples so a
// replacement fade (or normal light transition) starts at the visible level.
// Internal samples must not publish a new user setting or write preferences.
template<typename Light, typename Output>
void apply_backlight_fade_level(Light *light, Output *output, float level) {
  auto call = light->make_call();
  call.set_state(level > 0.0f);
  call.set_brightness(level);
  call.set_transition_length(0);
  call.set_publish(false);
  call.set_save(false);
  call.perform();
  // ESPHome schedules its output write for the next light loop. Apply this
  // sample now as well, before a redraw can delay that loop.
  output->set_level(level);
}

// Sample brightness from elapsed time. A busy loop skips overdue samples
// instead of extending the fade by waiting for every intermediate step.
class BacklightFade {
 public:
  void start(float from, float to, uint32_t now_ms, uint32_t duration_ms) {
    from_ = from;
    to_ = to;
    started_ms_ = now_ms;
    duration_ms_ = duration_ms;
  }

  bool finished(uint32_t now_ms) const {
    return now_ms - started_ms_ >= duration_ms_;
  }

  float level(uint32_t now_ms) const {
    if (finished(now_ms)) return to_;
    const float progress = static_cast<float>(now_ms - started_ms_) / duration_ms_;
    return from_ + (to_ - from_) * progress;
  }

 private:
  float from_{0.0f};
  float to_{0.0f};
  uint32_t started_ms_{0};
  uint32_t duration_ms_{0};
};

}  // namespace espcontrol
