#pragma once
#include <string>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cctype>

#if defined(USE_ESP_IDF)
#include <esp_sntp.h>
#endif

// ============================================================================
// Timezone coordinate and POSIX TZ lookup table
// ============================================================================
// Representative city lat/lon for sunrise/sunset calculation, plus POSIX TZ
// strings for DST-aware local time via setenv("TZ")/tzset().

struct TzCoord { const char* tz; float lat; float lon; const char* posix_tz; };
struct TzUtcPoint { int year; int month; int day; int hour; int minute; };
struct TzUtcRange { TzUtcPoint start; TzUtcPoint end; };

static const TzCoord TZ_COORDS[] = {
  {"Pacific/Midway",                    28.21f, -177.38f, "SST11"},
  {"Pacific/Pago_Pago",               -14.27f, -170.70f, "SST11"},
  {"Pacific/Honolulu",                  21.31f, -157.86f, "HST10"},
  {"America/Adak",                      51.88f, -176.66f, "HST10HDT,M3.2.0,M11.1.0"},
  {"America/Anchorage",                 61.22f, -149.90f, "AKST9AKDT,M3.2.0,M11.1.0"},
  {"America/Juneau",                    58.30f, -134.42f, "AKST9AKDT,M3.2.0,M11.1.0"},
  {"America/Los_Angeles",               34.05f, -118.24f, "PST8PDT,M3.2.0,M11.1.0"},
  {"America/Vancouver",                 49.28f, -123.12f, "PST8PDT,M3.2.0,M11.1.0"},
  {"America/Tijuana",                   32.51f, -117.04f, "PST8PDT,M3.2.0,M11.1.0"},
  {"America/Denver",                    39.74f, -104.98f, "MST7MDT,M3.2.0,M11.1.0"},
  {"America/Phoenix",                  33.45f, -112.07f, "MST7"},
  {"America/Edmonton",                  53.55f, -113.49f, "MST7MDT,M3.2.0,M11.1.0"},
  {"America/Boise",                     43.62f, -116.21f, "MST7MDT,M3.2.0,M11.1.0"},
  {"America/Chicago",                   41.88f,  -87.63f, "CST6CDT,M3.2.0,M11.1.0"},
  {"America/Mexico_City",               19.43f,  -99.13f, "CST6"},
  {"America/Winnipeg",                  49.90f,  -97.14f, "CST6CDT,M3.2.0,M11.1.0"},
  {"America/Guatemala",                 14.63f,  -90.51f, "CST6"},
  {"America/Costa_Rica",                 9.93f,  -84.08f, "CST6"},
  {"America/New_York",                  40.71f,  -74.01f, "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Toronto",                   43.65f,  -79.38f, "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Detroit",                   42.33f,  -83.05f, "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Havana",                    23.11f,  -82.37f, "CST5CDT,M3.2.0/0,M11.1.0/1"},
  {"America/Bogota",                     4.71f,  -74.07f, "<-05>5"},
  {"America/Lima",                     -12.05f,  -77.04f, "<-05>5"},
  {"America/Jamaica",                   18.11f,  -77.30f, "EST5"},
  {"America/Panama",                     8.98f,  -79.52f, "EST5"},
  {"America/Halifax",                   44.65f,  -63.57f, "AST4ADT,M3.2.0,M11.1.0"},
  {"America/Caracas",                   10.49f,  -66.88f, "<-04>4"},
  {"America/Santiago",                 -33.45f,  -70.67f, "<-04>4<-03>,M9.1.6/24,M4.1.6/24"},
  {"America/La_Paz",                   -16.50f,  -68.15f, "<-04>4"},
  {"America/Manaus",                    -3.12f,  -60.02f, "<-04>4"},
  {"America/Barbados",                  13.10f,  -59.61f, "AST4"},
  {"America/Puerto_Rico",              18.47f,  -66.11f, "AST4"},
  {"America/Santo_Domingo",            18.49f,  -69.93f, "AST4"},
  {"America/St_Johns",                  47.56f,  -52.71f, "NST3:30NDT,M3.2.0,M11.1.0"},
  {"America/Sao_Paulo",               -23.55f,  -46.63f, "<-03>3"},
  {"America/Argentina/Buenos_Aires",   -34.60f,  -58.38f, "<-03>3"},
  {"America/Montevideo",              -34.88f,  -56.16f, "<-03>3"},
  {"America/Paramaribo",                5.85f,  -55.17f, "<-03>3"},
  {"Atlantic/South_Georgia",          -54.28f,  -36.51f, "<-02>2"},
  {"Atlantic/Azores",                  38.72f,  -27.22f, "<-01>1<+00>,M3.5.0/0,M10.5.0/1"},
  {"Atlantic/Cape_Verde",              14.93f,  -23.51f, "<-01>1"},
  {"UTC",                               51.51f,   -0.13f, "UTC0"},
  {"Europe/London",                     51.51f,   -0.13f, "GMT0BST,M3.5.0/1,M10.5.0"},
  {"Europe/Dublin",                     53.35f,   -6.26f, "GMT0IST,M3.5.0/1,M10.5.0"},
  {"Europe/Lisbon",                     38.72f,   -9.14f, "WET0WEST,M3.5.0/1,M10.5.0"},
  {"Africa/Casablanca",                 33.57f,   -7.59f, "<+01>-1"},
  {"Africa/Accra",                       5.56f,   -0.19f, "GMT0"},
  {"Atlantic/Reykjavik",               64.15f,  -21.94f, "GMT0"},
  {"Europe/Paris",                      48.86f,    2.35f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Berlin",                     52.52f,   13.40f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Rome",                       41.90f,   12.50f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Madrid",                     40.42f,   -3.70f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Amsterdam",                  52.37f,    4.90f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Brussels",                   50.85f,    4.35f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Vienna",                     48.21f,   16.37f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Zurich",                     47.38f,    8.54f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Stockholm",                  59.33f,   18.07f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Oslo",                       59.91f,   10.75f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Copenhagen",                 55.68f,   12.57f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Warsaw",                     52.23f,   21.01f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Prague",                     50.08f,   14.44f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Budapest",                   47.50f,   19.04f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Europe/Belgrade",                   44.79f,   20.47f, "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"Africa/Lagos",                       6.45f,    3.39f, "WAT-1"},
  {"Africa/Tunis",                      36.81f,   10.17f, "CET-1"},
  {"Africa/Cairo",                      30.04f,   31.24f, "EET-2EEST,M4.5.5/0,M10.5.4/24"},
  {"Europe/Athens",                     37.98f,   23.73f, "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Europe/Bucharest",                  44.43f,   26.10f, "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Europe/Helsinki",                   60.17f,   24.94f, "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Europe/Kyiv",                       50.45f,   30.52f, "EET-2EEST,M3.5.0/3,M10.5.0/4"},
  {"Europe/Istanbul",                   41.01f,   28.98f, "<+03>-3"},
  {"Africa/Johannesburg",             -26.20f,   28.05f, "SAST-2"},
  {"Africa/Nairobi",                    -1.29f,   36.82f, "EAT-3"},
  {"Asia/Jerusalem",                    31.77f,   35.22f, "IST-2IDT,M3.4.4/26,M10.5.0"},
  {"Asia/Amman",                        31.95f,   35.93f, "<+03>-3"},
  {"Asia/Beirut",                       33.89f,   35.50f, "EET-2EEST,M3.5.0/0,M10.5.0/0"},
  {"Europe/Moscow",                     55.76f,   37.62f, "MSK-3"},
  {"Asia/Baghdad",                      33.31f,   44.37f, "<+03>-3"},
  {"Asia/Riyadh",                       24.69f,   46.72f, "<+03>-3"},
  {"Asia/Kuwait",                       29.38f,   47.98f, "<+03>-3"},
  {"Asia/Qatar",                        25.29f,   51.53f, "<+03>-3"},
  {"Africa/Addis_Ababa",                 9.01f,   38.75f, "EAT-3"},
  {"Asia/Tehran",                       35.69f,   51.39f, "<+0330>-3:30"},
  {"Asia/Dubai",                        25.20f,   55.27f, "<+04>-4"},
  {"Asia/Muscat",                       23.59f,   58.54f, "<+04>-4"},
  {"Asia/Baku",                         40.41f,   49.87f, "<+04>-4"},
  {"Asia/Tbilisi",                      41.72f,   44.79f, "<+04>-4"},
  {"Indian/Mauritius",                 -20.16f,   57.50f, "<+04>-4"},
  {"Asia/Kabul",                        34.53f,   69.17f, "<+0430>-4:30"},
  {"Asia/Karachi",                      24.86f,   67.01f, "PKT-5"},
  {"Asia/Tashkent",                     41.30f,   69.28f, "<+05>-5"},
  {"Asia/Yekaterinburg",                56.84f,   60.60f, "<+05>-5"},
  {"Asia/Kolkata",                      28.61f,   77.21f, "IST-5:30"},
  {"Asia/Colombo",                       6.93f,   79.84f, "<+0530>-5:30"},
  {"Asia/Kathmandu",                    27.72f,   85.32f, "<+0545>-5:45"},
  {"Asia/Dhaka",                        23.81f,   90.41f, "<+06>-6"},
  {"Asia/Almaty",                       43.24f,   76.95f, "<+05>-5"},
  {"Asia/Rangoon",                      16.87f,   96.20f, "<+0630>-6:30"},
  {"Asia/Bangkok",                      13.76f,  100.50f, "<+07>-7"},
  {"Asia/Jakarta",                      -6.21f,  106.85f, "WIB-7"},
  {"Asia/Ho_Chi_Minh",                  10.82f,  106.63f, "<+07>-7"},
  {"Asia/Singapore",                     1.35f,  103.82f, "<+08>-8"},
  {"Asia/Kuala_Lumpur",                  3.14f,  101.69f, "<+08>-8"},
  {"Asia/Shanghai",                     31.23f,  121.47f, "CST-8"},
  {"Asia/Hong_Kong",                    22.32f,  114.17f, "HKT-8"},
  {"Asia/Taipei",                       25.03f,  121.57f, "CST-8"},
  {"Asia/Manila",                       14.60f,  120.98f, "PST-8"},
  {"Australia/Perth",                  -31.95f,  115.86f, "AWST-8"},
  {"Asia/Tokyo",                        35.68f,  139.69f, "JST-9"},
  {"Asia/Seoul",                        37.57f,  126.98f, "KST-9"},
  {"Asia/Pyongyang",                    39.02f,  125.75f, "KST-9"},
  {"Australia/Adelaide",               -34.93f,  138.60f, "ACST-9:30ACDT,M10.1.0,M4.1.0/3"},
  {"Australia/Darwin",                 -12.46f,  130.84f, "ACST-9:30"},
  {"Australia/Sydney",                 -33.87f,  151.21f, "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Australia/Melbourne",              -37.81f,  144.96f, "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Australia/Brisbane",               -27.47f,  153.03f, "AEST-10"},
  {"Australia/Hobart",                 -42.88f,  147.33f, "AEST-10AEDT,M10.1.0,M4.1.0/3"},
  {"Pacific/Guam",                      13.44f,  144.79f, "ChST-10"},
  {"Pacific/Port_Moresby",             -6.31f,  147.17f, "<+10>-10"},
  {"Asia/Vladivostok",                  43.12f,  131.91f, "<+10>-10"},
  {"Pacific/Noumea",                   -22.28f,  166.46f, "<+11>-11"},
  {"Pacific/Norfolk",                  -29.05f,  167.96f, "<+11>-11<+12>,M10.1.0,M4.1.0/3"},
  {"Asia/Magadan",                      59.56f,  150.80f, "<+11>-11"},
  {"Pacific/Auckland",                 -36.85f,  174.76f, "NZST-12NZDT,M9.5.0,M4.1.0/3"},
  {"Pacific/Fiji",                     -18.14f,  178.44f, "<+12>-12"},
  {"Pacific/Chatham",                  -43.88f, -176.46f, "<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45"},
  {"Pacific/Tongatapu",               -21.21f, -175.15f, "<+13>-13"},
  {"Pacific/Apia",                     -13.83f, -171.76f, "<+13>-13"},
  {"Pacific/Kiritimati",                 1.87f, -157.47f, "<+14>-14"},
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
  static std::string active_servers[3];
  active_servers[0] = trim_ntp_server(server_1);
  active_servers[1] = trim_ntp_server(server_2);
  active_servers[2] = trim_ntp_server(server_3);

  if (active_servers[0].empty()) active_servers[0] = "0.pool.ntp.org";
  if (active_servers[1].empty()) active_servers[1] = "1.pool.ntp.org";
  if (active_servers[2].empty()) active_servers[2] = "2.pool.ntp.org";

  for (int i = 0; i < 3; i++) {
    esp_sntp_setservername(i, active_servers[i].c_str());
  }

  if (esp_sntp_enabled()) {
    esp_sntp_restart();
  }
#else
  (void) server_1;
  (void) server_2;
  (void) server_3;
#endif
}

inline float utc_offset_hours_at(time_t t) {
  struct tm utc_tm, local_tm;
  gmtime_r(&t, &utc_tm);
  localtime_r(&t, &local_tm);
  int diff_min = (local_tm.tm_hour - utc_tm.tm_hour) * 60
               + (local_tm.tm_min - utc_tm.tm_min);
  int day_diff = local_tm.tm_mday - utc_tm.tm_mday;
  if (day_diff > 1) day_diff = -1;
  else if (day_diff < -1) day_diff = 1;
  diff_min += day_diff * 1440;
  return diff_min / 60.0f;
}

inline float current_utc_offset_hours() {
  return utc_offset_hours_at(time(nullptr));
}

inline float utc_offset_hours_for_date(
    int year, int month, int day, const std::string &tz_option) {
  std::string tz_id = timezone_id_from_option(tz_option);
  setenv("TZ", lookup_posix_tz(tz_id), 1);
  tzset();

  struct tm local_noon = {};
  local_noon.tm_year = year - 1900;
  local_noon.tm_mon = month - 1;
  local_noon.tm_mday = day;
  local_noon.tm_hour = 12;
  local_noon.tm_isdst = -1;
  time_t noon_epoch = mktime(&local_noon);
  float offset = utc_offset_hours_at(noon_epoch);

  if (tz_id == "Africa/Casablanca") {
    struct tm utc_tm;
    gmtime_r(&noon_epoch, &utc_tm);
    offset = casablanca_pause_at_utc(utc_point_from_tm(utc_tm)) ? 0.0f : 1.0f;
  }

  apply_timezone(tz_option);
  return offset;
}

// ============================================================================
// NOAA sunrise/sunset calculator
// ============================================================================
// Simplified NOAA algorithm. Takes date, lat/lon, and UTC offset in hours.
// Writes sunrise and sunset as local hours and minutes.
// Returns false for polar day/night (no rise or set).

static constexpr float DEG_TO_RAD = M_PI / 180.0f;
static constexpr float RAD_TO_DEG = 180.0f / M_PI;

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
      + (1.916f * sinf(mean_anomaly * DEG_TO_RAD))
      + (0.020f * sinf(2.0f * mean_anomaly * DEG_TO_RAD))
      + 282.634f;
    while (sun_lon < 0) sun_lon += 360.0f;
    while (sun_lon >= 360.0f) sun_lon -= 360.0f;

    float ra = RAD_TO_DEG * atanf(0.91764f * tanf(sun_lon * DEG_TO_RAD));
    while (ra < 0) ra += 360.0f;
    while (ra >= 360.0f) ra -= 360.0f;

    int l_quad = ((int)(sun_lon / 90.0f)) * 90;
    int ra_quad = ((int)(ra / 90.0f)) * 90;
    ra += (l_quad - ra_quad);
    ra /= 15.0f;

    float sin_dec = 0.39782f * sinf(sun_lon * DEG_TO_RAD);
    float cos_dec = cosf(asinf(sin_dec));

    float zenith = 90.833f;
    float cos_h = (cosf(zenith * DEG_TO_RAD) - (sin_dec * sinf(lat * DEG_TO_RAD)))
                  / (cos_dec * cosf(lat * DEG_TO_RAD));

    if (cos_h > 1.0f || cos_h < -1.0f) return false;

    float h;
    if (is_sunrise)
      h = 360.0f - RAD_TO_DEG * acosf(cos_h);
    else
      h = RAD_TO_DEG * acosf(cos_h);
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
