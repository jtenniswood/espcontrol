#pragma once
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

#include "bounded_display_text.h"
#include "lvgl.h"
#include "page_forecast_runtime.h"
#include "weather_card_layout.h"
#include "weather_card_view.h"
#include "weather_condition.h"
namespace espcontrol::pages {
using PageForecastView = weather::View;
struct PageForecastTextStyle {
  const lv_font_t *font{nullptr}, *temperature_font{nullptr}, *icon_font{nullptr};
  const char* (*translate)(const char*){nullptr};
  float (*temperature)(float, const char*){nullptr};
  int32_t scale_x{256}, scale_y{256}, letter_space{0}, line_space{0};
  lv_base_dir_t direction{LV_BASE_DIR_AUTO};
};
struct PageForecastDrawText {
  weather::TextBox box;
  const lv_font_t* font{nullptr};
  const char* glyph{nullptr};  // Firmware-lifetime glyph; other text is owned.
  char text[65]{};
  lv_opa_t opacity{LV_OPA_COVER};
};
struct PageForecastFrame {
  std::array<PageForecastDrawText, weather::MAX_DAYS * 3> texts{};
  size_t count{0};
  char caption[65]{};
  const char* placeholder{nullptr};
  PageForecastTextStyle style;
  static const char* translated(const PageForecastTextStyle& style, const char* key) {
    const auto* value = style.translate ? style.translate(key) : nullptr;
    return value ? value : key;
  }
  static void temperature(char* output, size_t capacity, float value, const char* unit,
                          const PageForecastTextStyle& style) {
    if (style.temperature && std::isfinite(value)) value = style.temperature(value, unit);
    const double rounded = std::round(static_cast<double>(value));
    if (!std::isfinite(rounded) || rounded < std::numeric_limits<int>::min() ||
        rounded > std::numeric_limits<int>::max())
      std::snprintf(output, capacity, "--");
    else
      std::snprintf(output, capacity, "%d", static_cast<int>(rounded));
  }
  static void range(char* output, size_t capacity, const weather::Day& day, const char* unit,
                    const PageForecastTextStyle& style) {
    char high[16], low[16];
    temperature(high, sizeof(high), day.high, unit, style);
    temperature(low, sizeof(low), day.low, unit, style);
    std::snprintf(output, capacity, "%s°/%s°", high, low);
  }
  static const char* weekday(const weather::Day& day, const PageForecastTextStyle& style) {
    static const char* keys[] = {"Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"};
    const auto index = weather::weekday(day.date);
    return index < 0 || index > 6 ? "" : translated(style, keys[index]);
  }
  static int measured(const char* text, const lv_font_t* font) {
    lv_point_t size;
    lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x;
  }
  void add(weather::TextBox box, const lv_font_t* font, const char* text, bool glyph = false,
           lv_opa_t opacity = LV_OPA_COVER) {
    if (count == texts.size() || box.width <= 0) return;
    auto& entry = texts[count++];
    entry = {};
    entry.box = box;
    entry.font = font;
    entry.opacity = opacity;
    if (glyph)
      entry.glyph = text;
    else
      copy_display_text(entry.text, sizeof(entry.text), text);
    lv_point_t measured;
    lv_text_get_size(&measured, glyph ? text : entry.text, font, style.letter_space, style.line_space,
                     std::max(1, box.width), LV_TEXT_FLAG_NONE);
    entry.box.height = measured.y;
  }
  void build(const PageForecastSnapshot& snapshot, PageForecastView view, int width, int height,
             PageForecastTextStyle next, const char* const* glyphs, const char* loading_glyph) {
    count = 0;
    caption[0] = '\0';
    placeholder = nullptr;
    style = next;
    if (!style.font) style.font = LV_FONT_DEFAULT;
    if (!style.temperature_font) style.temperature_font = style.font;
    if (!style.icon_font) style.icon_font = style.font;
    const auto& forecast = snapshot.forecast;
    const auto first = weather::selected_day(forecast, view);
    if (!forecast.count || first >= forecast.count) {
      placeholder = snapshot.loading ? loading_glyph : glyphs[weather::condition_index("unavailable")];
      copy_display_text(caption, sizeof(caption),
                        translated(style, snapshot.loading ? "Loading" : "Forecast unavailable"));
      return;
    }
    if (width <= 0 || height <= 0) return;
    const auto font_height = lv_font_get_line_height(style.font),
               temperature_height = lv_font_get_line_height(style.temperature_font),
               icon_height = lv_font_get_line_height(style.icon_font);
    const size_t available = view == PageForecastView::DAILY ? forecast.count : 1;
    int minimum_width = std::max<int>(icon_height, measured("-188°", style.font));
    for (const auto* key : {"Today", "Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"})
      minimum_width = std::max(minimum_width, measured(translated(style, key), style.font));
    minimum_width += 24;
    for (size_t i = first; i < first + available; ++i) {
      char text[40];
      range(text, sizeof(text), forecast.days[i], forecast.temperature_unit, style);
      const auto size = measured(text, style.temperature_font);
      minimum_width = std::max(minimum_width, size + 24);
    }
    const auto arrangement =
        weather::card_frame_layout(width, height, font_height, temperature_height, icon_height, minimum_width,
                                   available);
    for (size_t i = 0; i < arrangement.layout.count; ++i) {
      const auto& day = forecast.days[first + i];
      const auto& boxes = arrangement.days[i];
      const char* date = view == PageForecastView::TOMORROW           ? translated(style, "Tomorrow")
                         : std::strcmp(day.date, forecast.today) == 0 ? translated(style, "Today")
                                                                      : weekday(day, style);
      add(boxes.icon, style.icon_font, glyphs[weather::condition_index(day.condition)], true);
      add(boxes.day, style.font, date);
      char text[40];
      range(text, sizeof(text), day, forecast.temperature_unit, style);
      add(boxes.temperature, style.temperature_font, text, false, LV_OPA_70);
    }
  }
};
inline lv_area_t forecast_transform_area(lv_area_t area, lv_point_t pivot, int32_t scale_x, int32_t scale_y,
                                         bool inverse) {
  lv_point_t points[4] = {
      {area.x1, area.y1}, {area.x1, area.y2 + 1}, {area.x2 + 1, area.y1}, {area.x2 + 1, area.y2 + 1}};
  if (inverse) {
    scale_x = (256 * 256 + scale_x - 1) / scale_x;
    scale_y = (256 * 256 + scale_y - 1) / scale_y;
  }
  lv_point_array_transform(points, 4, 0, scale_x, scale_y, &pivot, !inverse);
  return {std::min({points[0].x, points[1].x, points[2].x, points[3].x}),
          std::min({points[0].y, points[1].y, points[2].y, points[3].y}),
          std::max({points[0].x, points[1].x, points[2].x, points[3].x}),
          std::max({points[0].y, points[1].y, points[2].y, points[3].y})};
}
inline bool forecast_intersect(lv_area_t& area, const lv_area_t& other) {
  area = {std::max(area.x1, other.x1), std::max(area.y1, other.y1), std::min(area.x2, other.x2),
          std::min(area.y2, other.y2)};
  return area.x1 <= area.x2 && area.y1 <= area.y2;
}
// Copy one command before calling into LVGL. The caller validates its binding
// again before reading the next command, so reentrant model retirement cannot
// leave a pointer into a freed frame. LVGL owns a text copy for queued tasks.
inline void draw_forecast_text(lv_layer_t* layer, PageForecastDrawText entry, lv_point_t origin,
                               const PageForecastTextStyle& style, lv_color_t color, lv_opa_t text_opacity,
                               bool antialias) {
  if (entry.box.width <= 0 || entry.box.height <= 0) return;
  lv_area_t area = {origin.x + entry.box.x, origin.y + entry.box.y,
                    origin.x + entry.box.x + entry.box.width - 1,
                    origin.y + entry.box.y + entry.box.height - 1};
  lv_draw_label_dsc_t label;
  lv_draw_label_dsc_init(&label);
  label.font = entry.font;
  label.text = entry.glyph ? entry.glyph : entry.text;
  label.text_local = 1;
  label.color = color;
  label.opa = text_opacity == LV_OPA_COVER ? entry.opacity : LV_OPA_MIX2(entry.opacity, text_opacity);
  label.align = LV_TEXT_ALIGN_LEFT;
  label.letter_space = style.letter_space;
  label.line_space = style.line_space;
  label.bidi_dir = style.direction;
  if (style.scale_x == 256 && style.scale_y == 256) {
    lv_draw_label(layer, &label, &area);
    return;
  }
  // Match LVGL labels' glyph overhang and transform rounding margins.
  const int32_t overhang = lv_font_get_line_height(entry.font) / 4;
  lv_area_t object_area = area;
  lv_area_increase(&object_area, overhang, overhang);
  const lv_point_t pivot = {area.x1, area.y1};
  auto buffer_area = forecast_transform_area(object_area, pivot, style.scale_x, style.scale_y, false);
  // Read the current task-creation clip before queuing work. This is the same
  // clipped transform reservation used by LVGL 9.5 widgets, rather than a full
  // card-sized ARGB buffer for each line of text.
  if (!forecast_intersect(buffer_area, layer->_clip_area)) return;
  buffer_area = forecast_transform_area(buffer_area, pivot, style.scale_x, style.scale_y, true);
  if (!forecast_intersect(buffer_area, object_area)) return;
  lv_area_increase(&buffer_area, 5, 5);
  auto* transformed = lv_draw_layer_create(layer, LV_COLOR_FORMAT_ARGB8888, &buffer_area);
  if (!transformed) return;
  lv_draw_label(transformed, &label, &area);
  lv_draw_image_dsc_t image;
  lv_draw_image_dsc_init(&image);
  image.src = transformed;
  image.scale_x = style.scale_x;
  image.scale_y = style.scale_y;
  image.pivot = {area.x1 - buffer_area.x1, area.y1 - buffer_area.y1};
  image.image_area = object_area;
  image.antialias = antialias;
  lv_draw_layer(layer, &image, &buffer_area);
}
}  // namespace espcontrol::pages
