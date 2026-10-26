#pragma once

#include <cctype>
#include <cstring>
#include <limits>
#include <string>

#include "button_grid_limits.h"

// Shared grid-order parser; no LVGL types or widget ownership.
// ── Grid layout parsing ───────────────────────────────────────────────

// Result of parsing a button_order CSV string into grid cell positions
struct OrderResult {
  int positions[MAX_GRID_SLOTS] = {};    // slot number at each grid position (1-based, 0=empty)
  int row_span[MAX_GRID_SLOTS] = {};     // number of grid rows used by each slot
  int col_span[MAX_GRID_SLOTS] = {};     // number of grid columns used by each slot
};

constexpr int CARD_SIZE_SINGLE_ROW_SPAN = 1;
constexpr int CARD_SIZE_SINGLE_COL_SPAN = 1;
constexpr char CARD_SIZE_TALL_TOKEN = 'd';
constexpr int CARD_SIZE_TALL_ROW_SPAN = 2;
constexpr int CARD_SIZE_TALL_COL_SPAN = 1;
constexpr char CARD_SIZE_WIDE_TOKEN = 'w';
constexpr int CARD_SIZE_WIDE_ROW_SPAN = 1;
constexpr int CARD_SIZE_WIDE_COL_SPAN = 2;
constexpr char CARD_SIZE_LARGE_TOKEN = 'b';
constexpr int CARD_SIZE_LARGE_ROW_SPAN = 2;
constexpr int CARD_SIZE_LARGE_COL_SPAN = 2;
constexpr char CARD_SIZE_EXTRA_TALL_TOKEN = 't';
constexpr int CARD_SIZE_EXTRA_TALL_ROW_SPAN = 3;
constexpr int CARD_SIZE_EXTRA_TALL_COL_SPAN = 1;
constexpr char CARD_SIZE_EXTRA_WIDE_TOKEN = 'x';
constexpr int CARD_SIZE_EXTRA_WIDE_ROW_SPAN = 1;
constexpr int CARD_SIZE_EXTRA_WIDE_COL_SPAN = 3;
constexpr char CARD_SIZE_EXTRA_LARGE_TOKEN = 'q';
constexpr int CARD_SIZE_EXTRA_LARGE_ROW_SPAN = 3;
constexpr int CARD_SIZE_EXTRA_LARGE_COL_SPAN = 3;
constexpr char CARD_SIZE_MAX_WIDE_TOKEN = 'h';
constexpr int CARD_SIZE_MAX_WIDE_ROW_SPAN = 2;
constexpr int CARD_SIZE_MAX_WIDE_COL_SPAN = 3;
constexpr char CARD_SIZE_MAX_TALL_TOKEN = 'v';
constexpr int CARD_SIZE_MAX_TALL_ROW_SPAN = 3;
constexpr int CARD_SIZE_MAX_TALL_COL_SPAN = 2;
constexpr char CARD_SIZE_PORTRAIT_LARGE_TOKEN = 'p';
constexpr int CARD_SIZE_PORTRAIT_LARGE_ROW_SPAN = 4;
constexpr int CARD_SIZE_PORTRAIT_LARGE_COL_SPAN = 3;
constexpr char CARD_SIZE_LANDSCAPE_LARGE_TOKEN = 'l';
constexpr int CARD_SIZE_LANDSCAPE_LARGE_ROW_SPAN = 3;
constexpr int CARD_SIZE_LANDSCAPE_LARGE_COL_SPAN = 4;
constexpr char CARD_SIZE_ULTRA_WIDE_TOKEN = 'u';
constexpr int CARD_SIZE_ULTRA_WIDE_ROW_SPAN = 1;
constexpr int CARD_SIZE_ULTRA_WIDE_COL_SPAN = 5;

inline bool card_span_matches(int row_span, int col_span, int expected_rows, int expected_cols) {
  return row_span == expected_rows && col_span == expected_cols;
}

inline bool card_span_is_single(int row_span, int col_span) {
  return card_span_matches(row_span, col_span, CARD_SIZE_SINGLE_ROW_SPAN, CARD_SIZE_SINGLE_COL_SPAN);
}

inline bool card_span_is_wide(int row_span, int col_span) {
  return card_span_matches(row_span, col_span, CARD_SIZE_WIDE_ROW_SPAN, CARD_SIZE_WIDE_COL_SPAN);
}

inline bool card_span_is_large(int row_span, int col_span) {
  return card_span_matches(row_span, col_span, CARD_SIZE_LARGE_ROW_SPAN, CARD_SIZE_LARGE_COL_SPAN);
}

inline void grid_token_spans(char suffix, int &row_span, int &col_span) {
  row_span = CARD_SIZE_SINGLE_ROW_SPAN;
  col_span = CARD_SIZE_SINGLE_COL_SPAN;
  if (suffix == CARD_SIZE_TALL_TOKEN) {
    row_span = CARD_SIZE_TALL_ROW_SPAN;
    col_span = CARD_SIZE_TALL_COL_SPAN;
  } else if (suffix == CARD_SIZE_WIDE_TOKEN) {
    row_span = CARD_SIZE_WIDE_ROW_SPAN;
    col_span = CARD_SIZE_WIDE_COL_SPAN;
  } else if (suffix == CARD_SIZE_LARGE_TOKEN) {
    row_span = CARD_SIZE_LARGE_ROW_SPAN;
    col_span = CARD_SIZE_LARGE_COL_SPAN;
  } else if (suffix == CARD_SIZE_EXTRA_TALL_TOKEN) {
    row_span = CARD_SIZE_EXTRA_TALL_ROW_SPAN;
    col_span = CARD_SIZE_EXTRA_TALL_COL_SPAN;
  } else if (suffix == CARD_SIZE_EXTRA_WIDE_TOKEN) {
    row_span = CARD_SIZE_EXTRA_WIDE_ROW_SPAN;
    col_span = CARD_SIZE_EXTRA_WIDE_COL_SPAN;
  } else if (suffix == CARD_SIZE_EXTRA_LARGE_TOKEN) {
    row_span = CARD_SIZE_EXTRA_LARGE_ROW_SPAN;
    col_span = CARD_SIZE_EXTRA_LARGE_COL_SPAN;
  } else if (suffix == CARD_SIZE_MAX_WIDE_TOKEN) {
    row_span = CARD_SIZE_MAX_WIDE_ROW_SPAN;
    col_span = CARD_SIZE_MAX_WIDE_COL_SPAN;
  } else if (suffix == CARD_SIZE_MAX_TALL_TOKEN) {
    row_span = CARD_SIZE_MAX_TALL_ROW_SPAN;
    col_span = CARD_SIZE_MAX_TALL_COL_SPAN;
  } else if (suffix == CARD_SIZE_PORTRAIT_LARGE_TOKEN) {
    row_span = CARD_SIZE_PORTRAIT_LARGE_ROW_SPAN;
    col_span = CARD_SIZE_PORTRAIT_LARGE_COL_SPAN;
  } else if (suffix == CARD_SIZE_LANDSCAPE_LARGE_TOKEN) {
    row_span = CARD_SIZE_LANDSCAPE_LARGE_ROW_SPAN;
    col_span = CARD_SIZE_LANDSCAPE_LARGE_COL_SPAN;
  } else if (suffix == CARD_SIZE_ULTRA_WIDE_TOKEN) {
    row_span = CARD_SIZE_ULTRA_WIDE_ROW_SPAN;
    col_span = CARD_SIZE_ULTRA_WIDE_COL_SPAN;
  }
}

inline bool grid_token_has_span_suffix(char suffix) {
  return suffix == CARD_SIZE_TALL_TOKEN || suffix == CARD_SIZE_WIDE_TOKEN ||
    suffix == CARD_SIZE_LARGE_TOKEN || suffix == CARD_SIZE_EXTRA_TALL_TOKEN ||
    suffix == CARD_SIZE_EXTRA_WIDE_TOKEN || suffix == CARD_SIZE_EXTRA_LARGE_TOKEN ||
    suffix == CARD_SIZE_MAX_WIDE_TOKEN || suffix == CARD_SIZE_MAX_TALL_TOKEN ||
    suffix == CARD_SIZE_PORTRAIT_LARGE_TOKEN || suffix == CARD_SIZE_LANDSCAPE_LARGE_TOKEN ||
    suffix == CARD_SIZE_ULTRA_WIDE_TOKEN;
}

inline int parse_positive_int_span(const std::string &value, size_t start, size_t end) {
  while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) start++;
  int result = 0;
  bool has_digit = false;
  for (size_t i = start; i < end; i++) {
    char ch = value[i];
    if (ch < '0' || ch > '9') break;
    has_digit = true;
    const int digit = ch - '0';
    if (result > (std::numeric_limits<int>::max() - digit) / 10) return 0;
    result = result * 10 + digit;
  }
  return has_digit ? result : 0;
}

// Parse "1,2d,3w,4b,5t,6x,..." into positions + row/column spans
inline void parse_order_string(const std::string &order_str, int num_slots, OrderResult &result) {
  memset(result.positions, 0, sizeof(result.positions));
  for (int i = 0; i < MAX_GRID_SLOTS; i++) {
    result.row_span[i] = 1;
    result.col_span[i] = 1;
  }
  int slot_limit = bounded_grid_slots(num_slots);
  if (order_str.empty()) return;
  size_t gpos = 0, start = 0;
  while (start <= order_str.length() && gpos < (size_t)slot_limit) {
    size_t comma = order_str.find(',', start);
    if (comma == std::string::npos) comma = order_str.length();
    if (comma > start) {
      size_t token_end = comma;
      int row_span = 1, col_span = 1;
      while (token_end > start && std::isspace(static_cast<unsigned char>(order_str[token_end - 1]))) {
        token_end--;
      }
      if (token_end > start && grid_token_has_span_suffix(order_str[token_end - 1])) {
        grid_token_spans(order_str[token_end - 1], row_span, col_span);
        token_end--;
      }
      int v = parse_positive_int_span(order_str, start, token_end);
      if (v >= 1 && v <= slot_limit) {
        result.positions[gpos] = v;
        result.row_span[v - 1] = row_span;
        result.col_span[v - 1] = col_span;
      }
    }
    gpos++;
    start = comma + 1;
  }
}

// Saved layouts can originate on a larger display or an older web editor.
// Downgrade any card that extends beyond the active grid before passing its
// cell coordinates to LVGL.
inline void normalize_grid_span_for_position(int position, int num_slots,
                                             int cols, int &row_span,
                                             int &col_span) {
  int slot_limit = bounded_grid_slots(num_slots);
  if (position < 0 || position >= slot_limit || cols <= 0) {
    row_span = 1;
    col_span = 1;
    return;
  }
  if (row_span < 1) row_span = 1;
  if (col_span < 1) col_span = 1;
  int rows = (slot_limit + cols - 1) / cols;
  int row = position / cols;
  int col = position % cols;
  int last_cell = position + (row_span - 1) * cols + col_span - 1;
  if (row + row_span > rows || col + col_span > cols ||
      last_cell >= slot_limit) {
    row_span = 1;
    col_span = 1;
  }
}

// Zero out grid cells that are covered by a neighbouring multi-cell button
inline void clear_spanned_cells(const OrderResult &order, int num_slots, int cols, OrderResult &result) {
  int slot_limit = bounded_grid_slots(num_slots);
  for (int p = 0; p < slot_limit; p++) {
    result.positions[p] = order.positions[p];
    result.row_span[p] = order.row_span[p] > 0 ? order.row_span[p] : 1;
    result.col_span[p] = order.col_span[p] > 0 ? order.col_span[p] : 1;
  }
  for (int p = 0; p < slot_limit; p++) {
    if (result.positions[p] <= 0) continue;
    int idx = result.positions[p] - 1;
    int row_span = result.row_span[idx] > 0 ? result.row_span[idx] : 1;
    int col_span = result.col_span[idx] > 0 ? result.col_span[idx] : 1;
    normalize_grid_span_for_position(p, slot_limit, cols, row_span, col_span);
    result.row_span[idx] = row_span;
    result.col_span[idx] = col_span;
    int col = p % cols;
    for (int r = 0; r < row_span; r++) {
      for (int c = 0; c < col_span; c++) {
        if (r == 0 && c == 0) continue;
        if (col + c >= cols) continue;
        int covered = p + r * cols + c;
        if (covered < slot_limit) result.positions[covered] = 0;
      }
    }
  }
}
