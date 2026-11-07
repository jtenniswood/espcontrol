#pragma once

#include <array>
#include <cstring>
#include <string_view>

#include "camera_refresh_policy.h"

namespace espcontrol::pages {
inline bool page_image_valid_state(std::string_view value) {
  return !value.empty() && value != "unknown" && value != "unavailable" && value != "None";
}
enum class PageImageEntityKind : uint8_t { UNSUPPORTED, CAMERA, IMAGE };
inline PageImageEntityKind page_image_entity_kind(std::string_view value) {
  return value.substr(0, 7) == "camera."  ? PageImageEntityKind::CAMERA
         : value.substr(0, 6) == "image." ? PageImageEntityKind::IMAGE
                                          : PageImageEntityKind::UNSUPPORTED;
}
inline bool page_image_unavailable(std::string_view state) {
  state = state.substr(0, 64);
  return state == "unknown" || state == "unavailable";
}
inline bool page_image_trigger_valid(std::string_view value) {
  const size_t prefix = value.substr(0, 14) == "binary_sensor." ? 14 : value.substr(0, 6) == "event." ? 6 : 0;
  if (!prefix || value.size() <= prefix) return false;
  for (size_t i = prefix; i < value.size(); ++i) {
    const char ch = value[i];
    if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_')) return false;
  }
  return true;
}
// Call with raw HA bytes; both existing callers limit activity/revision to 80
// bytes before applying policy. Explicit lengths preserve embedded NULs.
class PageImageActivity {
 public:
  bool observe(std::string_view value, bool binary, uint32_t epoch) {
    value = value.substr(0, 80);
    if (connection_ != epoch) {
      connection_ = epoch;
      baselined_ = false;
    }
    if (!page_image_valid_state(value) || (binary && value != "on" && value != "off")) {
      baselined_ = false;
      length_ = 0;
      return false;
    }
    const bool activated =
        baselined_ && (binary ? previous() == "off" && value == "on" : previous() != value);
    length_ = value.size();
    std::memcpy(previous_.data(), value.data(), length_);
    previous_[length_] = '\0';
    baselined_ = true;
    return activated;
  }
  bool on(uint32_t epoch) const {
    return baselined_ && connection_ == epoch && previous() == "on";
  }
  std::string_view previous() const {
    return {previous_.data(), length_};
  }
  bool baselined() const {
    return baselined_;
  }
  uint32_t connection() const {
    return connection_;
  }

 private:
  std::array<char, 81> previous_{};
  uint8_t length_{0};
  bool baselined_{false};
  uint32_t connection_{0};
};
class PageImageRevision {
 public:
  uint32_t latest{0}, tile_requested{0}, tile_applied{0}, modal_requested{0}, modal_applied{0};
  bool observe(std::string_view value) {
    value = value.substr(0, 80);
    if (!page_image_valid_state(value) || value == timestamp()) return false;
    length_ = value.size();
    std::memcpy(timestamp_.data(), value.data(), length_);
    timestamp_[length_] = '\0';
    ++latest;
    return true;
  }
  void invalidate() {
    length_ = 0;
  }
  bool tile_dirty() const {
    return latest != tile_applied;
  }
  bool modal_dirty() const {
    return latest != modal_applied;
  }
  std::string_view timestamp() const {
    return {timestamp_.data(), length_};
  }

 private:
  std::array<char, 81> timestamp_{};
  uint8_t length_{0};
};
}  // namespace espcontrol::pages
