#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int row_span, int col_span) {
  // The compact card needs the smaller inset to keep its QR modules at the
  // next whole-pixel scale.
  return row_span == 1 && col_span == 1 ? 3 : 6;
}

// The compact canvas supplies two blank modules on each side; the surrounding
// white card inset supplies the remaining visual separation from adjacent UI.
inline constexpr int WIFI_QR_COMPACT_MARGIN_MODULES = 2;

inline constexpr int wifi_qr_compact_scale(int side, int module_count) {
  return side > 0 && module_count > 0
    ? side / (module_count + WIFI_QR_COMPACT_MARGIN_MODULES * 2)
    : 0;
}
