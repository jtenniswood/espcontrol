#include <cstdlib>
#include <cstring>
#include <string>

#include "calendar_agenda.h"

using namespace espcontrol;

#define CHECK(condition) do { if (!(condition)) return EXIT_FAILURE; } while (false)

int main() {
  // Day numbering matches known reference points.
  CHECK(agenda_days_from_civil(1970, 1, 1) == 0);
  CHECK(agenda_weekday(0) == 4);  // 1970-01-01 was a Thursday
  CHECK(agenda_days_from_civil(2026, 7, 17) == agenda_days_from_civil(2026, 7, 16) + 1);
  CHECK(agenda_days_from_civil(2000, 3, 1) == agenda_days_from_civil(2000, 2, 29) + 1);  // leap day

  // A bare date parses as an all-day event with no time and midnight sort key.
  AgendaEventTime all_day;
  CHECK(agenda_parse_start("2026-07-17", &all_day));
  CHECK(all_day.all_day);
  CHECK(all_day.year == 2026 && all_day.month == 7 && all_day.day == 17);
  CHECK(all_day.sort_key == static_cast<int64_t>(all_day.day_number) * 86400);

  // A timed event parses hour/minute and applies the offset to the sort key.
  AgendaEventTime timed;
  CHECK(agenda_parse_start("2026-07-21T09:00:00-04:00", &timed));
  CHECK(!timed.all_day);
  CHECK(timed.hour == 9 && timed.minute == 0);
  // 09:00 at -04:00 is 13:00 UTC.
  CHECK(timed.sort_key ==
        static_cast<int64_t>(timed.day_number) * 86400 + 13 * 3600);

  // A 'Z' (UTC) suffix and a positive offset both parse.
  AgendaEventTime utc, plus;
  CHECK(agenda_parse_start("2026-07-21T09:00:00Z", &utc));
  CHECK(utc.sort_key == static_cast<int64_t>(utc.day_number) * 86400 + 9 * 3600);
  CHECK(agenda_parse_start("2026-07-21T09:00:00+02:00", &plus));
  CHECK(plus.sort_key == static_cast<int64_t>(plus.day_number) * 86400 + 7 * 3600);

  // The same wall-clock time in different offsets orders by true instant.
  CHECK(plus.sort_key < timed.sort_key);  // 07:00 UTC before 13:00 UTC

  // Garbage is rejected; a "space" separator (HA REST style) is accepted.
  AgendaEventTime junk, spaced;
  CHECK(!agenda_parse_start("not-a-date", &junk));
  CHECK(!agenda_parse_start("", &junk));
  CHECK(!agenda_parse_start(nullptr, &junk));
  CHECK(agenda_parse_start("2026-07-21 14:30:00", &spaced));
  CHECK(!spaced.all_day && spaced.hour == 14 && spaced.minute == 30);

  // Time formatting, 12h and 24h.
  char buf[16];
  agenda_format_time(buf, sizeof(buf), timed, true);
  CHECK(std::strcmp(buf, "9:00 AM") == 0);
  agenda_format_time(buf, sizeof(buf), timed, false);
  CHECK(std::strcmp(buf, "09:00") == 0);
  AgendaEventTime afternoon;
  CHECK(agenda_parse_start("2026-07-21T14:30:00-04:00", &afternoon));
  agenda_format_time(buf, sizeof(buf), afternoon, true);
  CHECK(std::strcmp(buf, "2:30 PM") == 0);
  AgendaEventTime midnight;
  CHECK(agenda_parse_start("2026-07-21T00:15:00-04:00", &midnight));
  agenda_format_time(buf, sizeof(buf), midnight, true);
  CHECK(std::strcmp(buf, "12:15 AM") == 0);
  // All-day events format to an empty time.
  agenda_format_time(buf, sizeof(buf), all_day, true);
  CHECK(buf[0] == '\0');

  // Day headings: Today / Tomorrow / explicit "Wkd D Mon".
  const int32_t today = agenda_days_from_civil(2026, 7, 17);  // a Friday
  CHECK(agenda_weekday(today) == 5);
  agenda_format_day_heading(buf, sizeof(buf), today, today);
  CHECK(std::strcmp(buf, "Today") == 0);
  agenda_format_day_heading(buf, sizeof(buf), today + 1, today);
  CHECK(std::strcmp(buf, "Tomorrow") == 0);
  agenda_format_day_heading(buf, sizeof(buf), agenda_days_from_civil(2026, 7, 22), today);
  CHECK(std::strcmp(buf, "Wed 22 Jul") == 0);
  agenda_format_day_heading(buf, sizeof(buf), agenda_days_from_civil(2026, 12, 25), today);
  CHECK(std::strcmp(buf, "Fri 25 Dec") == 0);

  // Merge, sort, dedup, cap, and day-group boundaries.
  AgendaList list;
  CHECK(list.empty());
  // Added out of order and across "two calendars" (the doctor appt duplicated).
  list.add("2026-07-21T09:00:00-04:00", "Rachel @ doctor");
  list.add("2026-07-17", "Review finances together");
  list.add("2026-07-17T14:30:00-04:00", "School pickup");
  list.add("2026-07-21T09:00:00-04:00", "Rachel @ doctor");  // duplicate calendar
  list.add("2026-07-18", "Bracebridge Art Show");
  list.add("bad-start", "dropped");                          // unparseable
  list.add("2026-07-19T10:00:00-04:00", "");                 // empty summary
  CHECK(list.size() == 4);  // duplicate + two invalids excluded

  list.finalize(kAgendaMaxEvents);
  const auto &e = list.entries();
  CHECK(e.size() == 4);
  // Sorted by instant: 07-17 all-day, 07-17 14:30, 07-18 all-day, 07-21 09:00.
  CHECK(e[0].summary == "Review finances together");
  CHECK(e[1].summary == "School pickup");
  CHECK(e[2].summary == "Bracebridge Art Show");
  CHECK(e[3].summary == "Rachel @ doctor");
  // Day-group boundaries: 0 (new), 1 same day, 2 new day, 3 new day.
  CHECK(list.starts_new_day(0));
  CHECK(!list.starts_new_day(1));
  CHECK(list.starts_new_day(2));
  CHECK(list.starts_new_day(3));

  // The cap keeps only the earliest N.
  AgendaList capped;
  for (int i = 0; i < 12; i++) {
    char start[32];
    std::snprintf(start, sizeof(start), "2026-08-%02dT08:00:00-04:00", i + 1);
    capped.add(start, "event " + std::to_string(i));
  }
  capped.finalize(5);
  CHECK(capped.size() == 5);
  CHECK(capped.entries().front().summary == "event 0");
  CHECK(capped.entries().back().summary == "event 4");

  // now_epoch drops timed events that already ended, keeps all-day for the day.
  AgendaList windowed;
  windowed.add("2026-07-17T08:00:00-04:00", "past meeting");
  windowed.add("2026-07-17T20:00:00-04:00", "evening call");
  windowed.add("2026-07-17", "all day today");
  const int64_t noon_utc =
      static_cast<int64_t>(agenda_days_from_civil(2026, 7, 17)) * 86400 + 16 * 3600;
  windowed.finalize(kAgendaMaxEvents, noon_utc);
  // 08:00-04:00 = 12:00 UTC (before noon_utc 16:00) dropped; evening kept;
  // all-day kept.
  CHECK(windowed.size() == 2);
  bool has_evening = false, has_allday = false, has_past = false;
  for (const auto &entry : windowed.entries()) {
    if (entry.summary == "evening call") has_evening = true;
    if (entry.summary == "all day today") has_allday = true;
    if (entry.summary == "past meeting") has_past = true;
  }
  CHECK(has_evening && has_allday && !has_past);

  // Multi-line rendering groups by day with headings and indented events.
  AgendaList render;
  render.add("2026-07-17", "Review finances together");
  render.add("2026-07-17T14:30:00-04:00", "School pickup");
  render.add("2026-07-18", "Bracebridge Art Show");
  render.finalize(kAgendaMaxEvents);
  const std::string text = agenda_build_text(render, today, true);
  // Day groups are separated by a blank line.
  const std::string expected =
      "Today\n"
      "  Review finances together\n"
      "  2:30 PM  School pickup\n"
      "\n"
      "Tomorrow\n"
      "  Bracebridge Art Show";
  CHECK(text == expected);

  // An empty agenda renders to an empty string.
  AgendaList none;
  none.finalize(kAgendaMaxEvents);
  CHECK(agenda_build_text(none, today, true).empty());

  // Civil-from-days round-trips against days-from-civil.
  for (int32_t dn : {0, today, today + 14, agenda_days_from_civil(2024, 2, 29),
                     agenda_days_from_civil(2027, 1, 1)}) {
    int yy = 0, mm2 = 0, dd = 0;
    agenda_civil_from_days(dn, &yy, &mm2, &dd);
    CHECK(agenda_days_from_civil(yy, mm2, dd) == dn);
  }
  int wy = 0, wm = 0, wd = 0;
  agenda_civil_from_days(agenda_days_from_civil(2026, 7, 31) + 1, &wy, &wm, &wd);
  CHECK(wy == 2026 && wm == 8 && wd == 1);  // month rollover

  // Time ranges, all-day spans (exclusive end), and relative markers.
  {
    AgendaList rich;
    rich.add("2026-07-21T09:00:00-04:00", "2026-07-21T10:00:00-04:00",
             "Doctor", "Clinic", 1);
    rich.add("2026-07-17", "2026-07-20", "Art Show", "", 0);
    rich.add("2026-07-22", "2026-07-23", "Jen off", "", 2);
    rich.finalize(kAgendaMaxEvents);
    const auto &re = rich.entries();
    CHECK(re.size() == 3);
    CHECK(re[0].summary == "Art Show" && re[0].source == 0);
    CHECK(re[1].summary == "Doctor" && re[1].source == 1);
    CHECK(re[2].summary == "Jen off" && re[2].source == 2);
    CHECK(re[1].location == "Clinic");

    char range[48];
    agenda_format_range(range, sizeof(range), re[1], true, today);
    CHECK(std::strcmp(range, "9:00 AM - 10:00 AM") == 0);
    // Multi-day all-day: HA's end date is exclusive, so 17..20 displays as
    // ending the 19th.
    agenda_format_range(range, sizeof(range), re[0], true, today);
    CHECK(std::strncmp(range, "All day, ends ", 14) == 0);
    // Single all-day (22..23 exclusive = one day).
    agenda_format_range(range, sizeof(range), re[2], true, today);
    CHECK(std::strcmp(range, "All day") == 0);

    char rel[16];
    agenda_format_relative(rel, sizeof(rel), today, today);
    CHECK(rel[0] == '\0');
    agenda_format_relative(rel, sizeof(rel), today + 1, today);
    CHECK(rel[0] == '\0');
    agenda_format_relative(rel, sizeof(rel), today + 2, today);
    CHECK(std::strcmp(rel, "in 2 days") == 0);
    agenda_format_relative(rel, sizeof(rel), today + 9, today);
    CHECK(std::strcmp(rel, "in 9 days") == 0);

    // Timed event without an end renders just the start time.
    AgendaList open_ended;
    open_ended.add("2026-07-21T09:00:00-04:00", nullptr, "Solo", "", 0);
    open_ended.finalize(kAgendaMaxEvents);
    agenda_format_range(range, sizeof(range), open_ended.entries()[0], true, today);
    CHECK(std::strcmp(range, "9:00 AM") == 0);
  }

  // Entity CSV splitting trims and drops blanks.
  auto ents = agenda_split_entities(" calendar.family , calendar.work ,, ");
  CHECK(ents.size() == 2);
  CHECK(ents[0] == "calendar.family" && ents[1] == "calendar.work");
  CHECK(agenda_split_entities("").empty());
  CHECK(agenda_split_entities("   ").empty());

  CHECK(agenda_entity_id("calendar.family:#66BB6A") == "calendar.family");
  CHECK(agenda_entity_id("calendar.family") == "calendar.family");
  CHECK(agenda_entity_color("calendar.family:#66BB6A") == 0x66BB6Au);
  CHECK(agenda_entity_color("calendar.family:#66bb6a") == 0x66BB6Au);
  CHECK(agenda_entity_color("calendar.family") == 0u);
  CHECK(agenda_entity_color("calendar.family:#xyzzy1") == 0u);
  CHECK(agenda_entity_color("calendar.family:#fff") == 0u);
  {
    // Day counts drive the overlay's "+N more" marker.
    AgendaList day_list;
    day_list.add("2026-07-18 09:00:00", nullptr, "A", "", 0);
    day_list.add("2026-07-18 11:00:00", nullptr, "B", "", 0);
    day_list.add("2026-07-19 09:00:00", nullptr, "C", "", 0);
    day_list.finalize(8, 0);
    const int32_t d18 = agenda_days_from_civil(2026, 7, 18);
    const int32_t d19 = agenda_days_from_civil(2026, 7, 19);
    CHECK(agenda_events_on_day(day_list.entries(), d18) == 2);
    CHECK(agenda_events_on_day(day_list.entries(), d19) == 1);
    CHECK(agenda_events_on_day(day_list.entries(), d19 + 1) == 0);
    CHECK(agenda_events_on_day({}, d18) == 0);
  }

  {
    // Multi-day all-day events read as prose: "ends tomorrow", not "Tomorrow".
    AgendaList span;
    span.add("2026-07-17", "2026-07-19", "Art Show", "", 0);
    span.finalize(8, 0);
    char buf[48];
    const int32_t today = agenda_days_from_civil(2026, 7, 17);
    agenda_format_range(buf, sizeof(buf), span.entries().front(), true, today);
    CHECK(std::string(buf) == "All day, ends tomorrow");
  }

  auto colored = agenda_entity_ids(" calendar.a:#FF0000 , calendar.b ");
  CHECK(colored.size() == 2);
  CHECK(colored[0] == "calendar.a");
  CHECK(colored[1] == "calendar.b");
  auto one = agenda_split_entities("calendar.solo");
  CHECK(one.size() == 1 && one[0] == "calendar.solo");

  return EXIT_SUCCESS;
}
