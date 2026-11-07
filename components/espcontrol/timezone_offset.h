#pragma once

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <ctime>

struct TzPosixTransitionRule {
  int month;
  int week;
  int day;
  int seconds;
};

inline bool parse_tz_int(const char* text, size_t& idx, int& value) {
  if (!std::isdigit(static_cast<unsigned char>(text[idx]))) return false;
  value = 0;
  while (std::isdigit(static_cast<unsigned char>(text[idx]))) {
    value = value * 10 + (text[idx] - '0');
    idx++;
  }
  return true;
}

inline void skip_posix_tz_name(const char* text, size_t& idx) {
  if (text[idx] == '<') {
    idx++;
    while (text[idx] && text[idx] != '>') idx++;
    if (text[idx] == '>') idx++;
    return;
  }
  while (std::isalpha(static_cast<unsigned char>(text[idx]))) idx++;
}

inline bool parse_posix_time_seconds(const char* text, size_t& idx, int& seconds) {
  int sign = 1;
  if (text[idx] == '-') {
    sign = -1;
    idx++;
  } else if (text[idx] == '+') {
    idx++;
  }

  int hours = 0;
  if (!parse_tz_int(text, idx, hours)) return false;

  int minutes = 0;
  int secs = 0;
  if (text[idx] == ':') {
    idx++;
    if (!parse_tz_int(text, idx, minutes)) return false;
    if (text[idx] == ':') {
      idx++;
      if (!parse_tz_int(text, idx, secs)) return false;
    }
  }

  seconds = sign * (hours * 3600 + minutes * 60 + secs);
  return true;
}

inline bool parse_posix_m_rule(const char* text, size_t& idx, TzPosixTransitionRule& rule) {
  if (text[idx] != 'M') return false;
  idx++;
  if (!parse_tz_int(text, idx, rule.month) || text[idx] != '.') return false;
  idx++;
  if (!parse_tz_int(text, idx, rule.week) || text[idx] != '.') return false;
  idx++;
  if (!parse_tz_int(text, idx, rule.day)) return false;
  rule.seconds = 2 * 3600;
  if (text[idx] == '/') {
    idx++;
    if (!parse_posix_time_seconds(text, idx, rule.seconds)) return false;
  }
  return true;
}

inline bool parse_posix_tz_rule(const char* posix, int& std_offset_seconds, bool& has_dst,
                                int& dst_offset_seconds, TzPosixTransitionRule& start_rule,
                                TzPosixTransitionRule& end_rule) {
  size_t idx = 0;
  skip_posix_tz_name(posix, idx);
  if (!parse_posix_time_seconds(posix, idx, std_offset_seconds)) return false;

  has_dst = false;
  dst_offset_seconds = std_offset_seconds;
  if (!posix[idx]) return true;

  skip_posix_tz_name(posix, idx);
  if (posix[idx] != ',') {
    if (!parse_posix_time_seconds(posix, idx, dst_offset_seconds)) return false;
  } else {
    dst_offset_seconds = std_offset_seconds - 3600;
  }

  if (posix[idx] != ',') return true;
  idx++;
  if (!parse_posix_m_rule(posix, idx, start_rule) || posix[idx] != ',') return false;
  idx++;
  if (!parse_posix_m_rule(posix, idx, end_rule)) return false;
  has_dst = true;
  return true;
}

inline bool tz_is_leap_year(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

inline int tz_days_in_month(int year, int month) {
  static const int DAYS[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (month == 2 && tz_is_leap_year(year)) return 29;
  return DAYS[month];
}

inline int64_t tz_days_before_year(int year) {
  int64_t days = 0;
  for (int y = 1970; y < year; y++) days += tz_is_leap_year(y) ? 366 : 365;
  return days;
}

inline int64_t tz_epoch_utc(int year, int month, int day, int seconds_of_day) {
  int64_t days = tz_days_before_year(year);
  for (int m = 1; m < month; m++) days += tz_days_in_month(year, m);
  days += day - 1;
  return days * 86400 + seconds_of_day;
}

inline int tz_day_of_week(int year, int month, int day) {
  int64_t days = tz_days_before_year(year);
  for (int m = 1; m < month; m++) days += tz_days_in_month(year, m);
  days += day - 1;
  return static_cast<int>((days + 4) % 7);
}

inline int tz_transition_day_of_month(int year, const TzPosixTransitionRule& rule) {
  int first_dow = tz_day_of_week(year, rule.month, 1);
  int day = 1 + ((rule.day - first_dow + 7) % 7) + (rule.week - 1) * 7;
  int dim = tz_days_in_month(year, rule.month);
  if (rule.week == 5) {
    while (day + 7 <= dim) day += 7;
    if (day > dim) day -= 7;
  } else if (day > dim) {
    day -= 7;
  }
  return day;
}

inline int64_t tz_transition_utc_epoch(int year, const TzPosixTransitionRule& rule,
                                       int before_offset_seconds) {
  int day = tz_transition_day_of_month(year, rule);
  return tz_epoch_utc(year, rule.month, day, rule.seconds) + before_offset_seconds;
}

// POSIX rules are ROM-backed; conversion uses caller-owned scalar state only.
inline bool timezone_posix_offset_minutes(const char* posix, time_t epoch, int& offset_minutes) {
  if (!posix) return false;
  struct tm utc_tm;
  if (!gmtime_r(&epoch, &utc_tm)) return false;
  int std_offset_seconds = 0, dst_offset_seconds = 0;
  bool has_dst = false;
  TzPosixTransitionRule start_rule{}, end_rule{};
  if (!parse_posix_tz_rule(posix, std_offset_seconds, has_dst, dst_offset_seconds, start_rule, end_rule))
    return false;
  int offset_seconds = std_offset_seconds;
  if (has_dst) {
    const int year = utc_tm.tm_year + 1900;
    const int64_t start = tz_transition_utc_epoch(year, start_rule, std_offset_seconds);
    const int64_t end = tz_transition_utc_epoch(year, end_rule, dst_offset_seconds);
    const int64_t t = static_cast<int64_t>(epoch);
    const bool in_dst = start <= end ? t >= start && t < end : t >= start || t < end;
    if (in_dst) offset_seconds = dst_offset_seconds;
  }
  offset_minutes = -offset_seconds / 60;
  return true;
}
