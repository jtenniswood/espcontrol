#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int row_span, int col_span) {
  // A 6 px inset makes the compact square miss LVGL's next integer QR module
  // scale on 4-inch grids, leaving a much larger implicit border. Keep enough
  // separation from the card edge while allowing the code to use that scale.
  return row_span == 1 && col_span == 1 ? 3 : 6;
}
