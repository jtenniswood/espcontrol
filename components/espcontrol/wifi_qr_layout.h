#pragma once

inline constexpr int wifi_qr_tile_vertical_inset(int, int) {
  return 6;
}

inline constexpr int WIFI_QR_QUIET_ZONE_MODULES = 4;

inline constexpr int wifi_qr_scale_with_quiet_zone(int side, int module_count) {
  return side > 0 && module_count > 0
    ? side / (module_count + WIFI_QR_QUIET_ZONE_MODULES * 2)
    : 0;
}
