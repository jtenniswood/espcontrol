#pragma once

// Six visible day panels share the card's bounded seven-day forecast snapshot.
// Each panel reuses one two-line label for rain chance and wind speed.
struct WeatherDailyModalColumn { lv_obj_t *button, *day, *icon, *temperature, *conditions; };
struct WeatherDailyModal {
  lv_obj_t *overlay = nullptr, *owner = nullptr, *body = nullptr;
  lv_obj_t *placeholder_icon = nullptr, *message = nullptr, *details = nullptr;
  lv_obj_t *days = nullptr, *status = nullptr;
  WeatherDailySource *source = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr, *temperature_font = nullptr;
  std::vector<WeatherDailyModalColumn> columns;
  size_t day = 0, day_limit = 6, columns_per_row = 3;
  int row_width = 0, row_height = 0, forecast_height = 0;
  int gap = 6, day_gap = 0, day_padding = 8, day_vertical_padding = 8, text_gap = 2;
  bool side_header = false;
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
inline std::string weather_daily_modal_conditions(const WeatherDailyModal &ui,
    const espcontrol::weather::Day &day, int width) {
  const auto &forecast = ui.source->forecast;
  std::string result;
  auto append = [&](const char *key, std::string value) {
    auto text = espcontrol_i18n(std::string(key)) + " " + value;
    lv_point_t measured;
    lv_text_get_size(&measured, text.c_str(), ui.temperature_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    // Keep the value and provider units intact when a translated label is too wide.
    if (measured.x > width) text = std::move(value);
    if (!result.empty()) result += "\n";
    result += text;
  };
  if (std::isfinite(day.rain_probability)) append("Rain", weather_daily_modal_number(day.rain_probability, true));
  else if (std::isfinite(day.precipitation)) {
    auto value = weather_daily_modal_number(day.precipitation);
    if (forecast.precipitation_unit[0]) value += " " + std::string(forecast.precipitation_unit);
    append("Rain", value);
  }
  if (std::isfinite(day.wind)) {
    auto value = weather_daily_modal_number(day.wind);
    if (forecast.wind_unit[0]) value += " " + std::string(forecast.wind_unit);
    append("Wind", value);
  }
  return result;
}
inline void weather_daily_modal_refresh_details(WeatherDailyModal &ui) {
  const auto &forecast = ui.source->forecast;
  const auto &day = forecast.days[ui.day];
  std::string details;
  if (std::isfinite(day.precipitation)) {
    details = espcontrol_i18n(std::string("Rain")) + " " + weather_daily_modal_number(day.precipitation);
    if (forecast.precipitation_unit[0]) details += " " + std::string(forecast.precipitation_unit);
  }
  if (std::isfinite(day.humidity)) {
    if (!details.empty()) details += " / ";
    details += espcontrol_i18n(std::string("Humidity")) + " " + weather_daily_modal_number(day.humidity, true);
  }
  lv_label_set_display_text(ui.details, details.c_str());
  if (details.empty()) lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
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
    lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
    if (!ui.columns.empty()) { lv_obj_clean(ui.days); ui.columns.clear(); }
    lv_obj_add_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ui.status, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_add_flag(ui.message, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.placeholder_icon, LV_OBJ_FLAG_HIDDEN);
  const size_t count = std::min(ui.day_limit, forecast.count);
  ui.day = std::min(ui.day, count - 1);
  lv_obj_clear_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
  const int available = ui.row_width - ui.day_gap * 2;
  const int width = available / 3, remainder = available % 3;
  if (ui.columns.size() != count) {
    lv_obj_clean(ui.days); ui.columns.clear();
    const int icon_height = lv_font_get_line_height(ui.icon_font);
    const int text_height = lv_font_get_line_height(ui.font);
    const int small_height = lv_font_get_line_height(ui.temperature_font);
    const int icon_y = ui.day_vertical_padding;
    const int day_y = ui.side_header ? icon_y : icon_y + icon_height + ui.text_gap;
    const int temperature_y = day_y + text_height + 2;
    const int info_y = ui.side_header ? std::max(icon_y + icon_height, temperature_y + small_height) + 4
      : temperature_y + small_height + 4;
    for (size_t i = 0; i < count; ++i) {
      const int column = static_cast<int>(i % 3), row = static_cast<int>(i / 3);
      const int column_width = width + (column < remainder ? 1 : 0);
      auto *button = weather_daily_modal_container(ui.days, column_width, ui.row_height);
      lv_obj_set_pos(button, column * (width + ui.day_gap) + std::min(column, remainder), row * (ui.row_height + ui.gap));
      lv_obj_set_style_bg_color(button, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
      lv_obj_set_style_radius(button, text_height / 2, LV_PART_MAIN);
      lv_obj_add_flag(button, LV_OBJ_FLAG_CLICKABLE);
      control_modal_apply_pressed_fill(button);
      lv_obj_add_event_cb(button, weather_daily_modal_select_day, LV_EVENT_CLICKED, reinterpret_cast<void *>(i + 1));
      auto make = [&](const lv_font_t *font, int y, int x) {
        auto *label = weather_daily_label(button, font);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        lv_obj_set_width(label, column_width - ui.day_padding - x);
        lv_obj_set_pos(label, x, y); return label;
      };
      const int header_x = ui.day_padding + (ui.side_header ? icon_height + 4 : 0);
      WeatherDailyModalColumn col{button, make(ui.font, day_y, header_x), make(ui.icon_font, icon_y, ui.day_padding),
        make(ui.temperature_font, temperature_y, header_x), make(ui.temperature_font, info_y, ui.day_padding)};
      lv_obj_set_width(col.icon, icon_height);
      lv_obj_set_height(col.conditions, small_height * 2);
      lv_obj_set_style_text_color(col.temperature, lv_color_hex(current_theme().text_muted), LV_PART_MAIN);
      ui.columns.push_back(col);
    }
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &day = forecast.days[i];
    const auto &col = ui.columns[i];
    const int column_width = width + (static_cast<int>(i % 3) < remainder ? 1 : 0);
    lv_obj_set_style_bg_opa(col.button, i == ui.day ? LV_OPA_10 : LV_OPA_TRANSP, LV_PART_MAIN);
    auto date = weather_daily_date_label(day, forecast);
    lv_point_t measured;
    lv_text_get_size(&measured, date.c_str(), ui.font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    if (measured.x > column_width - ui.day_padding * 2 - (ui.side_header ? static_cast<int>(lv_font_get_line_height(ui.icon_font)) + 4 : 0))
      date = weather_daily_weekday_label(day);
    lv_label_set_display_text(col.day, date.c_str());
    lv_obj_set_style_text_opa(col.day, i == ui.day ? LV_OPA_COVER : LV_OPA_70, LV_PART_MAIN);
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    lv_label_set_display_text(col.temperature, weather_daily_temperature_range(day, forecast.temperature_unit).c_str());
    const auto info = weather_daily_modal_conditions(ui, day, column_width - ui.day_padding * 2);
    lv_label_set_display_text(col.conditions, info.c_str());
    if (info.empty()) lv_obj_add_flag(col.conditions, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(col.conditions, LV_OBJ_FLAG_HIDDEN);
  }
  weather_daily_modal_refresh_details(ui);
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
  ui.day = selected; ui.font = card->detail_font; ui.icon_font = card->icon_font; ui.temperature_font = card->temperature_font;
  ++ui.source->users;
  set_clock_bar_modal_label(espcontrol_i18n(std::string("Weather")));
  const int top = shell.layout.back_size + shell.layout.inset;
  const int height = std::max(60, static_cast<int>(shell.layout.panel_h) - ui.day_padding - top);
  const int icon_height = lv_font_get_line_height(ui.icon_font);
  const int text_height = lv_font_get_line_height(ui.font);
  const int small_height = lv_font_get_line_height(ui.temperature_font);
  ui.row_width = shell.content_w; ui.day_gap = weather_daily_day_gap(ui.font);
  ui.forecast_height = height - small_height - ui.gap;
  ui.row_height = (ui.forecast_height - ui.gap) / 2;
  // A side-by-side icon/header preserves readable existing fonts on short landscape panels.
  const int vertical_content = icon_height + text_height + small_height * 3 + ui.text_gap + 6;
  ui.side_header = vertical_content + 8 > ui.row_height;
  const int content_height = ui.side_header ? std::max(icon_height, text_height + 2 + small_height) + 4 + small_height * 2
    : vertical_content;
  ui.day_vertical_padding = std::max(4, std::min(8, (ui.row_height - content_height) / 2));
  ui.body = weather_daily_modal_container(shell.panel, ui.row_width, height);
  lv_obj_align(ui.body, LV_ALIGN_TOP_MID, 0, top);
  lv_obj_clear_flag(shell.panel, LV_OBJ_FLAG_SCROLLABLE);
  ui.days = weather_daily_modal_container(ui.body, ui.row_width, ui.forecast_height);
  lv_obj_set_pos(ui.days, 0, 0);
  ui.details = weather_daily_label(ui.body, ui.temperature_font);
  lv_obj_set_size(ui.details, ui.row_width, small_height);
  lv_obj_set_pos(ui.details, 0, ui.forecast_height + ui.gap);
  lv_obj_set_style_text_align(ui.details, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_obj_set_style_text_color(ui.details, lv_color_hex(current_theme().text_muted), LV_PART_MAIN);
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
