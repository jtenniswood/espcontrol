#pragma once
#include <cstdio>

#include "button_grid_order_parser.h"
#include "page_timezone_runtime.h"
namespace espcontrol::pages {
struct ClockCardOptions {
  bool show_label, explicit_large, center;
};
struct ClockCardLayout {
  bool large{false}, wide{false}, center{false};
};
inline ClockCardLayout clock_card_layout(ClockCardOptions options, bool enabled, int rows, int columns) {
  const bool square = card_span_is_large(rows, columns), wide = card_span_is_wide(rows, columns);
  const bool layout = square || (!options.show_label && (wide || columns > CARD_SIZE_WIDE_COL_SPAN));
  ClockCardLayout result;
  result.large = enabled && (layout || (!options.show_label && options.explicit_large));
  result.wide = result.large && ((!options.show_label && columns > CARD_SIZE_WIDE_COL_SPAN) || wide);
  result.center = options.show_label || (columns > CARD_SIZE_WIDE_COL_SPAN && options.center);
  return result;
}
struct ClockCardText {
  char value[8]{"--:--"};
  std::string_view label;
  bool city{false};
};
template <typename Translate>
inline ClockCardText format_clock_card(PageTimezoneSnapshot state, bool show_label,
                                       std::string_view configured_label, Translate translate) {
  ClockCardText result;
  if (state.valid) {
    int hour = state.hour;
    if (state.use_12h) {
      hour %= 12;
      if (!hour) hour = 12;
    }
    std::snprintf(result.value, sizeof(result.value), state.use_12h ? "%d:%02d" : "%02d:%02d", hour,
                  state.minute);
  }
  if (!show_label) return result;
  if (!configured_label.empty()) {
    result.label = configured_label;
    return result;
  }
  const auto slash = state.zone_id.rfind('/');
  result.label = slash == std::string_view::npos ? state.zone_id : state.zone_id.substr(slash + 1);
  if (result.label.empty()) {
    const auto* translated = translate ? translate("World Clock") : nullptr;
    result.label = translated ? translated : "World Clock";
  } else
    result.city = true;
  return result;
}
}  // namespace espcontrol::pages
