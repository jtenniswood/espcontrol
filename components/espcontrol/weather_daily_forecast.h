#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>

namespace espcontrol::weather {
constexpr size_t MAX_DAYS = 7;
constexpr size_t MAX_SOURCES = 8;
constexpr size_t MAX_PAYLOAD_BYTES = 2048;
constexpr float MISSING = std::numeric_limits<float>::quiet_NaN();
struct Day {
  char date[11]{};
  char condition[33]{};
  float high = MISSING, low = MISSING, rain_probability = MISSING;
  float precipitation = MISSING, wind = MISSING, humidity = MISSING;
};
struct Forecast {
  std::array<Day, MAX_DAYS> days{};
  size_t count = 0;
  char today[11]{}, tomorrow[11]{};
  char temperature_unit[13]{}, precipitation_unit[13]{}, wind_unit[13]{};
};
inline bool valid_date(const std::string &value) {
  if (value.size() != 10 || value[4] != '-' || value[7] != '-') return false;
  for (size_t i = 0; i < 10; ++i)
    if (i != 4 && i != 7 && (value[i] < '0' || value[i] > '9')) return false;
  int y = std::atoi(value.substr(0, 4).c_str()), m = std::atoi(value.substr(5, 2).c_str());
  int d = std::atoi(value.substr(8, 2).c_str());
  const int lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  if (y < 1970 || m < 1 || m > 12 || d < 1) return false;
  return d <= lengths[m - 1] + (m == 2 && y % 4 == 0 && (y % 100 != 0 || y % 400 == 0));
}
inline int weekday(const char *date) {
  int y = std::atoi(date), m = std::atoi(date + 5), d = std::atoi(date + 8);
  const int t[] = {0,3,2,5,0,3,5,1,4,6,2,4};
  y -= m < 3;
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}
inline float number(const std::string &value) {
  if (value.empty()) return MISSING;
  char *end = nullptr;
  float n = std::strtof(value.c_str(), &end);
  return end == value.c_str() || *end != '\0' || !std::isfinite(n) ? MISSING : n;
}
inline bool fields(const std::string &line, std::string *out, size_t count) {
  size_t start = 0;
  for (size_t i = 0; i < count; ++i) {
    size_t end = line.find('|', start);
    if ((i + 1 == count) != (end == std::string::npos)) return false;
    size_t length = end == std::string::npos ? line.size() - start : end - start;
    if (length > 64) return false;
    out[i] = line.substr(start, length);
    start = end + 1;
  }
  return true;
}
inline bool parse(const std::string &payload, Forecast &out) {
  out = Forecast{};
  if (payload.size() > MAX_PAYLOAD_BYTES) return false;
  size_t end = payload.find('\n');
  if (end == std::string::npos) return false;
  std::string header[6];
  if (!fields(payload.substr(0, end), header, 6) || header[0] != "WF1" ||
      !valid_date(header[1]) || !valid_date(header[2])) return false;
  if (header[3].size() > 12 || header[4].size() > 12 || header[5].size() > 12) return false;
  std::strcpy(out.today, header[1].c_str()); std::strcpy(out.tomorrow, header[2].c_str());
  std::strcpy(out.temperature_unit, header[3].c_str());
  std::strcpy(out.precipitation_unit, header[4].c_str()); std::strcpy(out.wind_unit, header[5].c_str());
  for (size_t start = end + 1; start < payload.size(); start = end + 1) {
    end = payload.find('\n', start);
    if (end == std::string::npos) end = payload.size();
    if (end == start) continue;
    std::string values[8];
    if (!fields(payload.substr(start, end - start), values, 8) || !valid_date(values[0]) || values[1].size() > 32) return false;
    if (values[0] < header[1]) continue;
    if (out.count && values[0] <= out.days[out.count - 1].date) return false;
    if (out.count == MAX_DAYS) return false;
    Day &day = out.days[out.count++];
    std::strcpy(day.date, values[0].c_str()); std::strcpy(day.condition, values[1].c_str());
    day.high = number(values[2]); day.low = number(values[3]);
    if (std::fabs(day.high) > 500) day.high = MISSING;
    if (std::fabs(day.low) > 500) day.low = MISSING;
    day.rain_probability = number(values[4]); day.precipitation = number(values[5]);
    day.wind = number(values[6]); day.humidity = number(values[7]);
    if (day.rain_probability < 0 || day.rain_probability > 100) day.rain_probability = MISSING;
    if (day.humidity < 0 || day.humidity > 100) day.humidity = MISSING;
    if (day.precipitation < 0) day.precipitation = MISSING;
    if (day.wind < 0) day.wind = MISSING;
  }
  return out.count > 0;
}
// Width per day derives from the actual display font, rather than device size.
inline size_t visible_days(int width, int minimum_day_width, size_t available = MAX_DAYS,
                           int gap = 0) {
  if (available == 0) return 0;
  const int column_width = minimum_day_width > 0 ? minimum_day_width : 1;
  const int column_gap = std::max(0, gap);
  int count = (width + column_gap) / (column_width + column_gap);
  return std::min(available, std::min(MAX_DAYS, static_cast<size_t>(std::max(1, count))));
}
static_assert(sizeof(Forecast) < 640, "Keep one shared week below 640 bytes");
}  // namespace espcontrol::weather
