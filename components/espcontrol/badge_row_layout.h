#pragma once
#include <vector>

// Badge rows sit outside the grid tracks and reserve only vertical space.
struct PageBadgeRow {
  lv_obj_t *page = nullptr;
  lv_obj_t *row = nullptr;
  int inset = 0;
  std::vector<lv_obj_t *> buttons;
};
inline std::vector<PageBadgeRow> &page_badge_rows() {
  static std::vector<PageBadgeRow> rows;
  return rows;
}
inline int page_badge_row_inset(lv_obj_t *page) {
  for (const auto &row : page_badge_rows()) if (row.page == page) return row.inset;
  return 0;
}
