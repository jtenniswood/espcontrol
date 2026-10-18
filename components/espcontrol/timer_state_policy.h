#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string_view>
namespace espcontrol::timer_state {
inline int parse_duration(std::string_view value) {
  // HA serializes timedeltas as H:MM:SS or N day(s), H:MM:SS.
  char text[96];
  if (value.empty() || value.size() >= sizeof(text)) return 0;
  std::memcpy(text, value.data(), value.size());
  text[value.size()] = '\0';
  int days = 0, offset = 0;
  if (value.find(',') != std::string_view::npos) {
    char unit[5] = {};
    if (sscanf(text, "%9d %4[a-z], %n", &days, unit, &offset) != 2 || offset == 0 || days < 0 ||
        (std::strcmp(unit, "day") != 0 && std::strcmp(unit, "days") != 0))
      return 0;
  }
  int h = 0, m = 0, s = 0, n = 0;
  if (sscanf(text + offset, "%9d:%2d:%2d%n", &h, &m, &s, &n) != 3 ||
      offset + n != static_cast<int>(value.size()) || h < 0 || m < 0 || m > 59 || s < 0 || s > 59)
    return 0;
  const int64_t total = int64_t(days) * 86400 + int64_t(h) * 3600 + m * 60 + s;
  return total <= INT32_MAX ? static_cast<int>(total) : 0;
}
inline int64_t parse_finish(std::string_view value) {
  char text[96];
  if (value.empty() || value.size() >= sizeof(text)) return 0;
  std::memcpy(text, value.data(), value.size());
  text[value.size()] = '\0';
  const char* s = text;
  int Y, Mo, D, h, mi, sec;
  int n = 0;
  if (sscanf(s, "%4d-%2d-%2dT%2d:%2d:%2d%n", &Y, &Mo, &D, &h, &mi, &sec, &n) != 6) return 0;
  if (Y < 1970 || Y > 9999 || Mo < 1 || Mo > 12 || D < 1 || D > 31 || h < 0 || h > 23 || mi < 0 || mi > 59 ||
      sec < 0 || sec > 59)
    return 0;
  const char* p = s + n;
  // Skip optional fractional seconds.
  if (*p == '.') {
    ++p;
    while (*p >= '0' && *p <= '9') ++p;
  }
  int tz_off = 0;
  if (*p == 'Z') {
    if (p[1] != '\0') return 0;
    tz_off = 0;
  } else if (*p == '+' || *p == '-') {
    int sign = (*p == '-') ? -1 : 1;
    int th = 0, tm = 0;
    int consumed = 0;
    if (sscanf(p + 1, "%2d:%2d%n", &th, &tm, &consumed) != 2 || p[1 + consumed] != '\0' || th > 23 || tm > 59)
      return 0;
    tz_off = sign * (th * 3600 + tm * 60);
  } else
    return 0;
  // timegm() isn't always available on ESP toolchain; compute UTC manually.
  // Days since 1970-01-01 using civil_from_days style algorithm.
  int y = (Mo <= 2) ? Y - 1 : Y;
  int era = (y >= 0 ? y : y - 399) / 400;
  unsigned yoe = (unsigned)(y - era * 400);
  unsigned doy = (153 * (Mo + (Mo > 2 ? -3 : 9)) + 2) / 5 + D - 1;
  unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097 + (long)doe - 719468;
  int64_t t = static_cast<int64_t>(days) * 86400 + h * 3600 + mi * 60 + sec - tz_off;
  return t;
}
inline void format_seconds(int secs, char* buf, size_t bufsz) {
  if (secs < 0) secs = 0;
  if (secs >= 3600) {
    // Drop seconds at or above one hour so the string stays short enough
    // to fit the card. Reverts to M:SS automatically once the timer drops
    // back below an hour.
    int h = secs / 3600;
    int m = (secs / 60) % 60;
    snprintf(buf, bufsz, "%d:%02d", h, m);
  } else {
    int m = secs / 60;
    int s = secs % 60;
    snprintf(buf, bufsz, "%d:%02d", m, s);
  }
}
}  // namespace espcontrol::timer_state
