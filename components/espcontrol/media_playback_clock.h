#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string_view>

namespace espcontrol::media {
inline bool parse_fixed_int(const char* text, size_t len, size_t pos, size_t digits, int& out) {
  if (!text || pos + digits > len) return false;
  int value = 0;
  for (size_t i = 0; i < digits; i++) {
    char c = text[pos + i];
    if (c < '0' || c > '9') return false;
    value = value * 10 + (c - '0');
  }
  out = value;
  return true;
}
inline int64_t days_from_civil(int year, unsigned month, unsigned day) {
  year -= month <= 2;
  const int era = (year >= 0 ? year : year - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(year - era * 400);
  const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return static_cast<int64_t>(era) * 146097 + static_cast<int64_t>(doe) - 719468;
}
inline bool parse_timestamp(std::string_view value, int64_t& epoch) {
  const auto text = value.substr(0, 40);
  const char* s = text.data();
  size_t len = text.size();
  if (len < 19 || s[4] != '-' || s[7] != '-' || (s[10] != 'T' && s[10] != ' ')) return false;
  int year, month, day, hour, minute, second;
  if (!parse_fixed_int(s, len, 0, 4, year) || !parse_fixed_int(s, len, 5, 2, month) ||
      !parse_fixed_int(s, len, 8, 2, day) || !parse_fixed_int(s, len, 11, 2, hour) ||
      !parse_fixed_int(s, len, 14, 2, minute) || !parse_fixed_int(s, len, 17, 2, second)) {
    return false;
  }
  if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 ||
      second < 0 || second > 60) {
    return false;
  }

  size_t tz_pos = 19;
  while (tz_pos < len && s[tz_pos] != 'Z' && s[tz_pos] != '+' && s[tz_pos] != '-') tz_pos++;
  int offset_seconds = 0;
  if (tz_pos < len && (s[tz_pos] == '+' || s[tz_pos] == '-')) {
    int offset_hour, offset_minute;
    if (!parse_fixed_int(s, len, tz_pos + 1, 2, offset_hour) || tz_pos + 3 >= len || s[tz_pos + 3] != ':' ||
        !parse_fixed_int(s, len, tz_pos + 4, 2, offset_minute) || offset_hour > 23 || offset_minute > 59) {
      return false;
    }
    offset_seconds = (offset_hour * 60 + offset_minute) * 60;
    if (s[tz_pos] == '-') offset_seconds = -offset_seconds;
  }

  int64_t days = days_from_civil(year, static_cast<unsigned>(month), static_cast<unsigned>(day));
  int64_t seconds_since_epoch = days * 86400 + hour * 3600 + minute * 60 + second;
  seconds_since_epoch -= offset_seconds;
  if (seconds_since_epoch < 0) return false;
  epoch = static_cast<int64_t>(seconds_since_epoch);
  return true;
}
inline bool timestamp_milliseconds(std::string_view value, int64_t now_epoch, uint32_t now_ms,
                                   uint32_t& updated_ms) {
  int64_t epoch;
  if (!parse_timestamp(value, epoch) || now_epoch <= 0 || epoch <= 0 || epoch > now_epoch) return false;
  const auto seconds = static_cast<uint64_t>(now_epoch - epoch);
  const auto elapsed = seconds > UINT32_MAX / 1000 ? UINT32_MAX : static_cast<uint32_t>(seconds * 1000);
  updated_ms = now_ms - elapsed;
  return true;
}
inline float nonnegative_progress_number(std::string_view value) {
  char text[65];
  const auto size = std::min<size_t>(value.size(), sizeof(text) - 1);
  if (size) std::memcpy(text, value.data(), size);
  text[size] = '\0';
  char* end = nullptr;
  const auto parsed = std::strtof(text, &end);
  return end != text && std::isfinite(parsed) && parsed >= 0 ? parsed : 0;
}
// Bounded, configuration-owned playback time. Updates and reads use no timer,
// UI object or heap storage; one application-loop tick can refresh all views.
struct PlaybackClock {
  float duration{0}, position{0};
  uint32_t updated_ms{0}, reported_ms{0};
  bool has_duration{false}, has_position{false}, has_reported_time{false}, anchored{false}, playing{false},
      state_known{false};
  float seconds(uint32_t now) const {
    if (!has_position) return 0;
    float value = position;
    if (playing && anchored) value += static_cast<uint32_t>(now - updated_ms) / 1000.0f;
    if (has_duration && duration > 0 && value > duration) value = duration;
    return std::isfinite(value) && value >= 0 ? value : 0;
  }
  uint8_t percent(uint32_t now) const {
    if (!has_duration || duration <= 0) return 0;
    const auto position = seconds(now);
    const float legacy_percent = position * 100.0f / duration;
    if (std::isfinite(legacy_percent))
      return static_cast<uint8_t>(std::clamp(legacy_percent + 0.5f, 0.0f, 100.0f));
    const double bounded = static_cast<double>(position) * 100 / duration;
    return static_cast<uint8_t>(std::clamp(std::round(bounded), 0.0, 100.0));
  }
  void playback(bool next, uint32_t now) {
    if (!state_known) {
      state_known = true;
      playing = next;
      return;
    }
    if (next == playing) return;
    position = seconds(now);
    updated_ms = now;
    anchored = true;
    has_reported_time = false;
    reported_ms = 0;
    playing = next;
  }
  void duration_value(std::string_view value) {
    duration = nonnegative_progress_number(value);
    has_duration = duration > 0;
  }
  void position_value(std::string_view value, uint32_t now) {
    position = nonnegative_progress_number(value);
    has_position = true;
    updated_ms = has_reported_time ? reported_ms : now;
    anchored = true;
  }
  void timestamp_value(std::string_view value, int64_t epoch, uint32_t now) {
    has_reported_time = timestamp_milliseconds(value, epoch, now, reported_ms);
    if (!has_reported_time) reported_ms = 0;
    updated_ms = has_reported_time ? reported_ms : now;
    anchored = true;
  }
};
inline bool seek_seconds(float duration, uint8_t percent, int& output) {
  if (!std::isfinite(duration) || duration <= 0 || percent > 100) return false;
  const float rounded = duration * percent / 100.0f + 0.5f;
  if (!std::isfinite(rounded) || static_cast<double>(rounded) >= std::numeric_limits<int>::max())
    return false;
  output = static_cast<int>(rounded);
  return true;
}
inline void format_playback_time(float seconds, char* output, size_t capacity) {
  if (!output || !capacity) return;
  if (!std::isfinite(seconds) || seconds < 0) seconds = 0;
  const float rounded = seconds + 0.5f;
  if (!std::isfinite(rounded) || static_cast<double>(rounded) >= std::numeric_limits<int>::max()) {
    std::snprintf(output, capacity, "--:--");
    return;
  }
  const int total = static_cast<int>(rounded), hours = total / 3600, minutes = (total / 60) % 60,
            remainder = total % 60;
  if (hours > 0)
    std::snprintf(output, capacity, "%d:%02d:%02d", hours, minutes, remainder);
  else
    std::snprintf(output, capacity, "%d:%02d", minutes, remainder);
}
}  // namespace espcontrol::media
