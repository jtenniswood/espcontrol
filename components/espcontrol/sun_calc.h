#ifndef ESPCONTROL_SUN_CALC_H
#define ESPCONTROL_SUN_CALC_H

#pragma once
#include <string>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <cstdio>

#if defined(USE_ESP_IDF)
#include <esp_sntp.h>
#endif

#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/components/network/util.h"

// ============================================================================
// Timezone coordinate and POSIX TZ lookup table
// ============================================================================
// Representative city lat/lon for sunrise/sunset calculation, plus POSIX TZ
// strings for DST-aware local time. Keep the POSIX names alphabetic and DST
// offsets explicit for embedded parser compatibility.

struct TzCoord { const char* tz; float lat; float lon; const char* posix_tz; };
struct TzUtcPoint { int year; int month; int day; int hour; int minute; };
struct TzUtcRange { TzUtcPoint start; TzUtcPoint end; };

static const TzCoord TZ_COORDS[] = {
  {"Pacific/Midway",                    28.21f, -177.38f, "SST11"},
  {"Pacific/Pago_Pago",               -14.27f, -170.70f, "SST11"},
  {"Pacific/Honolulu",                  21.31f, -157.86f, "HST10"},
  {"America/Adak",                      51.88f, -176.66f, "HST10HDT9,M3.2.0,M11.1.0"},
  {"America/Anchorage",                 61.22f, -149.90f, "AKST9AKDT8,M3.2.0,M11.1.0"},
  {"America/Juneau",                    58.30f, -134.42f, "AKST9AKDT8,M3.2.0,M11.1.0"},
  {"America/Los_Angeles",               34.05f, -118.24f, "PST8PDT7,M3.2.0,M11.1.0"},
  {"America/Vancouver",                 49.28f, -123.12f, "PST8PDT7,M3.2.0,M11.1.0"},
  {"America/Tijuana",                   32.51f, -117.04f, "PST8PDT7,M3.2.0,M11.1.0"},
  {"America/Denver",                    39.74f, -104.98f, "MST7MDT6,M3.2.0,M11.1.0"},
  {"America/Phoenix",                  33.45f, -112.07f, "MST7"},
  {"America/Edmonton",                  53.55f, -113.49f, "MST7MDT6,M3.2.0,M11.1.0"},
  {"America/Boise",                     43.62f, -116.21f, "MST7MDT6,M3.2.0,M11.1.0"},
  {"America/Chicago",                   41.88f,  -87.63f, "CST6CDT5,M3.2.0,M11.1.0"},
  {"America/Mexico_City",               19.43f,  -99.13f, "CST6"},
  {"America/Winnipeg",                  49.90f,  -97.14f, "CST6CDT5,M3.2.0,M11.1.0"},
  {"America/Guatemala",                 14.63f,  -90.51f, "CST6"},
  {"America/Costa_Rica",                 9.93f,  -84.08f, "CST6"},
  {"America/New_York",                  40.71f,  -74.01f, "EST5EDT4,M3.2.0,M11.1.0"},
  {"America/Toronto",                   43.65f,  -79.38f, "EST5EDT4,M3.2.0,M11.1.0"},
  {"America/Detroit",                   42.33f,  -83.05f, "EST5EDT4,M3.2.0,M11.1.0"},
  {"America/Havana",                    23.11f,  -82.37f, "CST5CDT4,M3.2.0/0,M11.1.0/1"},
  {"America/Bogota",                     4.71f,  -74.07f, "GMT5"},
  {"America/Lima",                     -12.05f,  -77.04f, "GMT5"},
  {"America/Jamaica",                   18.11f,  -77.30f, "EST5"},
  {"America/Panama",                     8.98f,  -79.52f, "EST5"},
  {"America/Halifax",                   44.65f,  -63.57f, "AST4ADT3,M3.2.0,M11.1.0"},
  {"America/Caracas",                   10.49f,  -66.88f, "GMT4"},
  {"America/Santiago",                 -33.45f,  -70.67f, "CLT4CLST3,M9.1.6/24,M4.1.6/24"},
  {"America/La_Paz",                   -16.50f,  -68.15f, "GMT4"},
  {"America/Manaus",                    -3.12f,  -60.02f, "GMT4"},
  {"America/Barbados",                  13.10f,  -59.61f, "AST4"},
  {"America/Puerto_Rico",              18.47f,  -66.11f, "AST4"},
  {"America/Santo_Domingo",            18.49f,  -69.93f, "AST4"},
  {"America/St_Johns",                  47.56f,  -52.71f, "NST3:30NDT2:30,M3.2.0,M11.1.0"},
  {"America/Sao_Paulo",               -23.55f,  -46.63f, "GMT3"},
  {"America/Argentina/Buenos_Aires",   -34.60f,  -58.38f, "GMT3"},
  {"America/Montevideo",              -34.88f,  -56.16f, "GMT3"},
  {"America/Paramaribo",                5.85f,  -55.17f, "GMT3"},
  {"Atlantic/South_Georgia",          -54.28f,  -36.51f, "GMT2"},
  {"Atlantic/Azores",                  38.72f,  -27.22f, "AZOT1AZOST0,M3.5.0/0,M10.5.0/1"},
  {"Atlantic/Cape_Verde",              14.93f,  -23.51f, "GMT1"},
  {"UTC",                               51.51f,   -0.13f, "UTC0"},
  {"Europe/London",                     51.51f,   -0.13f, "GMT0BST-1,M3.5.0/1,M10.5.0"},
  {"Europe/Dublin",                     53.35f,   -6.26f, "GMT0IST-1,M3.5.0/1,M10.5.0"},
  {"Europe/Lisbon",                     38.72f,   -9.14f, "WET0WEST-1,M3.5.0/1,M10.5.0"},
  {"Africa/Casablanca",                 33.57f,   -7.59f, "GMT-1"},
  {"Africa/Accra",                       5.56f,   -0.19f, "GMT0"},
  {"Atlantic/Reykjavik",               64.15f,  -21.94f, "GMT0"},
  {"Europe/Paris",                      48.86f,    2.35f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Berlin",                     52.52f,   13.40f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Rome",                       41.90f,   12.50f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Madrid",                     40.42f,   -3.70f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Amsterdam",                  52.37f,    4.90f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Brussels",                   50.85f,    4.35f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Vienna",                     48.21f,   16.37f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Zurich",                     47.38f,    8.54f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Stockholm",                  59.33f,   18.07f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Oslo",                       59.91f,   10.75f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Copenhagen",                 55.68f,   12.57f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Warsaw",                     52.23f,   21.01f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Prague",                     50.08f,   14.44f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Budapest",                   47.50f,   19.04f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Europe/Belgrade",                   44.79f,   20.47f, "CET-1CEST-2,M3.5.0,M10.5.0/3"},
  {"Africa/Lagos",                       6.45f,    3.39f, "WAT-1"},
  {"Africa/Tunis",                      36.81f,   10.17f, "CET-1"},
  {"Africa/Cairo",                      30.04f,   31.24f, "EET-2EEST-3,M4.5.5/0,M10.5.4/24"},
  {"Europe/Athens",                     37.98f,   23.73f, "EET-2EEST-3,M3.5.0/3,M10.5.0/4"},
  {"Europe/Bucharest",                  44.43f,   26.10f, "EET-2EEST-3,M3.5.0/3,M10.5.0/4"},
  {"Europe/Helsinki",                   60.17f,   24.94f, "EET-2EEST-3,M3.5.0/3,M10.5.0/4"},
  {"Europe/Kyiv",                       50.45f,   30.52f, "EET-2EEST-3,M3.5.0/3,M10.5.0/4"},
  {"Europe/Istanbul",                   41.01f,   28.98f, "GMT-3"},
  {"Africa/Johannesburg",             -26.20f,   28.05f, "SAST-2"},
  {"Africa/Nairobi",                    -1.29f,   36.82f, "EAT-3"},
  {"Asia/Jerusalem",                    31.77f,   35.22f, "IST-2IDT-3,M3.4.4/26,M10.5.0"},
  {"Asia/Amman",                        31.95f,   35.93f, "GMT-3"},
  {"Asia/Beirut",                       33.89f,   35.50f, "EET-2EEST-3,M3.5.0/0,M10.5.0/0"},
  {"Europe/Moscow",                     55.76f,   37.62f, "MSK-3"},
  {"Asia/Baghdad",                      33.31f,   44.37f, "GMT-3"},
  {"Asia/Riyadh",                       24.69f,   46.72f, "GMT-3"},
  {"Asia/Kuwait",                       29.38f,   47.98f, "GMT-3"},
  {"Asia/Qatar",                        25.29f,   51.53f, "GMT-3"},
  {"Africa/Addis_Ababa",                 9.01f,   38.75f, "EAT-3"},
  {"Asia/Tehran",                       35.69f,   51.39f, "GMT-3:30"},
  {"Asia/Dubai",                        25.20f,   55.27f, "GMT-4"},
  {"Asia/Muscat",                       23.59f,   58.54f, "GMT-4"},
  {"Asia/Baku",                         40.41f,   49.87f, "GMT-4"},
  {"Asia/Tbilisi",                      41.72f,   44.79f, "GMT-4"},
  {"Indian/Mauritius",                 -20.16f,   57.50f, "GMT-4"},
  {"Asia/Kabul",                        34.53f,   69.17f, "GMT-4:30"},
  {"Asia/Karachi",                      24.86f,   67.01f, "PKT-5"},
  {"Asia/Tashkent",                     41.30f,   69.28f, "GMT-5"},
  {"Asia/Yekaterinburg",                56.84f,   60.60f, "GMT-5"},
  {"Asia/Kolkata",                      28.61f,   77.21f, "IST-5:30"},
  {"Asia/Colombo",                       6.93f,   79.84f, "GMT-5:30"},
  {"Asia/Kathmandu",                    27.72f,   85.32f, "GMT-5:45"},
  {"Asia/Dhaka",                        23.81f,   90.41f, "GMT-6"},
  {"Asia/Almaty",                       43.24f,   76.95f, "GMT-5"},
  {"Asia/Rangoon",                      16.87f,   96.20f, "GMT-6:30"},
  {"Asia/Bangkok",                      13.76f,  100.50f, "GMT-7"},
  {"Asia/Jakarta",                      -6.21f,  106.85f, "WIB-7"},
  {"Asia/Ho_Chi_Minh",                  10.82f,  106.63f, "GMT-7"},
  {"Asia/Singapore",                     1.35f,  103.82f, "GMT-8"},
  {"Asia/Kuala_Lumpur",                  3.14f,  101.69f, "GMT-8"},
  {"Asia/Shanghai",                     31.23f,  121.47f, "CST-8"},
  {"Asia/Hong_Kong",                    22.32f,  114.17f, "HKT-8"},
  {"Asia/Taipei",                       25.03f,  121.57f, "CST-8"},
  {"Asia/Manila",                       14.60f,  120.98f, "PST-8"},
  {"Australia/Perth",                  -31.95f,  115.86f, "AWST-8"},
  {"Asia/Tokyo",                        35.68f,  139.69f, "JST-9"},
  {"Asia/Seoul",                        37.57f,  126.98f, "KST-9"},
  {"Asia/Pyongyang",                    39.02f,  125.75f, "KST-9"},
  {"Australia/Adelaide",               -34.93f,  138.60f, "ACST-9:30ACDT-10:30,M10.1.0,M4.1.0/3"},
  {"Australia/Darwin",                 -12.46f,  130.84f, "ACST-9:30"},
  {"Australia/Sydney",                 -33.87f,  151.21f, "AEST-10AEDT-11,M10.1.0,M4.1.0/3"},
  {"Australia/Melbourne",              -37.81f,  144.96f, "AEST-10AEDT-11,M10.1.0,M4.1.0/3"},
  {"Australia/Brisbane",               -27.47f,  153.03f, "AEST-10"},
  {"Australia/Hobart",                 -42.88f,  147.33f, "AEST-10AEDT-11,M10.1.0,M4.1.0/3"},
  {"Pacific/Guam",                      13.44f,  144.79f, "ChST-10"},
  {"Pacific/Port_Moresby",             -6.31f,  147.17f, "GMT-10"},
  {"Asia/Vladivostok",                  43.12f,  131.91f, "GMT-10"},
  {"Pacific/Noumea",                   -22.28f,  166.46f, "GMT-11"},
  {"Pacific/Norfolk",                  -29.05f,  167.96f, "NFT-11NFDT-12,M10.1.0,M4.1.0/3"},
  {"Asia/Magadan",                      59.56f,  150.80f, "GMT-11"},
  {"Pacific/Auckland",                 -36.85f,  174.76f, "NZST-12NZDT-13,M9.5.0,M4.1.0/3"},
  {"Pacific/Fiji",                     -18.14f,  178.44f, "GMT-12"},
  {"Pacific/Chatham",                  -43.88f, -176.46f, "CHAST-12:45CHADT-13:45,M9.5.0/2:45,M4.1.0/3:45"},
  {"Pacific/Tongatapu",               -21.21f, -175.15f, "GMT-13"},
  {"Pacific/Apia",                     -13.83f, -171.76f, "GMT-13"},
  {"Pacific/Kiritimati",                 1.87f, -157.47f, "GMT-14"},
};

static constexpr int TZ_COORDS_COUNT = sizeof(TZ_COORDS) / sizeof(TZ_COORDS[0]);

// Morocco pauses UTC+1 during Ramadan. POSIX TZ strings cannot represent these
// lunar-calendar transitions, so keep the known UTC transition windows explicit.
static const TzUtcRange CASABLANCA_UTC_PAUSES[] = {
  {{2024, 3, 10, 2, 0}, {2024, 4, 14, 2, 0}},
  {{2025, 2, 23, 2, 0}, {2025, 4, 6, 2, 0}},
  {{2026, 2, 15, 2, 0}, {2026, 3, 22, 2, 0}},
  {{2027, 2, 7, 2, 0}, {2027, 3, 14, 2, 0}},
  {{2028, 1, 23, 2, 0}, {2028, 3, 5, 2, 0}},
  {{2029, 1, 14, 2, 0}, {2029, 2, 18, 2, 0}},
  {{2029, 12, 30, 2, 0}, {2030, 2, 10, 2, 0}},
  {{2030, 12, 22, 2, 0}, {2031, 1, 26, 2, 0}},
  {{2031, 12, 14, 2, 0}, {2032, 1, 18, 2, 0}},
  {{2032, 11, 28, 2, 0}, {2033, 1, 9, 2, 0}},
  {{2033, 11, 20, 2, 0}, {2033, 12, 25, 2, 0}},
  {{2034, 11, 5, 2, 0}, {2034, 12, 17, 2, 0}},
  {{2035, 10, 28, 2, 0}, {2035, 12, 9, 2, 0}},
  {{2036, 10, 19, 2, 0}, {2036, 11, 23, 2, 0}},
  {{2037, 10, 4, 2, 0}, {2037, 11, 15, 2, 0}},
  {{2038, 9, 26, 2, 0}, {2038, 10, 31, 2, 0}},
  {{2039, 9, 18, 2, 0}, {2039, 10, 23, 2, 0}},
  {{2040, 9, 2, 2, 0}, {2040, 10, 14, 2, 0}},
  {{2041, 8, 25, 2, 0}, {2041, 9, 29, 2, 0}},
  {{2042, 8, 10, 2, 0}, {2042, 9, 21, 2, 0}},
  {{2043, 8, 2, 2, 0}, {2043, 9, 13, 2, 0}},
  {{2044, 7, 24, 2, 0}, {2044, 8, 28, 2, 0}},
  {{2045, 7, 9, 2, 0}, {2045, 8, 20, 2, 0}},
  {{2046, 7, 1, 2, 0}, {2046, 8, 5, 2, 0}},
  {{2047, 6, 23, 2, 0}, {2047, 7, 28, 2, 0}},
  {{2048, 6, 7, 2, 0}, {2048, 7, 19, 2, 0}},
  {{2049, 5, 30, 2, 0}, {2049, 7, 4, 2, 0}},
  {{2050, 5, 15, 2, 0}, {2050, 6, 26, 2, 0}},
};

static constexpr int CASABLANCA_UTC_PAUSE_COUNT =
    sizeof(CASABLANCA_UTC_PAUSES) / sizeof(CASABLANCA_UTC_PAUSES[0]);

inline std::string timezone_id_from_option(const std::string &tz_option) {
  size_t idx = tz_option.find(" (");
  return idx == std::string::npos ? tz_option : tz_option.substr(0, idx);
}

inline bool lookup_tz_coords(const std::string &tz_id, float &lat, float &lon) {
  for (int i = 0; i < TZ_COORDS_COUNT; i++) {
    if (tz_id == TZ_COORDS[i].tz) {
      lat = TZ_COORDS[i].lat;
      lon = TZ_COORDS[i].lon;
      return true;
    }
  }
  return false;
}

inline const char* lookup_posix_tz(const std::string &tz_id) {
  for (int i = 0; i < TZ_COORDS_COUNT; i++) {
    if (tz_id == TZ_COORDS[i].tz)
      return TZ_COORDS[i].posix_tz;
  }
  return "UTC0";
}

inline int compare_utc_point(const TzUtcPoint &a, const TzUtcPoint &b) {
  if (a.year != b.year) return a.year < b.year ? -1 : 1;
  if (a.month != b.month) return a.month < b.month ? -1 : 1;
  if (a.day != b.day) return a.day < b.day ? -1 : 1;
  if (a.hour != b.hour) return a.hour < b.hour ? -1 : 1;
  if (a.minute != b.minute) return a.minute < b.minute ? -1 : 1;
  return 0;
}

inline TzUtcPoint utc_point_from_tm(const struct tm &tm) {
  return {tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min};
}

inline bool casablanca_pause_at_utc(const TzUtcPoint &utc_point) {
  for (int i = 0; i < CASABLANCA_UTC_PAUSE_COUNT; i++) {
    const auto &range = CASABLANCA_UTC_PAUSES[i];
    if (compare_utc_point(utc_point, range.start) >= 0 &&
        compare_utc_point(utc_point, range.end) < 0)
      return true;
  }
  return false;
}

inline const char* resolve_posix_tz_at_utc(
    const std::string &tz_id, const TzUtcPoint &utc_point) {
  if (tz_id == "Africa/Casablanca" && casablanca_pause_at_utc(utc_point))
    return "GMT0";
  return lookup_posix_tz(tz_id);
}

inline const char* current_posix_tz(const std::string &tz_id) {
  time_t t = time(nullptr);
  struct tm utc_tm;
  gmtime_r(&t, &utc_tm);
  return resolve_posix_tz_at_utc(tz_id, utc_point_from_tm(utc_tm));
}

inline const char* apply_timezone(const std::string &tz_option) {
  std::string tz_id = timezone_id_from_option(tz_option);
  const char* posix = current_posix_tz(tz_id);
  setenv("TZ", posix, 1);
  tzset();
  return posix;
}

struct TzPosixTransitionRule {
  int month;
  int week;
  int day;
  int seconds;
};

inline bool parse_tz_int(const char *text, size_t &idx, int &value) {
  if (!std::isdigit(static_cast<unsigned char>(text[idx]))) return false;
  value = 0;
  while (std::isdigit(static_cast<unsigned char>(text[idx]))) {
    value = value * 10 + (text[idx] - '0');
    idx++;
  }
  return true;
}

inline void skip_posix_tz_name(const char *text, size_t &idx) {
  if (text[idx] == '<') {
    idx++;
    while (text[idx] && text[idx] != '>') idx++;
    if (text[idx] == '>') idx++;
    return;
  }
  while (std::isalpha(static_cast<unsigned char>(text[idx]))) idx++;
}

inline bool parse_posix_time_seconds(const char *text, size_t &idx, int &seconds) {
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

inline bool parse_posix_m_rule(const char *text, size_t &idx,
                               TzPosixTransitionRule &rule) {
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

inline bool parse_posix_tz_rule(const char *posix,
                                int &std_offset_seconds,
                                bool &has_dst,
                                int &dst_offset_seconds,
                                TzPosixTransitionRule &start_rule,
                                TzPosixTransitionRule &end_rule) {
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

inline int tz_transition_day_of_month(int year, const TzPosixTransitionRule &rule) {
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

inline int64_t tz_transition_utc_epoch(int year,
                                       const TzPosixTransitionRule &rule,
                                       int before_offset_seconds) {
  int day = tz_transition_day_of_month(year, rule);
  return tz_epoch_utc(year, rule.month, day, rule.seconds) + before_offset_seconds;
}

inline bool timezone_offset_minutes_at_utc(const std::string &tz_option,
                                           time_t epoch,
                                           int &offset_minutes) {
  std::string tz_id = timezone_id_from_option(tz_option);
  struct tm utc_tm;
  gmtime_r(&epoch, &utc_tm);
  const char *posix = resolve_posix_tz_at_utc(tz_id, utc_point_from_tm(utc_tm));

  int std_offset_seconds = 0;
  int dst_offset_seconds = 0;
  bool has_dst = false;
  TzPosixTransitionRule start_rule = {};
  TzPosixTransitionRule end_rule = {};
  if (!parse_posix_tz_rule(posix, std_offset_seconds, has_dst,
                           dst_offset_seconds, start_rule, end_rule)) {
    return false;
  }

  int offset_seconds = std_offset_seconds;
  if (has_dst) {
    int year = utc_tm.tm_year + 1900;
    int64_t start = tz_transition_utc_epoch(year, start_rule, std_offset_seconds);
    int64_t end = tz_transition_utc_epoch(year, end_rule, dst_offset_seconds);
    int64_t t = static_cast<int64_t>(epoch);
    bool in_dst = start <= end ? (t >= start && t < end) : (t >= start || t < end);
    if (in_dst) offset_seconds = dst_offset_seconds;
  }

  offset_minutes = -offset_seconds / 60;
  return true;
}

template <typename TimeT>
inline TimeT panel_time_or_fallback(TimeT primary, TimeT fallback) {
  return primary.is_valid() ? primary : fallback;
}

inline std::string trim_ntp_server(const std::string &value) {
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
  return value.substr(start, end - start);
}

inline void apply_ntp_servers(const std::string &server_1,
                              const std::string &server_2,
                              const std::string &server_3) {
#if defined(USE_ESP_IDF)
  if (!esphome::App.is_setup_complete()) {
    ESP_LOGI("sntp", "Application setup not complete; deferring NTP server apply");
    return;
  }

  if (esphome::network::get_ip_addresses().empty()) {
    ESP_LOGI("sntp", "Network not ready; deferring NTP server apply");
    return;
  }

  static std::string active_servers[3];
  active_servers[0] = trim_ntp_server(server_1);
  active_servers[1] = trim_ntp_server(server_2);
  active_servers[2] = trim_ntp_server(server_3);

  if (active_servers[0].empty()) active_servers[0] = "0.pool.ntp.org";
  if (active_servers[1].empty()) active_servers[1] = "1.pool.ntp.org";
  if (active_servers[2].empty()) active_servers[2] = "2.pool.ntp.org";

  static char server_storage[3][101] = {};

  if (esp_sntp_enabled()) {
    esp_sntp_stop();
  }

  esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
  for (int i = 0; i < 3; i++) {
    std::snprintf(server_storage[i], sizeof(server_storage[i]), "%s",
                  active_servers[i].c_str());
    esp_sntp_setservername(i, server_storage[i]);
  }

  esp_sntp_init();
  ESP_LOGI("sntp", "NTP servers applied: %s, %s, %s",
           server_storage[0], server_storage[1], server_storage[2]);
#else
  (void) server_1;
  (void) server_2;
  (void) server_3;
#endif
}

inline float utc_offset_hours_for_date(
    int year, int month, int day, const std::string &tz_option) {
  int64_t local_noon = tz_epoch_utc(year, month, day, 12 * 3600);
  int offset_minutes = 0;
  if (!timezone_offset_minutes_at_utc(
          tz_option, static_cast<time_t>(local_noon), offset_minutes)) {
    return 0.0f;
  }

  time_t utc_noon = static_cast<time_t>(
      local_noon - static_cast<int64_t>(offset_minutes) * 60);
  int refined_offset_minutes = offset_minutes;
  if (timezone_offset_minutes_at_utc(tz_option, utc_noon, refined_offset_minutes)) {
    offset_minutes = refined_offset_minutes;
  }
  return offset_minutes / 60.0f;
}

// ============================================================================
// NOAA sunrise/sunset calculator
// ============================================================================
// Simplified NOAA algorithm. Takes date, lat/lon, and UTC offset in hours.
// Writes sunrise and sunset as local hours and minutes.
// Returns false for polar day/night (no rise or set).

static constexpr float SUN_CALC_DEG_TO_RAD = M_PI / 180.0f;
static constexpr float SUN_CALC_RAD_TO_DEG = 180.0f / M_PI;

inline bool calc_sunrise_sunset(int year, int month, int day,
                                float lat, float lon, float tz_offset,
                                int &rise_h, int &rise_m,
                                int &set_h, int &set_m) {
  int n1 = 275 * month / 9;
  int n2 = (month + 9) / 12;
  int n3 = 1 + ((year - 4 * (year / 4) + 2) / 3);
  int day_of_year = n1 - (n2 * n3) + day - 30;

  float lng_hour = lon / 15.0f;

  auto calc_time = [&](bool is_sunrise, int &out_h, int &out_m) -> bool {
    float t = is_sunrise
      ? day_of_year + ((6.0f - lng_hour) / 24.0f)
      : day_of_year + ((18.0f - lng_hour) / 24.0f);

    float mean_anomaly = (0.9856f * t) - 3.289f;
    float sun_lon = mean_anomaly
      + (1.916f * sinf(mean_anomaly * SUN_CALC_DEG_TO_RAD))
      + (0.020f * sinf(2.0f * mean_anomaly * SUN_CALC_DEG_TO_RAD))
      + 282.634f;
    while (sun_lon < 0) sun_lon += 360.0f;
    while (sun_lon >= 360.0f) sun_lon -= 360.0f;

    float ra = SUN_CALC_RAD_TO_DEG * atanf(0.91764f * tanf(sun_lon * SUN_CALC_DEG_TO_RAD));
    while (ra < 0) ra += 360.0f;
    while (ra >= 360.0f) ra -= 360.0f;

    int l_quad = ((int)(sun_lon / 90.0f)) * 90;
    int ra_quad = ((int)(ra / 90.0f)) * 90;
    ra += (l_quad - ra_quad);
    ra /= 15.0f;

    float sin_dec = 0.39782f * sinf(sun_lon * SUN_CALC_DEG_TO_RAD);
    float cos_dec = cosf(asinf(sin_dec));

    float zenith = 90.833f;
    float cos_h = (cosf(zenith * SUN_CALC_DEG_TO_RAD) - (sin_dec * sinf(lat * SUN_CALC_DEG_TO_RAD)))
                  / (cos_dec * cosf(lat * SUN_CALC_DEG_TO_RAD));

    if (cos_h > 1.0f || cos_h < -1.0f) return false;

    float h;
    if (is_sunrise)
      h = 360.0f - SUN_CALC_RAD_TO_DEG * acosf(cos_h);
    else
      h = SUN_CALC_RAD_TO_DEG * acosf(cos_h);
    h /= 15.0f;

    float local_t = h + ra - (0.06571f * t) - 6.622f;
    float ut = local_t - lng_hour;
    while (ut < 0) ut += 24.0f;
    while (ut >= 24.0f) ut -= 24.0f;

    float local_time = ut + tz_offset;
    while (local_time < 0) local_time += 24.0f;
    while (local_time >= 24.0f) local_time -= 24.0f;

    out_h = (int)local_time;
    out_m = (int)((local_time - out_h) * 60.0f);
    return true;
  };

  bool ok_rise = calc_time(true, rise_h, rise_m);
  bool ok_set = calc_time(false, set_h, set_m);

  if (!ok_rise) { rise_h = 6; rise_m = 0; }
  if (!ok_set)  { set_h = 18; set_m = 0; }

  return ok_rise && ok_set;
}

#endif  // ESPCONTROL_SUN_CALC_H
