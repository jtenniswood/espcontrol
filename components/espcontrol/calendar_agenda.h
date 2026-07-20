#ifndef ESPCONTROL_CALENDAR_AGENDA_H
#define ESPCONTROL_CALENDAR_AGENDA_H

#pragma once

// Pure logic for the calendar agenda (a simple Calendar-Card-Pro-style list of
// upcoming events, grouped by day). Everything here is host-testable and free
// of ESPHome, LVGL, and ArduinoJson: it parses the ISO start strings that
// Home Assistant's calendar.get_events returns, merges events from several
// calendars into one time-ordered list, and formats day headings and times.
//
// Home Assistant facts this encodes (verified against a live calendar.get_events):
//   - A timed event's start is ISO 8601 with an offset: "2026-07-21T09:00:00-04:00".
//   - An all-day event's start is a bare date: "2026-07-17".
//   - Events per calendar come back sorted, but a merge of several calendars
//     must be re-sorted; the same event can appear once per calendar.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace espcontrol {

inline constexpr std::size_t kAgendaMaxEvents = 8;

// Days since 1970-01-01 for a civil date (proleptic Gregorian). Howard
// Hinnant's algorithm — exact integer math, valid for any reasonable year.
inline int32_t agenda_days_from_civil(int y, int m, int d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + static_cast<int>(doe) - 719468;
}

// 0=Sunday .. 6=Saturday. 1970-01-01 (day 0) was a Thursday.
inline int agenda_weekday(int32_t day_number) {
  return static_cast<int>(((day_number % 7) + 4 + 7) % 7);
}

// Inverse of agenda_days_from_civil: recover the civil date from a day number.
inline void agenda_civil_from_days(int32_t day_number, int *y, int *m, int *d) {
  int32_t z = day_number + 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned doe = static_cast<unsigned>(z - era * 146097);
  const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int year = static_cast<int>(yoe) + era * 400;
  const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned mp = (5 * doy + 2) / 153;
  const unsigned day = doy - (153 * mp + 2) / 5 + 1;
  const unsigned month = mp + (mp < 10 ? 3 : -9);
  if (y) *y = month <= 2 ? year + 1 : year;
  if (m) *m = static_cast<int>(month);
  if (d) *d = static_cast<int>(day);
}

// Split a comma-separated list of calendar entity ids, trimming surrounding
// whitespace and dropping empty items. Used to configure the agenda from a
// single text entity holding several calendars.
inline std::vector<std::string> agenda_split_entities(const std::string &csv) {
  std::vector<std::string> out;
  std::size_t start = 0;
  while (start <= csv.size()) {
    std::size_t comma = csv.find(',', start);
    std::size_t end = comma == std::string::npos ? csv.size() : comma;
    std::size_t a = start;
    std::size_t b = end;
    while (a < b && (csv[a] == ' ' || csv[a] == '\t')) a++;
    while (b > a && (csv[b - 1] == ' ' || csv[b - 1] == '\t')) b--;
    if (b > a) out.push_back(csv.substr(a, b - a));
    if (comma == std::string::npos) break;
    start = comma + 1;
  }
  return out;
}

// One entry of the calendar list: "calendar.family" optionally suffixed with a
// display color as "calendar.family:#66BB6A". agenda_split_entities keeps the
// raw items; these helpers separate the entity id from its color.
inline std::string agenda_entity_id(const std::string &item) {
  std::size_t hash = item.find(":#");
  std::string id = hash == std::string::npos ? item : item.substr(0, hash);
  while (!id.empty() && (id.back() == ' ' || id.back() == '\t')) id.pop_back();
  return id;
}

// Returns the "#RRGGBB" color as 0xRRGGBB, or 0 when absent or malformed.
inline uint32_t agenda_entity_color(const std::string &item) {
  std::size_t hash = item.find(":#");
  if (hash == std::string::npos) return 0;
  const std::string hex = item.substr(hash + 2);
  if (hex.size() != 6) return 0;
  uint32_t value = 0;
  for (char ch : hex) {
    value <<= 4;
    if (ch >= '0' && ch <= '9') value |= static_cast<uint32_t>(ch - '0');
    else if (ch >= 'a' && ch <= 'f') value |= static_cast<uint32_t>(ch - 'a' + 10);
    else if (ch >= 'A' && ch <= 'F') value |= static_cast<uint32_t>(ch - 'A' + 10);
    else return 0;
  }
  return value;
}

inline std::vector<std::string> agenda_entity_ids(const std::string &csv) {
  std::vector<std::string> out;
  for (const std::string &item : agenda_split_entities(csv))
    out.push_back(agenda_entity_id(item));
  return out;
}

struct AgendaEventTime {
  int64_t sort_key{0};   // seconds for time-ordering the merged list
  int32_t day_number{0}; // days_from_civil of the local start date, for grouping
  int year{0};
  int month{0};
  int day{0};
  int hour{0};
  int minute{0};
  bool all_day{false};
};

// Parse a calendar event's start string. Accepts a bare date (all-day) or an
// ISO 8601 datetime with an optional timezone offset or trailing 'Z'. Returns
// false when the leading date cannot be read.
inline bool agenda_parse_start(const char *iso, AgendaEventTime *out) {
  if (iso == nullptr || out == nullptr) return false;
  int y = 0, mo = 0, d = 0;
  if (std::sscanf(iso, "%4d-%2d-%2d", &y, &mo, &d) != 3) return false;
  if (mo < 1 || mo > 12 || d < 1 || d > 31) return false;

  AgendaEventTime t;
  t.year = y;
  t.month = mo;
  t.day = d;
  t.day_number = agenda_days_from_civil(y, mo, d);

  // A 'T' (or space) separator introduces a time; its absence marks an all-day
  // event, whose start is a bare date.
  const char *sep = std::strchr(iso, 'T');
  if (sep == nullptr) sep = std::strchr(iso, ' ');
  if (sep == nullptr) {
    t.all_day = true;
    t.sort_key = static_cast<int64_t>(t.day_number) * 86400;
    *out = t;
    return true;
  }

  int hh = 0, mm = 0, ss = 0;
  if (std::sscanf(sep + 1, "%2d:%2d:%2d", &hh, &mm, &ss) < 2) {
    // Time present but unreadable; treat as all-day rather than dropping it.
    t.all_day = true;
    t.sort_key = static_cast<int64_t>(t.day_number) * 86400;
    *out = t;
    return true;
  }
  t.hour = hh;
  t.minute = mm;

  // Offset: find the last '+'/'-' after the time, or a trailing 'Z' (UTC).
  int offset_seconds = 0;
  const char *time_part = sep + 1;
  const char *plus = std::strrchr(time_part, '+');
  const char *minus = std::strrchr(time_part, '-');
  const char *sign = plus ? plus : minus;
  if (sign != nullptr) {
    int oh = 0, om = 0;
    if (std::sscanf(sign + 1, "%2d:%2d", &oh, &om) >= 1) {
      offset_seconds = (oh * 3600 + om * 60) * (*sign == '-' ? -1 : 1);
    }
  }
  // UTC epoch so events from calendars in different offsets sort correctly.
  t.sort_key = static_cast<int64_t>(t.day_number) * 86400 +
               hh * 3600 + mm * 60 + ss - offset_seconds;
  *out = t;
  return true;
}

// Format an event's time: "9:00 AM" / "2:30 PM" (12h) or "09:00" / "14:30"
// (24h). All-day events have no time; the buffer is left empty.
inline void agenda_format_time(char *buf, std::size_t size, const AgendaEventTime &t,
                               bool use_12h) {
  if (buf == nullptr || size == 0) return;
  if (t.all_day) {
    buf[0] = '\0';
    return;
  }
  if (use_12h) {
    int display_hour = t.hour % 12;
    if (display_hour == 0) display_hour = 12;
    std::snprintf(buf, size, "%d:%02d %s", display_hour, t.minute,
                  t.hour < 12 ? "AM" : "PM");
  } else {
    std::snprintf(buf, size, "%02d:%02d", t.hour, t.minute);
  }
}

// Format a day heading relative to today: "Today", "Tomorrow", or "Wed 23 Jul".
inline void agenda_format_day_heading(char *buf, std::size_t size, int32_t day_number,
                                      int32_t today_number) {
  if (buf == nullptr || size == 0) return;
  const int32_t delta = day_number - today_number;
  if (delta == 0) {
    std::snprintf(buf, size, "Today");
    return;
  }
  if (delta == 1) {
    std::snprintf(buf, size, "Tomorrow");
    return;
  }
  static const char *const kWeekdays[] = {"Sun", "Mon", "Tue", "Wed",
                                          "Thu", "Fri", "Sat"};
  static const char *const kMonths[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  int year = 0, month = 1, day = 1;
  agenda_civil_from_days(day_number, &year, &month, &day);
  (void) year;
  std::snprintf(buf, size, "%s %d %s", kWeekdays[agenda_weekday(day_number)],
                day, kMonths[(month - 1) % 12]);
}

struct AgendaEntry {
  AgendaEventTime when;
  AgendaEventTime end;
  bool has_end{false};
  std::string summary;
  std::string location;
  uint8_t source{0};
};

// Format an event's time range the way Calendar Card Pro does:
//   timed:            "9:00 AM - 10:00 AM"
//   all-day:          "All day"
//   multi-day all-day "All day, ends Tomorrow" / "All day, ends Wed 23 Jul"
// Home Assistant's all-day end date is exclusive, so an event through the
// 19th reports an end of the 20th; the displayed end is end-1.
inline void agenda_format_range(char *buf, std::size_t size,
                                const AgendaEntry &entry, bool use_12h,
                                int32_t today_number) {
  if (buf == nullptr || size == 0) return;
  if (entry.when.all_day) {
    if (entry.has_end) {
      const int32_t display_end = entry.end.day_number - 1;
      if (display_end > entry.when.day_number) {
        char end_text[24];
        agenda_format_day_heading(end_text, sizeof(end_text), display_end,
                                  today_number);
        // Mid-sentence here, so "Tomorrow" becomes "tomorrow"; dated headings
        // like "Wed 23 Jul" keep their capital.
        if (std::strcmp(end_text, "Today") == 0 ||
            std::strcmp(end_text, "Tomorrow") == 0) {
          end_text[0] = static_cast<char>(end_text[0] - 'A' + 'a');
        }
        std::snprintf(buf, size, "%s, %s %s", "All day", "ends", end_text);
        return;
      }
    }
    std::snprintf(buf, size, "%s", "All day");
    return;
  }
  char start_text[16];
  agenda_format_time(start_text, sizeof(start_text), entry.when, use_12h);
  if (entry.has_end && !entry.end.all_day) {
    char end_text[16];
    agenda_format_time(end_text, sizeof(end_text), entry.end, use_12h);
    std::snprintf(buf, size, "%s - %s", start_text, end_text);
  } else {
    std::snprintf(buf, size, "%s", start_text);
  }
}

// Right-hand relative marker: "in N days" for events two or more days out;
// empty for today and tomorrow (the date column already says so).
inline void agenda_format_relative(char *buf, std::size_t size,
                                   int32_t day_number, int32_t today_number) {
  if (buf == nullptr || size == 0) return;
  const int32_t delta = day_number - today_number;
  if (delta >= 2) {
    std::snprintf(buf, size, "in %d days", static_cast<int>(delta));
  } else {
    buf[0] = '\0';
  }
}


// Accumulates events across calendars, then time-orders, de-duplicates, and
// caps them for display. Grouping by day is derived from adjacent entries'
// day_number once sorted.
class AgendaList {
 public:
  void clear() { entries_.clear(); }
  bool empty() const { return entries_.empty(); }
  std::size_t size() const { return entries_.size(); }
  const std::vector<AgendaEntry> &entries() const { return entries_; }

  // Add an event from its raw start string. Unparseable or empty-summary
  // events are skipped. Duplicates (same start second and summary, e.g. an
  // event shared across two calendars) are dropped.
  void add(const char *start, const std::string &summary) {
    this->add(start, nullptr, summary, std::string(), 0);
  }

  void add(const char *start, const char *end, const std::string &summary,
           const std::string &location, uint8_t source) {
    if (summary.empty()) return;
    AgendaEventTime when;
    if (!agenda_parse_start(start, &when)) return;
    for (const AgendaEntry &existing : entries_) {
      if (existing.when.sort_key == when.sort_key && existing.summary == summary) {
        return;
      }
    }
    AgendaEntry entry;
    entry.when = when;
    entry.summary = summary;
    entry.location = location;
    entry.source = source;
    if (end != nullptr) {
      entry.has_end = agenda_parse_start(end, &entry.end);
    }
    entries_.push_back(std::move(entry));
  }

  // Sort by time, drop anything already ended before now_epoch when provided
  // (0 disables the filter), and cap to max_events.
  void finalize(std::size_t max_events, int64_t now_epoch = 0) {
    // Simple insertion sort keeps this allocation-free and stable; the lists
    // are short (a handful of events).
    for (std::size_t i = 1; i < entries_.size(); i++) {
      AgendaEntry key = entries_[i];
      std::size_t j = i;
      while (j > 0 && entries_[j - 1].when.sort_key > key.when.sort_key) {
        entries_[j] = entries_[j - 1];
        j--;
      }
      entries_[j] = key;
    }
    if (now_epoch != 0) {
      std::size_t keep = 0;
      for (std::size_t i = 0; i < entries_.size(); i++) {
        // Keep timed events at/after now and all-day events for today onward.
        if (entries_[i].when.sort_key >= now_epoch ||
            entries_[i].when.all_day) {
          entries_[keep++] = entries_[i];
        }
      }
      entries_.resize(keep);
    }
    if (max_events > 0 && entries_.size() > max_events) {
      entries_.resize(max_events);
    }
  }

  // True when this entry begins a new day group (first entry, or a different
  // day_number than the previous entry).
  bool starts_new_day(std::size_t index) const {
    if (index >= entries_.size()) return false;
    if (index == 0) return true;
    return entries_[index].when.day_number != entries_[index - 1].when.day_number;
  }

 private:
  std::vector<AgendaEntry> entries_;
};

// Render the agenda as a multi-line string: a day heading line ("Today",
// "Tomorrow", "Wed 22 Jul") followed by one indented line per event, the time
// (when present) and the summary. Grouped days are separated by a blank line.
// This is what both the grid card and the screensaver overlay display in a
// single wrapping label, so day grouping needs no per-row widgets.
inline std::string agenda_build_text(const AgendaList &list, int32_t today_number,
                                     bool use_12h) {
  std::string text;
  const std::vector<AgendaEntry> &entries = list.entries();
  for (std::size_t i = 0; i < entries.size(); i++) {
    if (list.starts_new_day(i)) {
      if (!text.empty()) text += "\n";
      char heading[24];
      agenda_format_day_heading(heading, sizeof(heading), entries[i].when.day_number,
                                today_number);
      text += heading;
      text += "\n";
    }
    char time_buf[16];
    agenda_format_time(time_buf, sizeof(time_buf), entries[i].when, use_12h);
    text += "  ";
    if (time_buf[0] != '\0') {
      text += time_buf;
      text += "  ";
    }
    text += entries[i].summary;
    if (i + 1 < entries.size()) text += "\n";
  }
  return text;
}

// How many of `entries` fall on a given day. The screensaver overlay draws a
// fixed number of rows and cannot scroll, so it uses this to say how many of
// the day's events it is leaving out instead of dropping them silently.
inline std::size_t agenda_events_on_day(const std::vector<AgendaEntry> &entries,
                                        int32_t day_number) {
  std::size_t count = 0;
  for (const AgendaEntry &entry : entries) {
    if (entry.when.day_number == day_number) count++;
  }
  return count;
}

}  // namespace espcontrol

#endif  // ESPCONTROL_CALENDAR_AGENDA_H
