#pragma once

#include "page_basic_card.h"
namespace espcontrol::pages {
inline bool basic_number_large_font(const BasicCardDefinition& card, int rows, int columns) {
  if (!card.large_numbers) return false;
  if (card.kind == BasicCardKind::CLOCK) return clock_card_layout(card.clock, true, rows, columns).large;
  return card_span_is_large(rows, columns) || (card.kind == BasicCardKind::CLIMATE && card.explicit_large);
}
inline int basic_number_unit_offset(bool large, int font_height, int percent) {
  return large ? font_height * percent / 100 : 0;
}
}  // namespace espcontrol::pages
