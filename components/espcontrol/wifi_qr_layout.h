#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int row_span, int col_span) {
  // The compact card needs the smaller inset to keep its QR modules at the
  // next whole-pixel scale.
  return row_span == 1 && col_span == 1 ? 3 : 6;
}

// QR scanners require a four-module quiet zone around the encoded matrix.
inline constexpr int WIFI_QR_COMPACT_MARGIN_MODULES = 4;

inline constexpr int wifi_qr_compact_scale(int side, int module_count) {
  return side > 0 && module_count > 0
    ? side / (module_count + WIFI_QR_COMPACT_MARGIN_MODULES * 2)
    : 0;
}
