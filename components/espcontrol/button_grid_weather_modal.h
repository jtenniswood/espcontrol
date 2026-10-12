#pragma once

// Included by the weather widget layer after its shared formatting helpers.
// Allocate at most seven day panels and three metric tiles; release callbacks on close.
struct WeatherDailyModalColumn { lv_obj_t *button, *day, *icon, *high, *low; };
struct WeatherDailyModalMetric { lv_obj_t *tile, *label, *value, *unit; };
struct WeatherDailyModal {
  lv_obj_t *overlay = nullptr, *owner = nullptr, *condition = nullptr, *name = nullptr;
  lv_obj_t *icon = nullptr, *temperature = nullptr, *range = nullptr, *details = nullptr;
  lv_obj_t *days = nullptr, *metrics = nullptr, *status = nullptr;
  WeatherDailySource *source = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr, *value_font = nullptr;
  std::vector<WeatherDailyModalColumn> columns;
  std::vector<WeatherDailyModalMetric> metric_tiles;
  size_t day = 0, day_limit = 5, columns_per_row = 1;
  int row_width = 0, row_height = 0;
  // Current conditions exist only for the open modal, not once per card/source.
  char current_condition[33]{}, friendly_name[65]{}, current_unit[13]{};
  float current_temperature = espcontrol::weather::MISSING;
};
inline WeatherDailyModal &weather_daily_modal() { static WeatherDailyModal ui; return ui; }
inline void weather_daily_hide_modal() {
  auto &ui = weather_daily_modal();
  auto *source = ui.source;
  if (ui.overlay) ha_release_callbacks_for_owner(ui.overlay);
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
  struct Metric { const char *key; std::string value, unit; } values[3];
  size_t count = 0;
  if (std::isfinite(day.rain_probability)) {
    std::string rainfall;
    if (std::isfinite(day.precipitation)) {
      rainfall = weather_daily_modal_number(day.precipitation);
      if (forecast.precipitation_unit[0]) rainfall += " " + std::string(forecast.precipitation_unit);
    }
    values[count++] = {"Rain chance", weather_daily_modal_number(day.rain_probability, true), rainfall};
  } else if (std::isfinite(day.precipitation)) {
    values[count++] = {"Rainfall", weather_daily_modal_number(day.precipitation), forecast.precipitation_unit};
  }
  if (std::isfinite(day.wind)) values[count++] = {"Wind", weather_daily_modal_number(day.wind), forecast.wind_unit};
  if (std::isfinite(day.humidity)) values[count++] = {"Humidity", weather_daily_modal_number(day.humidity, true), ""};
  if (ui.metric_tiles.size() != count) {
    lv_obj_clean(ui.metrics); ui.metric_tiles.clear();
    if (count) {
      const int gap = 12, text_height = lv_font_get_line_height(ui.font);
      const int value_height = lv_font_get_line_height(ui.value_font);
      const int height = text_height * 3 + value_height + 24;
      const int available = ui.row_width - gap * static_cast<int>(count - 1);
      const int width = available / static_cast<int>(count), remainder = available % static_cast<int>(count);
      lv_obj_set_height(ui.metrics, height);
      for (size_t i = 0; i < count; ++i) {
        const int index = static_cast<int>(i), tile_width = width + (index < remainder ? 1 : 0);
        auto *tile = weather_daily_modal_container(ui.metrics, tile_width, height);
        lv_obj_set_pos(tile, index * (width + gap) + std::min(index, remainder), 0);
        lv_obj_set_style_bg_color(tile, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(tile, LV_OPA_10, LV_PART_MAIN);
        lv_obj_set_style_radius(tile, text_height / 2, LV_PART_MAIN);
        auto label = [&](const lv_font_t *font, int y) {
          auto *obj = weather_daily_label(tile, font);
          lv_obj_set_width(obj, tile_width - 16); lv_obj_set_pos(obj, 8, y); return obj;
        };
        WeatherDailyModalMetric metric{tile, label(ui.font, 8),
          label(ui.value_font, text_height * 2 + 8), label(ui.font, text_height * 2 + value_height + 16)};
        lv_label_set_long_mode(metric.label, LV_LABEL_LONG_WRAP);
        lv_obj_set_height(metric.label, text_height * 2);
        lv_obj_set_style_text_opa(metric.label, LV_OPA_70, LV_PART_MAIN);
        lv_obj_set_style_text_opa(metric.unit, LV_OPA_70, LV_PART_MAIN);
        ui.metric_tiles.push_back(metric);
      }
    }
  }
  for (size_t i = 0; i < count; ++i) {
    lv_label_set_display_text(ui.metric_tiles[i].label, espcontrol_i18n(std::string(values[i].key)).c_str());
    lv_label_set_display_text(ui.metric_tiles[i].value, values[i].value.c_str());
    lv_label_set_display_text(ui.metric_tiles[i].unit, values[i].unit.c_str());
  }
  if (count) {
    const std::string heading = weather_daily_date_label(day, forecast) + " - " + weather_label_for_state(day.condition);
    lv_label_set_display_text(ui.details, heading.c_str());
    lv_obj_clear_flag(ui.details, LV_OBJ_FLAG_HIDDEN); lv_obj_clear_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN); lv_obj_add_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.source->stale) {
    lv_label_set_display_text(ui.status, espcontrol_i18n(std::string("Last forecast")).c_str());
    lv_obj_clear_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
  } else lv_obj_add_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
}
inline void weather_daily_refresh_modal() {
  auto &ui = weather_daily_modal();
  if (!ui.overlay || !ui.source) return;
  const auto &forecast = ui.source->forecast;
  lv_label_set_display_text(ui.name, ui.friendly_name[0] ? ui.friendly_name : ui.source->entity_id.c_str());
  const char *current = ui.current_condition[0] ? ui.current_condition : "unknown";
  const bool current_available = std::strcmp(current, "unknown") != 0 && std::strcmp(current, "unavailable") != 0;
  const float temperature = current_available && ui.current_unit[0]
    ? ui.current_temperature : espcontrol::weather::MISSING;
  const std::string temperature_text = weather_daily_temperature(temperature, ui.current_unit) + display_temperature_unit_symbol();
  lv_label_set_display_text(ui.temperature, temperature_text.c_str());
  // The summary's range belongs to today; selecting another day changes only its details below.
  std::string range;
  for (size_t i = 0; i < forecast.count; ++i) {
    if (std::strcmp(forecast.days[i].date, forecast.today) != 0) continue;
    range = weather_daily_temperature(forecast.days[i].high, forecast.temperature_unit) + "\u00B0 / " +
      weather_daily_temperature(forecast.days[i].low, forecast.temperature_unit) + "\u00B0";
    break;
  }
  lv_label_set_display_text(ui.range, range.c_str());
  if (!forecast.count) {
    const char *status_key = ui.source->loading ? "Loading forecast" : "Forecast unavailable";
    lv_obj_set_style_text_font(ui.condition, ui.font, LV_PART_MAIN);
    lv_label_set_display_text(ui.condition, espcontrol_i18n(std::string(status_key)).c_str());
    lv_label_set_display_text(ui.icon, ui.source->loading
      ? find_icon("Weather Cloudy Clock") : weather_icon_for_state("unavailable"));
    lv_label_set_display_text(ui.details, "");
    lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
    if (!ui.columns.empty()) { lv_obj_clean(ui.days); ui.columns.clear(); }
    lv_obj_add_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clean(ui.metrics); ui.metric_tiles.clear();
    lv_obj_add_flag(ui.metrics, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_set_style_text_font(ui.condition, lv_obj_get_style_text_font(ui.temperature, LV_PART_MAIN), LV_PART_MAIN);
  lv_label_set_display_text(ui.condition, weather_label_for_state(current).c_str());
  lv_label_set_display_text(ui.icon, weather_icon_for_state(current));
  const size_t count = std::min(ui.day_limit, forecast.count);
  ui.day = std::min(ui.day, count - 1);
  lv_obj_clear_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
  // Show the complete limited forecast. Narrow/rotated displays wrap balanced rows.
  // Data updates and day selection reuse widgets when the count is unchanged.
  if (ui.columns.size() != count) {
    lv_obj_clean(ui.days); ui.columns.clear();
    const int gap = 12;
    const size_t rows = (count + ui.columns_per_row - 1) / ui.columns_per_row;
    lv_obj_set_height(ui.days, static_cast<int>(rows) * (ui.row_height + gap) - gap);
    const int text_height = lv_font_get_line_height(ui.font);
    const int icon_y = text_height + 12;
    const int high_y = icon_y + lv_font_get_line_height(ui.icon_font) + 12;
    size_t row = 0, column = 0;
    for (size_t i = 0; i < count; ++i) {
      const size_t row_count = count / rows + (row < count % rows ? 1 : 0);
      const int available = ui.row_width - gap * static_cast<int>(row_count - 1);
      const int width = available / static_cast<int>(row_count), remainder = available % static_cast<int>(row_count);
      const int index = static_cast<int>(column);
      const int column_width = width + (index < remainder ? 1 : 0);
      auto *button = weather_daily_modal_container(ui.days, column_width, ui.row_height);
      lv_obj_set_pos(button, index * (width + gap) + std::min(index, remainder), static_cast<int>(row) * (ui.row_height + gap));
      lv_obj_set_style_bg_color(button, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
      lv_obj_set_style_radius(button, text_height / 2, LV_PART_MAIN);
      if (++column == row_count) { ++row; column = 0; }
      lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
      control_modal_apply_pressed_fill(button);
      lv_obj_add_event_cb(button, weather_daily_modal_select_day, LV_EVENT_CLICKED,
        reinterpret_cast<void *>(i + 1));
      auto make = [&](const lv_font_t *font, int y) {
        auto *label = weather_daily_label(button, font);
        lv_obj_set_width(label, column_width); lv_obj_set_pos(label, 0, y); return label;
      };
      WeatherDailyModalColumn col{button, make(ui.font, 0), make(ui.icon_font, icon_y),
        make(ui.font, high_y), make(ui.font, high_y + text_height + 4)};
      lv_obj_set_style_text_opa(col.day, LV_OPA_70, LV_PART_MAIN);
      lv_obj_set_style_text_opa(col.low, LV_OPA_70, LV_PART_MAIN);
      ui.columns.push_back(col);
    }
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &day = forecast.days[i];
    const auto &col = ui.columns[i];
    lv_obj_set_style_bg_opa(col.button, i == ui.day ? LV_OPA_10 : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_label_set_display_text(col.day, weather_daily_date_label(day, forecast).c_str());
    lv_obj_set_style_text_opa(col.day, i == ui.day ? LV_OPA_COVER : LV_OPA_70, LV_PART_MAIN);
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    const std::string high = weather_daily_temperature(day.high, forecast.temperature_unit) + "\u00B0";
    const std::string low = weather_daily_temperature(day.low, forecast.temperature_unit) + "\u00B0";
    lv_label_set_display_text(col.high, high.c_str()); lv_label_set_display_text(col.low, low.c_str());
  }
  weather_daily_modal_refresh_metrics(ui);
}
inline void weather_daily_modal_bind_current_conditions(WeatherDailyModal &ui) {
  auto *overlay = ui.overlay;
  HaCallbackOwnerScope owner_scope(overlay);
  auto update = [overlay](int field, esphome::StringRef state) {
    auto &modal = weather_daily_modal();
    if (modal.overlay != overlay) return;
    const size_t limit = field == 1 ? 64 : field == 2 ? 24 : field == 3 ? 12 : 32;
    const std::string text = string_ref_limited(state, limit);
    if (field == 0) std::snprintf(modal.current_condition, sizeof(modal.current_condition), "%s", text.c_str());
    else if (field == 1) {
      const bool missing = text.empty() || text == "unknown" || text == "unavailable" || text == "None";
      std::snprintf(modal.friendly_name, sizeof(modal.friendly_name), "%s", missing ? "" : text.c_str());
    } else if (field == 2) {
      const float temperature = espcontrol::weather::number(text);
      modal.current_temperature = std::fabs(temperature) <= 500 ? temperature : espcontrol::weather::MISSING;
    } else std::snprintf(modal.current_unit, sizeof(modal.current_unit), "%s", text.c_str());
    weather_daily_refresh_modal();
    notify_dashboard_content_changed();
  };
  ha_subscribe_state(ui.source->entity_id, [update](esphome::StringRef state) { update(0, state); });
  ha_subscribe_attribute(ui.source->entity_id, "friendly_name", [update](esphome::StringRef state) { update(1, state); });
  ha_subscribe_attribute(ui.source->entity_id, "temperature", [update](esphome::StringRef state) { update(2, state); });
  ha_subscribe_attribute(ui.source->entity_id, "temperature_unit", [update](esphome::StringRef state) { update(3, state); });
  // The modal may add attributes after Home Assistant's subscription handshake.
  ha_reannounce_state_subscriptions();
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
  ui.value_font = card->summary_font;
  // Resolution separates the supported 4/4.3-inch panels from 7/10-inch panels,
  // independently of orientation (the square P4 has a large-layout font profile).
  ui.day_limit = std::max(shell.layout.sw, shell.layout.sh) <= 800 ? 5 : 7;
  ++ui.source->users;
  set_clock_bar_modal_label(espcontrol_i18n(std::string("Weather")));
  const int top = shell.layout.back_size + shell.layout.inset * 2;
  auto *list = control_modal_create_scroll_list(shell.panel, shell.content_w,
    std::max(60, static_cast<int>(shell.layout.panel_h - shell.layout.inset) - top), 12);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, top);
  const int icon_height = lv_font_get_line_height(card->icon_font);
  const int title_height = lv_font_get_line_height(card->summary_font);
  const int text_height = lv_font_get_line_height(ui.font);
  const int header_height = std::max(icon_height, title_height + text_height + 6);
  auto *header = weather_daily_modal_container(list, shell.content_w, header_height);
  lv_point_t measured;
  lv_text_get_size(&measured, "-188\u00B0C", card->summary_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  int temperature_width = measured.x;
  lv_text_get_size(&measured, "-188\u00B0 / -188\u00B0", ui.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  temperature_width = std::min(static_cast<int>(shell.content_w) / 2, std::max(temperature_width, static_cast<int>(measured.x)) + 8);
  const int name_x = icon_height + 12;
  const int name_width = std::max(1, static_cast<int>(shell.content_w) - name_x - temperature_width - 12);
  auto summary_label = [&](const lv_font_t *font, int x, int y, int width, lv_text_align_t align) {
    auto *label = weather_daily_label(header, font);
    lv_obj_set_width(label, width); lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_align(label, align, LV_PART_MAIN); return label;
  };
  ui.icon = summary_label(card->icon_font, 0, 0, icon_height, LV_TEXT_ALIGN_LEFT);
  ui.condition = summary_label(card->summary_font, name_x, 0, name_width, LV_TEXT_ALIGN_LEFT);
  ui.name = summary_label(ui.font, name_x, title_height + 6, name_width, LV_TEXT_ALIGN_LEFT);
  ui.temperature = summary_label(card->summary_font, shell.content_w - temperature_width, 0, temperature_width, LV_TEXT_ALIGN_RIGHT);
  ui.range = summary_label(ui.font, shell.content_w - temperature_width, title_height + 6, temperature_width, LV_TEXT_ALIGN_RIGHT);
  lv_obj_set_style_text_opa(ui.name, LV_OPA_70, LV_PART_MAIN);
  lv_obj_set_style_text_opa(ui.range, LV_OPA_70, LV_PART_MAIN);
  ui.row_width = shell.content_w;
  const int minimum = weather_daily_minimum_width(ui.font, ui.icon_font, 12);
  ui.columns_per_row = espcontrol::weather::visible_days(ui.row_width, minimum, ui.day_limit, 12);
  ui.row_height = text_height * 3 + icon_height + 28;
  ui.days = weather_daily_modal_container(list, ui.row_width, ui.row_height);
  ui.details = weather_daily_label(list, ui.font); lv_obj_set_width(ui.details, shell.content_w);
  lv_obj_set_style_text_align(ui.details, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_label_set_long_mode(ui.details, LV_LABEL_LONG_WRAP);
  ui.metrics = weather_daily_modal_container(list, ui.row_width, 1);
  ui.status = weather_daily_label(list, ui.font); lv_obj_set_width(ui.status, shell.content_w);
  lv_obj_set_style_text_align(ui.status, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_obj_set_style_text_opa(ui.status, LV_OPA_70, LV_PART_MAIN);
  weather_daily_refresh_modal();
  weather_daily_modal_bind_current_conditions(ui);
}
