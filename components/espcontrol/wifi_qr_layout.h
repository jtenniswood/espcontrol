#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int row_span, int col_span) {
  // The compact card needs the smaller inset to keep its QR modules at the
  // next whole-pixel scale while still reserving a four-module quiet zone.
  return row_span == 1 && col_span == 1 ? 3 : 6;
}

inline constexpr int WIFI_QR_QUIET_ZONE_MODULES = 4;

inline constexpr int wifi_qr_scale_with_quiet_zone(int side, int module_count) {
  return side > 0 && module_count > 0
    ? side / (module_count + WIFI_QR_QUIET_ZONE_MODULES * 2)
    : 0;
}
