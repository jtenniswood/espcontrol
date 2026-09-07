#pragma once

#include <cstdint>

namespace espcontrol {

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
