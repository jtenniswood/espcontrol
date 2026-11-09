#pragma once

#include <cstdio>
#include <cstring>

#include "page_calendar_runtime.h"
namespace espcontrol::pages {
struct CalendarCardText {
  char value[8]{'-', '-', '\0'};
  char label[32]{};
  const char* static_label{nullptr};
};
inline const char* calendar_month_key(int month) {
  constexpr const char* months[] = {"January", "February", "March",     "April",   "May",      "June",
                                    "July",    "August",   "September", "October", "November", "December"};
  return month >= 1 && month <= 12 ? months[month - 1] : "Date";
}
inline CalendarCardText format_calendar_card(PageCalendarState state, bool show_time,
                                             const char* (*translate)(const char*) = nullptr) {
  const auto caption = [&](const char* key) {
    const auto* value = translate ? translate(key) : nullptr;
    return value ? value : key;
  };
  CalendarCardText result;
  result.static_label = caption("Date");
  const bool date = state.day >= 1 && state.day <= 31 && state.month >= 1 && state.month <= 12;
  if (show_time) {
    if (state.time_valid && date && state.hour >= 0 && state.hour <= 23 && state.minute >= 0 &&
        state.minute <= 59) {
      if (state.use_12h) {
        int hour = state.hour % 12;
        if (!hour) hour = 12;
        std::snprintf(result.value, sizeof(result.value), "%d:%02d", hour, state.minute);
      } else
        std::snprintf(result.value, sizeof(result.value), "%02d:%02d", state.hour, state.minute);
      std::snprintf(result.label, sizeof(result.label), "%d %s", state.day,
                    caption(calendar_month_key(state.month)));
      result.static_label = nullptr;
    }
  } else if (state.date_valid && date) {
    std::snprintf(result.value, sizeof(result.value), "%d", state.day);
    result.static_label = caption(calendar_month_key(state.month));
  }
  return result;
}
}  // namespace espcontrol::pages
