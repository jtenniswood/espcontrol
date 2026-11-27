#pragma once

#include <esp_heap_caps.h>

#include "flights_map_coastline.h"
#include "flights_map_data.h"

struct FlightsMapMarker {
  lv_obj_t *button = nullptr;
  lv_obj_t *plane = nullptr;
  lv_obj_t *label = nullptr;
  espcontrol::flights::MapTrack track;
};

struct FlightsMapUi {
  lv_obj_t *area = nullptr;
  lv_obj_t *canvas = nullptr;
  lv_obj_t *details = nullptr;
  lv_timer_t *timer = nullptr;
  uint16_t *pixels = nullptr;
  int stride = 0;
  int marker_size = 0;
  espcontrol::flights::MapProjection projection;
  std::array<FlightsMapMarker, espcontrol::flights::MAX_FLIGHTS> markers{};
  std::string selected;
};

inline void flights_map_line(FlightsMapUi &map, espcontrol::flights::MapPoint start,
                             espcontrol::flights::MapPoint end, uint16_t color) {
  // Clip before walking the pixels. Offscreen segments may span continents.
  const float dx = end.x - start.x, dy = end.y - start.y;
  float first = 0, last = 1;
  const float p[] = {-dx, dx, -dy, dy};
  const float q[] = {start.x, map.projection.width - 1 - start.x,
                     start.y, map.projection.height - 1 - start.y};
  for (size_t i = 0; i < 4; ++i) {
    if (p[i] == 0) {
      if (q[i] < 0) return;
    } else if (p[i] < 0) {
      first = std::max(first, q[i] / p[i]);
    } else {
      last = std::min(last, q[i] / p[i]);
    }
  }
  if (first > last) return;
  int x = std::clamp(static_cast<int>(std::lround(start.x + dx * first)), 0, map.projection.width - 1);
  int y = std::clamp(static_cast<int>(std::lround(start.y + dy * first)), 0, map.projection.height - 1);
  const int x_end = std::clamp(static_cast<int>(std::lround(start.x + dx * last)), 0, map.projection.width - 1);
  const int y_end = std::clamp(static_cast<int>(std::lround(start.y + dy * last)), 0, map.projection.height - 1);
  const int x_delta = std::abs(x_end - x), y_delta = -std::abs(y_end - y);
  const int x_step = x < x_end ? 1 : -1, y_step = y < y_end ? 1 : -1;
  int error = x_delta + y_delta;
  while (true) {
    map.pixels[y * map.stride + x] = color;
    if (x == x_end && y == y_end) break;
    const int twice = 2 * error;
    if (twice >= y_delta) { error += y_delta; x += x_step; }
    if (twice <= x_delta) { error += x_delta; y += y_step; }
  }
}

inline void flights_map_draw_background(FlightsMapUi &map,
                                       espcontrol::flights::MapBounds bounds) {
  using namespace espcontrol::flights;
  const auto &p = map.projection;
  auto rgb = [](uint32_t hex) { return lv_color_to_u16(lv_color_hex(hex)); };
  std::fill_n(map.pixels, map.stride * p.height, rgb(0x132632));
  const auto grid = rgb(0x263d49);
  for (int part = 1; part < 4; ++part) {
    const float x = p.width * part / 4.0f, y = p.height * part / 4.0f;
    flights_map_line(map, {x, 0}, {x, static_cast<float>(p.height - 1)}, grid);
    flights_map_line(map, {0, y}, {static_cast<float>(p.width - 1), y}, grid);
  }
  const auto coast = rgb(0x739695);
  const float latitude_limit = p.height / p.y_scale / 2 + 1;
  bool have_previous = false;
  MapPoint previous{};
  for (const auto &point : MAP_COASTLINE) {
    if (point.latitude == 32767) {
      have_previous = false;
      continue;
    }
    const float lat = point.latitude / 100.0f;
    if (std::fabs(lat - p.lat) > latitude_limit) {
      have_previous = false;
      continue;
    }
    const auto next = p.project(lat, point.longitude / 100.0f);
    if (have_previous && std::fabs(next.x - previous.x) < 180.0f * p.x_scale)
      flights_map_line(map, previous, next, coast);
    previous = next;
    have_previous = true;
  }
  // Use the unwrapped bounds so the outline stays intact at the date line.
  const float width = (bounds.east - bounds.west) * p.x_scale / 2;
  const float height = (bounds.north - bounds.south) * p.y_scale / 2;
  const MapPoint center = p.project(p.lat, p.lon);
  const MapPoint tl{center.x - width, center.y - height}, br{center.x + width, center.y + height};
  const auto outline = rgb(0x417086);
  flights_map_line(map, tl, {br.x, tl.y}, outline);
  flights_map_line(map, {br.x, tl.y}, br, outline);
  flights_map_line(map, br, {tl.x, br.y}, outline);
  flights_map_line(map, {tl.x, br.y}, tl, outline);
  flights_map_line(map, {center.x - 5, center.y}, {center.x + 5, center.y}, coast);
  flights_map_line(map, {center.x, center.y - 5}, {center.x, center.y + 5}, coast);
  lv_obj_invalidate(map.canvas);
}

inline void flights_map_animate(FlightsMapUi &map) {
  if (!map.area || lv_obj_has_flag(map.area, LV_OBJ_FLAG_HIDDEN)) return;
  const auto &p = map.projection;
  const uint32_t now = lv_tick_get();
  for (auto &marker : map.markers) {
    if (!marker.button || marker.track.id.empty()) continue;
    const auto point = marker.track.position(p, now);
    const int x = std::lround(point.x), y = std::lround(point.y);
    if (x < 0 || y < 0 || x >= p.width || y >= p.height) {
      lv_obj_add_flag(marker.button, LV_OBJ_FLAG_HIDDEN);
      continue;
    }
    lv_obj_clear_flag(marker.button, LV_OBJ_FLAG_HIDDEN);
    const int left = x - map.marker_size / 2, top = y - map.marker_size / 2;
    if (lv_obj_get_x(marker.button) != left || lv_obj_get_y(marker.button) != top)
      lv_obj_set_pos(marker.button, left, top);
  }
}

inline void flights_map_release(FlightsMapUi &map) {
  if (map.timer) lv_timer_del(map.timer);
  if (map.area) lv_obj_del(map.area);
  heap_caps_free(map.pixels);
  map = FlightsMapUi{};
}

inline bool flights_map_create(FlightsMapUi &map, lv_obj_t *parent,
                               const espcontrol::flights::MapProjection &projection,
                               int inset, int top, int marker_size,
                               const lv_font_t *label_font) {
  map.projection = projection;
  map.marker_size = marker_size;
  map.stride = lv_draw_buf_width_to_stride(projection.width, LV_COLOR_FORMAT_RGB565) / 2;
  map.pixels = static_cast<uint16_t *>(heap_caps_aligned_alloc(
      LV_DRAW_BUF_ALIGN, map.stride * projection.height * sizeof(uint16_t),
      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!map.pixels) return false;

  map.area = lv_obj_create(parent);
  lv_obj_remove_style_all(map.area);
  lv_obj_set_size(map.area, projection.width, projection.height);
  lv_obj_align(map.area, LV_ALIGN_TOP_LEFT, inset, top);
  lv_obj_set_style_radius(map.area, 12, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(map.area, true, LV_PART_MAIN);
  lv_obj_clear_flag(map.area, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(map.area, LV_OBJ_FLAG_CLICKABLE);

  map.canvas = lv_canvas_create(map.area);
  lv_canvas_set_buffer(map.canvas, map.pixels, projection.width, projection.height, LV_COLOR_FORMAT_RGB565);
  lv_obj_clear_flag(map.canvas, LV_OBJ_FLAG_CLICKABLE);

  map.details = lv_label_create(map.area);
  lv_obj_set_width(map.details, lv_pct(100));
  lv_label_set_long_mode(map.details, LV_LABEL_LONG_DOT);
  lv_obj_set_style_text_align(map.details, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_text_color(map.details, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_bg_color(map.details, lv_color_hex(0x132632), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(map.details, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_pad_all(map.details, 6, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(map.details, label_font, LV_PART_MAIN);
  lv_obj_align(map.details, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_add_flag(map.details, LV_OBJ_FLAG_HIDDEN);

  map.timer = lv_timer_create([](lv_timer_t *timer) {
    flights_map_animate(*static_cast<FlightsMapUi *>(lv_timer_get_user_data(timer)));
  }, 100, &map);
  return true;
}

inline void flights_map_create_marker(FlightsMapUi &map, FlightsMapMarker &marker,
                                      const lv_font_t *icon_font, const lv_font_t *label_font,
                                      lv_event_cb_t click) {
  marker.button = lv_obj_create(map.area);
  lv_obj_remove_style_all(marker.button);
  lv_obj_set_size(marker.button, map.marker_size, map.marker_size);
  lv_obj_clear_flag(marker.button, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(marker.button, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(marker.button, click, LV_EVENT_CLICKED, &marker);

  marker.plane = lv_label_create(marker.button);
  lv_obj_set_style_text_color(marker.plane, lv_color_hex(0x81e4ff), LV_PART_MAIN);
  if (icon_font) lv_obj_set_style_text_font(marker.plane, icon_font, LV_PART_MAIN);
  lv_label_set_display_text(marker.plane, find_icon("Airplane"));
  lv_obj_update_layout(marker.plane);
  lv_obj_set_style_transform_pivot_x(marker.plane, lv_obj_get_width(marker.plane) / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(marker.plane, lv_obj_get_height(marker.plane) / 2, LV_PART_MAIN);
  lv_obj_center(marker.plane);

  marker.label = lv_label_create(marker.button);
  lv_obj_set_style_text_color(marker.label, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_set_style_bg_color(marker.label, lv_color_hex(0x172e3c), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(marker.label, LV_OPA_90, LV_PART_MAIN);
  lv_obj_set_style_pad_hor(marker.label, 3, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(marker.label, label_font, LV_PART_MAIN);
  lv_obj_align(marker.label, LV_ALIGN_BOTTOM_MID, 0, map.marker_size / 3);
  lv_obj_add_flag(marker.button, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
}
