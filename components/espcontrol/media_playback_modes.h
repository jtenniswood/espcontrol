#pragma once

#include <cctype>
#include <cstdint>
#include <string>

namespace espcontrol::media {

constexpr int SUPPORT_SHUFFLE_SET = 32768;
constexpr int SUPPORT_REPEAT_SET = 262144;

enum class RepeatMode : uint8_t {
  UNKNOWN = 0,
  OFF = 1,
  ALL = 2,
  ONE = 3,
};

inline bool shuffle_supported(bool supported_features_known,
                              int supported_features) {
  return supported_features_known &&
         (supported_features & SUPPORT_SHUFFLE_SET) != 0;
}

inline bool repeat_supported(bool supported_features_known,
                             int supported_features) {
  return supported_features_known &&
         (supported_features & SUPPORT_REPEAT_SET) != 0;
}

inline std::string media_mode_normalize(std::string value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  value = value.substr(start, end - start);
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

inline bool parse_shuffle_state(const std::string &raw, bool &enabled) {
  const std::string value = media_mode_normalize(raw);
  if (value == "true" || value == "on" || value == "yes" || value == "1") {
    enabled = true;
    return true;
  }
  if (value == "false" || value == "off" || value == "no" || value == "0") {
    enabled = false;
    return true;
  }
  return false;
}

inline RepeatMode parse_repeat_mode(const std::string &raw) {
  const std::string value = media_mode_normalize(raw);
  if (value == "off") return RepeatMode::OFF;
  if (value == "all") return RepeatMode::ALL;
  if (value == "one") return RepeatMode::ONE;
  return RepeatMode::UNKNOWN;
}

inline RepeatMode next_repeat_mode(RepeatMode current) {
  if (current == RepeatMode::OFF) return RepeatMode::ALL;
  if (current == RepeatMode::ALL) return RepeatMode::ONE;
  if (current == RepeatMode::ONE) return RepeatMode::OFF;
  return RepeatMode::UNKNOWN;
}

inline const char *repeat_mode_value(RepeatMode mode) {
  if (mode == RepeatMode::OFF) return "off";
  if (mode == RepeatMode::ALL) return "all";
  if (mode == RepeatMode::ONE) return "one";
  return nullptr;
}

struct MediaTransportLayout {
  int primary_size = 0;
  int mode_size = 0;
  int gap = 0;
  int total_width = 0;
  int start_x = 0;
};

inline int media_transport_scaled_px(int px, int short_side) {
  if (short_side < 1) short_side = 480;
  int scaled = px * short_side / 480;
  return scaled > 0 ? scaled : 1;
}

inline MediaTransportLayout media_transport_layout(int content_width,
                                                    int short_side,
                                                    bool show_shuffle,
                                                    bool show_repeat) {
  MediaTransportLayout layout;
  if (content_width < 1) return layout;
  const int mode_count = (show_shuffle ? 1 : 0) + (show_repeat ? 1 : 0);
  const int button_count = 3 + mode_count;
  layout.gap = media_transport_scaled_px(mode_count == 0 ? 16 : 12, short_side);
  const int minimum_gap = mode_count == 0 ? 12 : 8;
  if (layout.gap < minimum_gap) layout.gap = minimum_gap;
  layout.primary_size = media_transport_scaled_px(88, short_side);
  const int minimum_primary = media_transport_scaled_px(74, short_side);
  if (layout.primary_size < minimum_primary) layout.primary_size = minimum_primary;
  layout.mode_size = layout.primary_size * 3 / 4;

  const int gaps_width = layout.gap * (button_count - 1);
  layout.total_width = layout.primary_size * 3 +
                       layout.mode_size * mode_count + gaps_width;
  if (layout.total_width > content_width) {
    const int available = content_width - gaps_width;
    if (available <= 0) return MediaTransportLayout();
    layout.primary_size = available * 4 / (12 + mode_count * 3);
    layout.mode_size = layout.primary_size * 3 / 4;
    layout.total_width = layout.primary_size * 3 +
                         layout.mode_size * mode_count + gaps_width;
  }
  layout.start_x = (content_width - layout.total_width) / 2;
  if (layout.start_x < 0) layout.start_x = 0;
  return layout;
}

}  // namespace espcontrol::media
