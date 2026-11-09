#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "page_store.h"

namespace espcontrol::pages {
struct PageViewGeometry {
  uint16_t visible_slots{0};
  uint16_t columns{0};
};
struct PageCellLayout {
  CardId card{NO_CARD};
  uint8_t column{0}, row{0}, column_span{1}, row_span{1};
  bool visible{false}, back{false};
};
struct PageBackAppearance {
  // Empty means use the active locale's Back translation at display time.
  std::string custom_label;
  uint32_t background_color{0};
  bool custom_color{false};
};

// Preparation-only adapter over the same parsers used by the existing grid.
// It writes at most capacity cells. No layouts are parsed during navigation.
bool compile_page_layout(const PageStore &store, PageId page, PageViewGeometry geometry,
                         PageCellLayout *cells, size_t capacity, PageBackAppearance *back);
}  // namespace espcontrol::pages
