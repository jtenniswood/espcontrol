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
  int button_size = 0;
  int gap = 0;
  int row_gap = 0;
  int total_width = 0;
  int total_height = 0;
  int first_row_width = 0;
  int first_row_start_x = 0;
  int second_row_width = 0;
  int second_row_start_x = 0;
  bool modes_on_second_row = false;
};

inline int media_transport_scaled_px(int px, int short_side) {
  if (short_side < 1) short_side = 480;
  int scaled = px * short_side / 480;
  return scaled > 0 ? scaled : 1;
}

inline MediaTransportLayout media_transport_layout(int content_width,
                                                    int short_side,
                                                    bool show_shuffle,
                                                    bool show_repeat,
                                                    bool stack_modes = false) {
  MediaTransportLayout layout;
  if (content_width < 1) return layout;
  const int mode_count = (show_shuffle ? 1 : 0) + (show_repeat ? 1 : 0);
  layout.modes_on_second_row = stack_modes && mode_count > 0;
  const int first_row_count = layout.modes_on_second_row ? 3 : 3 + mode_count;
  const int second_row_count = layout.modes_on_second_row ? mode_count : 0;
  const int widest_row_count = first_row_count > second_row_count
    ? first_row_count : second_row_count;
  layout.gap = media_transport_scaled_px(mode_count == 0 ? 16 : 12, short_side);
  const int minimum_gap = mode_count == 0 ? 12 : 8;
  if (layout.gap < minimum_gap) layout.gap = minimum_gap;
  layout.row_gap = media_transport_scaled_px(12, short_side);
  if (layout.row_gap < 8) layout.row_gap = 8;
  layout.button_size = media_transport_scaled_px(88, short_side);
  const int minimum_button = media_transport_scaled_px(74, short_side);
  if (layout.button_size < minimum_button) layout.button_size = minimum_button;

  const int widest_gaps_width = layout.gap * (widest_row_count - 1);
  const int widest_row_width = layout.button_size * widest_row_count +
                               widest_gaps_width;
  if (widest_row_width > content_width) {
    const int available = content_width - widest_gaps_width;
    if (available <= 0) return MediaTransportLayout();
    layout.button_size = available / widest_row_count;
  }
  layout.first_row_width = layout.button_size * first_row_count +
                           layout.gap * (first_row_count - 1);
  layout.first_row_start_x = (content_width - layout.first_row_width) / 2;
  if (layout.first_row_start_x < 0) layout.first_row_start_x = 0;
  if (second_row_count > 0) {
    layout.second_row_width = layout.button_size * second_row_count +
                              layout.gap * (second_row_count - 1);
    layout.second_row_start_x = (content_width - layout.second_row_width) / 2;
    if (layout.second_row_start_x < 0) layout.second_row_start_x = 0;
  }
  layout.total_width = layout.first_row_width > layout.second_row_width
    ? layout.first_row_width : layout.second_row_width;
  layout.total_height = layout.button_size;
  if (layout.modes_on_second_row) {
    layout.total_height += layout.row_gap + layout.button_size;
  }
  return layout;
}

}  // namespace espcontrol::media
