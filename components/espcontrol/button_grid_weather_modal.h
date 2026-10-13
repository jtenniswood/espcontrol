#pragma once

// Included by the weather widget layer after its shared formatting helpers.
// Allocate at most seven day panels and three metric tiles, sharing the card snapshot.
struct WeatherDailyModalColumn { lv_obj_t *button, *day, *icon, *temperature; };
struct WeatherDailyModalMetric { lv_obj_t *tile, *label, *value, *secondary; };
struct WeatherDailyModal {
  lv_obj_t *overlay = nullptr, *owner = nullptr, *body = nullptr;
  lv_obj_t *placeholder_icon = nullptr, *message = nullptr, *details = nullptr;
  lv_obj_t *days = nullptr, *metrics = nullptr, *status = nullptr;
  WeatherDailySource *source = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr, *value_font = nullptr, *temperature_font = nullptr;
  std::vector<WeatherDailyModalColumn> columns;
  std::vector<WeatherDailyModalMetric> metric_tiles;
  size_t day = 0, day_limit = 5, columns_per_row = 1;
  int row_width = 0, row_height = 0, forecast_height = 0;
  int gap = 6, day_gap = 0, row_gap = 6, day_padding = 12, day_vertical_padding = 8, text_gap = 2;
  int metric_height = 0, metric_padding = 6, metric_label_lines = 2;
  bool weekday_labels = false;
};
inline WeatherDailyModal &weather_daily_modal() { static WeatherDailyModal ui; return ui; }
inline void weather_daily_hide_modal() {
  auto &ui = weather_daily_modal();
  auto *source = ui.source;
  control_modal_delete_overlay(ControlModalKind::WEATHER_FORECAST, ui.overlay);
  ui = WeatherDailyModal{};
  weather_daily_release_source(source);
}
inline lv_obj_t *weather_daily_modal_container(lv_obj_t *parent, int width, int height) {
  auto *container = lv_obj_create(parent);
  lv_obj_set_size(container, width, height);
  lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);
  return container;
}
inline void weather_daily_refresh_modal();
inline void weather_daily_modal_select_day(lv_event_t *event) {
  auto &ui = weather_daily_modal();
  if (!ui.source) return;
  const size_t column = reinterpret_cast<uintptr_t>(lv_event_get_user_data(event)) - 1;
  if (column >= ui.columns.size()) return;
  ui.day = column;
  weather_daily_refresh_modal();
}
inline std::string weather_daily_modal_number(float value, bool percent = false) {
  char text[24];
  if (percent) std::snprintf(text, sizeof(text), "%.0f%%", std::round(value));
  else {
    std::snprintf(text, sizeof(text), "%.1f", value);
    const size_t length = std::strlen(text);
    if (length >= 2 && std::strcmp(text + length - 2, ".0") == 0) text[length - 2] = '\0';
  }
  return text;
}
inline void weather_daily_modal_refresh_metrics(WeatherDailyModal &ui) {
  const auto &forecast = ui.source->forecast;
  const auto &day = forecast.days[ui.day];
  // Transient strings only: the shared seven-day snapshot remains the sole data store.
  struct Metric { const char *key; std::string value, unit; bool below = false; } values[3];
  size_t count = 0;
  if (std::isfinite(day.rain_probability)) {
    std::string rainfall;
    if (std::isfinite(day.precipitation)) {
      rainfall = weather_daily_modal_number(day.precipitation);
      if (forecast.precipitation_unit[0]) rainfall += " " + std::string(forecast.precipitation_unit);
    }
    values[count++] = {"Rain", weather_daily_modal_number(day.rain_probability, true), rainfall, true};
  } else if (std::isfinite(day.precipitation)) {
    values[count++] = {"Rain", weather_daily_modal_number(day.precipitation), forecast.precipitation_unit, true};
  }
  if (std::isfinite(day.wind)) values[count++] = {"Wind", weather_daily_modal_number(day.wind), forecast.wind_unit};
  if (std::isfinite(day.humidity)) values[count++] = {"Humidity", weather_daily_modal_number(day.humidity, true), ""};
  if (ui.metric_tiles.size() != count) {
    lv_obj_clean(ui.metrics); ui.metric_tiles.clear();
    if (count) {
      const int gap = ui.gap, text_height = lv_font_get_line_height(ui.font);
      const int height = ui.metric_height;
      const int value_y = ui.metric_padding;
      const int label_y = height - ui.metric_padding - ui.metric_label_lines * text_height;
      const int available = ui.row_width - gap * static_cast<int>(count - 1);
      const int width = available / static_cast<int>(count), remainder = available % static_cast<int>(count);
      lv_obj_set_height(ui.metrics, height);
      for (size_t i = 0; i < count; ++i) {
        const int index = static_cast<int>(i), tile_width = width + (index < remainder ? 1 : 0);
        auto *tile = weather_daily_modal_container(ui.metrics, tile_width, height);
        lv_obj_set_pos(tile, index * (width + gap) + std::min(index, remainder), 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(current_theme().surface_card), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tile, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_radius(tile, text_height / 2, LV_PART_MAIN);
        auto label = [&](const lv_font_t *font, int y) {
          auto *obj = weather_daily_label(tile, font);
          lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
          lv_obj_set_width(obj, tile_width - ui.metric_padding * 2);
          lv_obj_set_pos(obj, ui.metric_padding, y); return obj;
        };
        WeatherDailyModalMetric metric{tile, label(ui.font, label_y), label(ui.value_font, value_y),
          label(ui.temperature_font, value_y + static_cast<int>(lv_font_get_line_height(ui.value_font)) + 2)};
        lv_obj_set_style_text_color(metric.secondary, lv_color_hex(current_theme().text_muted), LV_PART_MAIN);
        lv_label_set_long_mode(metric.label, ui.metric_label_lines == 2 ? LV_LABEL_LONG_WRAP : LV_LABEL_LONG_DOT);
        lv_obj_set_height(metric.label, text_height * ui.metric_label_lines);
        ui.metric_tiles.push_back(metric);
      }
    }
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &tile = ui.metric_tiles[i];
    const int available_width = ui.row_width - ui.gap * static_cast<int>(count - 1);
    const int tile_width = available_width / static_cast<int>(count) +
      (static_cast<int>(i) < available_width % static_cast<int>(count) ? 1 : 0);
    const int usable = tile_width - ui.metric_padding * 2;
    const auto label = espcontrol_i18n(std::string(values[i].key));
    lv_label_set_display_text(tile.label, label.c_str());
    lv_point_t label_size;
    lv_text_get_size(&label_size, label.c_str(), ui.font, 0, 0, usable, LV_TEXT_FLAG_NONE);
    const int text_height = lv_font_get_line_height(ui.font);
    const int label_height = std::min(text_height * ui.metric_label_lines, std::max(text_height, static_cast<int>(label_size.y)));
    lv_obj_set_height(tile.label, label_height);
    lv_obj_set_y(tile.label, ui.metric_height - ui.metric_padding - label_height);
    lv_label_set_display_text(tile.value, values[i].value.c_str());
    lv_label_set_display_text(tile.secondary, values[i].unit.c_str());
    lv_point_t number_size, unit_size;
    lv_text_get_size(&number_size, values[i].value.c_str(), ui.value_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    lv_text_get_size(&unit_size, values[i].unit.c_str(), ui.temperature_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    const bool inline_unit = !values[i].below && number_size.x + unit_size.x + 4 <= usable;
    lv_obj_set_width(tile.value, inline_unit ? std::min(usable, static_cast<int>(number_size.x)) : usable);
    const int unit_x = ui.metric_padding + (inline_unit ? static_cast<int>(number_size.x) + 4 : 0);
    const int unit_y = ui.metric_padding + static_cast<int>(lv_font_get_line_height(ui.value_font)) +
      (inline_unit ? -static_cast<int>(lv_font_get_line_height(ui.temperature_font)) : 2);
    lv_obj_set_pos(tile.secondary, unit_x, unit_y);
    lv_obj_set_width(tile.secondary, usable - (unit_x - ui.metric_padding));
    if (values[i].unit.empty()) lv_obj_add_flag(tile.secondary, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(tile.secondary, LV_OBJ_FLAG_HIDDEN);
  }
  if (count) {
    const std::string heading = weather_daily_date_label(day, forecast) + " - " + weather_label_for_state(day.condition);
    lv_label_set_display_text(ui.details, heading.c_str());
    lv_obj_clear_flag(ui.details, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
  }
  int status_width = 0;
  if (ui.source->stale) {
    const auto text = espcontrol_i18n(std::string("Last forecast"));
    lv_point_t measured;
    lv_text_get_size(&measured, text.c_str(), ui.temperature_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    status_width = std::min(ui.row_width / 2, static_cast<int>(measured.x) + 4);
    lv_obj_set_width(ui.status, status_width);
    lv_obj_set_pos(ui.status, ui.row_width - status_width, ui.forecast_height + ui.gap);
    lv_label_set_display_text(ui.status, text.c_str());
    lv_obj_clear_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
  } else lv_obj_add_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_width(ui.details, ui.row_width - (status_width ? status_width + ui.gap : 0));
}
inline void weather_daily_refresh_modal() {
  auto &ui = weather_daily_modal();
  if (!ui.overlay || !ui.source) return;
  const auto &forecast = ui.source->forecast;
  if (!forecast.count) {
    const char *status_key = ui.source->loading ? "Loading forecast" : "Forecast unavailable";
    lv_label_set_display_text(ui.message, espcontrol_i18n(std::string(status_key)).c_str());
    lv_label_set_display_text(ui.placeholder_icon, ui.source->loading
      ? find_icon("Weather Cloudy Clock") : weather_icon_for_state("unavailable"));
    lv_obj_clear_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ui.placeholder_icon, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_display_text(ui.details, "");
    lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
    if (!ui.columns.empty()) { lv_obj_clean(ui.days); ui.columns.clear(); }
    lv_obj_add_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(ui.metrics); ui.metric_tiles.clear();
    lv_obj_add_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.placeholder_icon, LV_OBJ_FLAG_HIDDEN);
  const size_t count = std::min(ui.day_limit, forecast.count);
  ui.day = std::min(ui.day, count - 1);
  lv_obj_clear_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
  // Show the complete limited forecast. Narrow/rotated displays wrap rows with consistent day widths.
  // Data updates and day selection reuse widgets when the count is unchanged.
  if (ui.columns.size() != count) {
    lv_obj_clean(ui.days); ui.columns.clear();
    const size_t columns = std::min(count, ui.columns_per_row);
    lv_obj_set_height(ui.days, ui.forecast_height);
    const int text_height = lv_font_get_line_height(ui.font);
    const int icon_y = ui.day_vertical_padding;
    const int day_y = icon_y + static_cast<int>(lv_font_get_line_height(ui.icon_font)) + ui.text_gap;
    const int temperature_y = day_y + text_height + 2;
    const int available = ui.row_width - ui.day_gap * static_cast<int>(columns - 1);
    const int width = available / static_cast<int>(columns), remainder = available % static_cast<int>(columns);
    for (size_t i = 0; i < count; ++i) {
      const int column = static_cast<int>(i % columns), row = static_cast<int>(i / columns);
      const int column_width = width + (column < remainder ? 1 : 0);
      auto *button = weather_daily_modal_container(ui.days, column_width, ui.row_height);
      lv_obj_set_pos(button, column * (width + ui.day_gap) + std::min(column, remainder), row * (ui.row_height + ui.row_gap));
      lv_obj_set_style_bg_color(button, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
      lv_obj_set_style_radius(button, text_height / 2, LV_PART_MAIN);
      lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
      control_modal_apply_pressed_fill(button);
      lv_obj_add_event_cb(button, weather_daily_modal_select_day, LV_EVENT_CLICKED,
        reinterpret_cast<void *>(i + 1));
      auto make = [&](const lv_font_t *font, int y) {
        auto *label = weather_daily_label(button, font);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_width(label, column_width - ui.day_padding * 2); lv_obj_set_pos(label, ui.day_padding, y); return label;
      };
      WeatherDailyModalColumn col{button, make(ui.font, day_y), make(ui.icon_font, icon_y),
        make(ui.temperature_font, temperature_y)};
      lv_obj_set_style_text_color(col.temperature, lv_color_hex(current_theme().text_muted), LV_PART_MAIN);
      ui.columns.push_back(col);
    }
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &day = forecast.days[i];
    const auto &col = ui.columns[i];
    lv_obj_set_style_bg_opa(col.button, i == ui.day ? LV_OPA_10 : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_label_set_display_text(col.day, (ui.weekday_labels ? weather_daily_weekday_label(day) : weather_daily_date_label(day, forecast)).c_str());
    lv_obj_set_style_text_opa(col.day, i == ui.day ? LV_OPA_COVER : LV_OPA_70, LV_PART_MAIN);
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    const auto temperature = weather_daily_temperature_range(day, forecast.temperature_unit);
    lv_label_set_display_text(col.temperature, temperature.c_str());
  }
  weather_daily_modal_refresh_metrics(ui);
}
inline int weather_daily_modal_minimum_width(const WeatherDailyModal &ui, bool include_today = true) {
  return weather_daily_forecast_minimum_width(ui.font, ui.icon_font, ui.temperature_font,
    ui.source->forecast, 0, ui.day_limit, ui.day_padding * 2, include_today);
}
inline void weather_daily_open_modal(WeatherDailyCard *card) {
  if (!card || !card->source || !card->owner) return;
  const size_t selected = weather_daily_selected_day(*card);
  if (card->view != WeatherForecastView::DAILY && selected >= card->source->forecast.count) return;
  auto shell = control_modal_open_shell(ControlModalKind::WEATHER_FORECAST,
    card->owner, 100, card->icon_font, weather_daily_hide_modal);
  if (!shell.overlay || !shell.panel) return;
  auto &ui = weather_daily_modal();
  ui.overlay = shell.overlay; ui.owner = card->owner; ui.source = card->source;
  ui.day = selected; ui.font = card->detail_font; ui.icon_font = card->icon_font;
  ui.value_font = card->summary_font; ui.temperature_font = card->temperature_font;
  // Resolution separates the supported 4/4.3-inch panels from 7/10-inch panels,
  // independently of orientation (the square P4 has a large-layout font profile).
  ui.day_limit = std::max(shell.layout.sw, shell.layout.sh) <= 800 ? 5 : 7;
  ++ui.source->users;
  set_clock_bar_modal_label(espcontrol_i18n(std::string("Weather")));
  const int top = shell.layout.back_size + shell.layout.inset;
  const int height = std::max(60, static_cast<int>(shell.layout.panel_h) - ui.day_padding - top);
  ui.row_width = shell.content_w;
  ui.body = weather_daily_modal_container(shell.panel, ui.row_width, height);
  lv_obj_align(ui.body, LV_ALIGN_TOP_MID, 0, top);
  lv_obj_clear_flag(shell.panel, LV_OBJ_FLAG_SCROLLABLE);
  const int icon_height = lv_font_get_line_height(ui.icon_font);
  const int text_height = lv_font_get_line_height(ui.font);
  const int small_height = lv_font_get_line_height(ui.temperature_font);
  const int value_height = lv_font_get_line_height(ui.value_font);
  ui.day_gap = weather_daily_day_gap(ui.font);
  auto columns_for = [&](bool today) {
    return espcontrol::weather::visible_days(ui.row_width, weather_daily_modal_minimum_width(ui, today), ui.day_limit, ui.day_gap);
  };
  ui.columns_per_row = columns_for(true);
  int rows = static_cast<int>((ui.day_limit + ui.columns_per_row - 1) / ui.columns_per_row);
  const int content_height = icon_height + text_height + small_height + ui.text_gap + 2;
  auto metric_minimum = [&]() {
    return value_height + small_height + text_height * ui.metric_label_lines + ui.metric_padding * 2 + 4;
  };
  int minimum_metric = metric_minimum();
  auto forecast_budget = [&]() { return height - minimum_metric - small_height - ui.gap * 2; };
  auto forecast_minimum = [&]() { return rows * (content_height + 8) + (rows - 1) * ui.row_gap; };
  // Reserve space for full-sized metric numbers and a separate grey rainfall line.
  // Short panels use one bottom-label line; main numbers keep the larger font.
  if (forecast_minimum() > forecast_budget()) {
    ui.metric_label_lines = 1;
    minimum_metric = metric_minimum();
  }
  if (forecast_minimum() > forecast_budget()) {
    ui.weekday_labels = true;
    ui.columns_per_row = columns_for(false);
    rows = static_cast<int>((ui.day_limit + ui.columns_per_row - 1) / ui.columns_per_row);
  }
  const int maximum_row = (forecast_budget() - (rows - 1) * ui.row_gap) / rows;
  ui.day_vertical_padding = std::max(4, std::min(8, (maximum_row - content_height) / 2));
  ui.row_height = content_height + ui.day_vertical_padding * 2;
  ui.forecast_height = rows * ui.row_height + (rows - 1) * ui.row_gap;
  ui.metric_height = height - ui.forecast_height - small_height - ui.gap * 2;
  ui.days = weather_daily_modal_container(ui.body, ui.row_width, ui.forecast_height);
  lv_obj_set_pos(ui.days, 0, 0);
  ui.details = weather_daily_label(ui.body, ui.temperature_font);
  lv_obj_set_size(ui.details, ui.row_width, small_height);
  lv_obj_set_pos(ui.details, 0, ui.forecast_height + ui.gap);
  lv_obj_set_style_text_align(ui.details, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  ui.metrics = weather_daily_modal_container(ui.body, ui.row_width, ui.metric_height);
  lv_obj_set_pos(ui.metrics, 0, ui.forecast_height + small_height + ui.gap * 2);
  ui.status = weather_daily_label(ui.body, ui.temperature_font);
  lv_obj_set_size(ui.status, ui.row_width, small_height);
  lv_obj_set_pos(ui.status, 0, ui.forecast_height + ui.gap);
  lv_obj_set_style_text_align(ui.status, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
  lv_obj_set_style_text_opa(ui.status, LV_OPA_70, LV_PART_MAIN);
  ui.placeholder_icon = weather_daily_label(ui.body, ui.icon_font);
  lv_obj_set_size(ui.placeholder_icon, icon_height, icon_height);
  lv_obj_set_pos(ui.placeholder_icon, 0, 0);
  lv_obj_set_style_text_align(ui.placeholder_icon, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  ui.message = weather_daily_label(ui.body, ui.font);
  lv_obj_set_size(ui.message, ui.row_width, text_height);
  lv_obj_set_pos(ui.message, 0, icon_height + ui.gap);
  lv_obj_set_style_text_align(ui.message, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  weather_daily_refresh_modal();
}
