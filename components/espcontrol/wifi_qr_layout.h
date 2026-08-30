#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int row_span, int col_span) {
  return row_span == 1 && col_span == 1 ? 6 : 3;
}
