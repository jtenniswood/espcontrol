#pragma once

#include <cstdint>

// Forecast widgets exist only on configured forecast cards. The modal allocates
// only visible forecast columns and a stale-status label, sharing the owner's source.
enum class WeatherForecastView : uint8_t { DAILY, TODAY, TOMORROW };
struct WeatherDailyColumn { lv_obj_t *icon, *day, *high; };
struct WeatherDailyCard {
  lv_obj_t *owner = nullptr, *root = nullptr, *caption = nullptr, *placeholder_icon = nullptr;
  WeatherDailySource *source = nullptr;
  WeatherForecastView view = WeatherForecastView::DAILY;
  const lv_font_t *font = nullptr, *icon_font = nullptr, *summary_font = nullptr, *detail_font = nullptr, *temperature_font = nullptr;
  std::vector<WeatherDailyColumn> columns;
  int width = 0, height = 0;
  espcontrol::weather::CardLayout layout;
};
inline std::vector<WeatherDailyCard *> &weather_daily_cards() {
  static std::vector<WeatherDailyCard *> cards;
  return cards;
}
inline std::string weather_daily_temperature(float value, const char *unit) {
  if (!std::isfinite(value)) return "--";
  char text[16];
  std::snprintf(text, sizeof(text), "%d", static_cast<int>(std::round(
    convert_temperature_value_for_display_float(value, unit))));
  return text;
}
inline std::string weather_daily_temperature_range(const espcontrol::weather::Day &day, const char *unit) {
  return weather_daily_temperature(day.high, unit) + "\u00B0/" + weather_daily_temperature(day.low, unit) + "\u00B0";
}
inline std::string weather_daily_weekday_label(const espcontrol::weather::Day &day) {
  static const char *const weekdays[] = {"Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"};
  const int index = espcontrol::weather::weekday(day.date);
  if (index < 0 || index >= 7) return "";
  return espcontrol_i18n(std::string(weekdays[index]));
}
inline std::string weather_daily_date_label(const espcontrol::weather::Day &day,
                                           const espcontrol::weather::Forecast &forecast) {
  return std::strcmp(day.date, forecast.today) == 0 ? espcontrol_i18n(std::string("Today")) : weather_daily_weekday_label(day);
}
// Select by Home Assistant's local date, rather than assuming the first entry is today.
// A provider may omit either requested day. Do not substitute a different date.
inline size_t weather_daily_selected_day(const WeatherDailyCard &card) {
  const auto &forecast = card.source->forecast;
  if (card.view == WeatherForecastView::DAILY) return 0;
  const char *date = card.view == WeatherForecastView::TODAY ? forecast.today : forecast.tomorrow;
  for (size_t i = 0; i < forecast.count; ++i)
    if (std::strcmp(forecast.days[i].date, date) == 0) return i;
  return forecast.count;
}
inline lv_obj_t *weather_daily_label(lv_obj_t *parent, const lv_font_t *font) {
  auto *label = lv_label_create(parent);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_set_style_text_color(label, lv_color_hex(current_theme().text_primary), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  apply_text_width_compensation(label);
  return label;
}
inline int weather_daily_minimum_width(const lv_font_t *font, const lv_font_t *icon_font, int padding, bool include_today = true) {
  lv_point_t measured;
  lv_text_get_size(&measured, "-188\u00B0", font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  int width = std::max(static_cast<int>(measured.x), static_cast<int>(lv_font_get_line_height(icon_font)));
  static const char *const labels[] = {"Today", "Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"};
  for (const char *key : labels) {
    if (!include_today && std::strcmp(key, "Today") == 0) continue;
    const auto text = espcontrol_i18n(std::string(key));
    lv_text_get_size(&measured, text.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    width = std::max(width, static_cast<int>(measured.x));
  }
  return width + padding;
}
inline int weather_daily_day_gap(const lv_font_t *font) {
  return std::max(8, static_cast<int>(lv_font_get_line_height(font)));
}
inline int weather_daily_forecast_minimum_width(const lv_font_t *font, const lv_font_t *icon_font,
    const lv_font_t *temperature_font, const espcontrol::weather::Forecast &forecast,
    size_t first, size_t count, int padding = 24, bool include_today = true) {
  int minimum = weather_daily_minimum_width(font, icon_font, padding, include_today);
  for (size_t i = first; i < std::min(first + count, forecast.count); ++i) {
    const auto temperature = weather_daily_temperature_range(forecast.days[i], forecast.temperature_unit);
    lv_point_t measured;
    lv_text_get_size(&measured, temperature.c_str(), temperature_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    minimum = std::max(minimum, static_cast<int>(measured.x) + padding);
  }
  return minimum;
}
#include "button_grid_weather_modal.h"

inline void weather_daily_render_card(WeatherDailyCard *card) {
  if (!card || !card->source || !card->root) return;
  auto &forecast = card->source->forecast;
  const int width = lv_obj_get_content_width(card->owner);
  const int height = lv_obj_get_content_height(card->owner);
  if (width <= 0 || height <= 0) return;
  const size_t first = weather_daily_selected_day(*card);
  if (!forecast.count || first >= forecast.count) {
    // Reuse the normal card icon until real days arrive; no placeholder columns.
    if (!card->columns.empty()) {
      lv_obj_clean(card->root);
      card->columns.clear();
    }
    lv_obj_add_flag(card->root, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_display_text(card->placeholder_icon, card->source->loading
      ? find_icon("Weather Cloudy Clock") : weather_icon_for_state("unavailable"));
    lv_obj_clear_flag(card->placeholder_icon, LV_OBJ_FLAG_HIDDEN);
    const char *status_key = card->source->loading ? "Loading forecast" : "Forecast unavailable";
    lv_label_set_display_text(card->caption, espcontrol_i18n(std::string(status_key)).c_str());
    return;
  }
  lv_obj_add_flag(card->placeholder_icon, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(card->root, LV_OBJ_FLAG_HIDDEN);
  const int font_height = lv_font_get_line_height(card->font);
  const int temperature_height = lv_font_get_line_height(card->temperature_font);
  const int day_gap = weather_daily_day_gap(card->font);
  const int icon_height = lv_font_get_line_height(card->icon_font);
  const size_t available_days = card->view == WeatherForecastView::DAILY ? forecast.count : 1;
  const int minimum_day_width = weather_daily_forecast_minimum_width(card->font, card->icon_font,
    card->temperature_font, forecast, first, available_days);
  const int caption_height = card->source->stale ? font_height + 2 : 0;
  const int content_height = std::max(1, height - caption_height);
  const int row_gap = std::max(8, font_height / 2), list_gap = std::max(8, font_height / 3);
  const int minimum_height = icon_height + font_height + temperature_height + 4;
  int list_text_width = 0;
  for (size_t i = first; i < first + available_days; ++i) {
    lv_point_t measured;
    const auto text = weather_daily_temperature_range(forecast.days[i], forecast.temperature_unit);
    lv_text_get_size(&measured, text.c_str(), card->temperature_font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    list_text_width = std::max(list_text_width, static_cast<int>(measured.x));
  }
  const auto layout = espcontrol::weather::card_layout(width, content_height, minimum_day_width,
    minimum_height, icon_height + list_gap + list_text_width,
    std::max(icon_height, font_height + temperature_height + 2), day_gap, row_gap, available_days);
  const bool grid = !layout.list && layout.rows > 1;
  const int icon_gap = grid ? std::min(std::max(6, font_height / 3),
    (content_height - row_gap * static_cast<int>(layout.rows - 1)) / static_cast<int>(layout.rows)
      - icon_height - font_height - temperature_height - 2) : 2;
  const int group_height = icon_height + icon_gap + font_height + temperature_height + 2;
  // Group each icon with its text; use spare height to separate whole rows.
  const int grid_row_gap = grid ? std::min(std::max(row_gap, font_height),
    (content_height - group_height * static_cast<int>(layout.rows)) / static_cast<int>(layout.rows - 1)) : row_gap;
  // Keep widgets stable on refresh; rebuild only when the geometry or layout changes.
  if (card->width != width || card->height != height || card->columns.size() != layout.count ||
      card->layout.columns != layout.columns || card->layout.rows != layout.rows || card->layout.list != layout.list) {
    lv_obj_clean(card->root); card->columns.clear();
    card->width = width; card->height = height;
    card->layout = layout;
    lv_obj_set_size(card->root, width, height);
    lv_obj_align(card->root, LV_ALIGN_TOP_LEFT, 0, 0);
    for (size_t i = 0; i < layout.count; ++i) {
      auto make = [&](const lv_font_t *font) {
        auto *label = weather_daily_label(card->root, font);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        return label;
      };
      WeatherDailyColumn col{make(card->icon_font), make(card->font), make(card->temperature_font)};
      theme_set_content_foreground(col.high);
      card->columns.push_back(col);
    }
  }
  std::string caption;
  if (card->source->stale) caption = espcontrol_i18n(std::string("Last forecast"));
  lv_label_set_display_text(card->caption, caption.c_str());
  for (size_t i = 0; i < card->columns.size(); ++i) {
    const auto &col = card->columns[i];
    const auto &day = forecast.days[first + i];
    const int column = static_cast<int>(i % layout.columns), row = static_cast<int>(i / layout.columns);
    const int usable_width = width - day_gap * static_cast<int>(layout.columns - 1);
    const int usable_height = content_height - grid_row_gap * static_cast<int>(layout.rows - 1);
    const int base_width = usable_width / static_cast<int>(layout.columns), remainder_width = usable_width % layout.columns;
    const int base_height = usable_height / static_cast<int>(layout.rows), remainder_height = usable_height % layout.rows;
    const int cell_width = base_width + (column < remainder_width ? 1 : 0);
    const int cell_height = base_height + (row < remainder_height ? 1 : 0);
    const int x = column * (base_width + day_gap) + std::min(column, remainder_width);
    const int y = row * (base_height + grid_row_gap) + std::min(row, remainder_height);
    const int icon_y = y + (layout.list ? (cell_height - icon_height) / 2
      : grid ? (cell_height - group_height) / 2 : 0);
    const int text_x = x + (layout.list ? icon_height + list_gap : 0);
    const int text_width = cell_width - (text_x - x);
    const int text_top = layout.list ? y + (cell_height - font_height - temperature_height - 2) / 2
      : grid ? icon_y + icon_height + icon_gap
      : y + std::max(icon_height + 2, cell_height - temperature_height - font_height - 2);
    lv_obj_set_width(col.icon, layout.list ? icon_height : cell_width);
    lv_obj_set_pos(col.icon, x, icon_y);
    lv_obj_set_width(col.day, text_width); lv_obj_set_width(col.high, text_width);
    lv_obj_set_pos(col.day, text_x, text_top);
    lv_obj_set_pos(col.high, text_x, text_top + font_height + 2);
    std::string date = card->view == WeatherForecastView::TOMORROW
      ? espcontrol_i18n(std::string("Tomorrow")) : weather_daily_date_label(day, forecast);
    if (layout.list) {
      lv_point_t measured;
      lv_text_get_size(&measured, date.c_str(), card->font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
      if (measured.x > text_width) date = weather_daily_weekday_label(day);
    }
    lv_label_set_display_text(col.day, date.c_str());
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    const auto high = weather_daily_temperature_range(day, forecast.temperature_unit);
    lv_obj_set_style_text_color(col.high, lv_color_hex(current_theme().text_muted), LV_PART_MAIN);
    lv_label_set_display_text(col.high, high.c_str());
  }
}
inline void weather_daily_card_clicked(lv_event_t *event);
inline void weather_daily_card_resized(lv_event_t *event);
inline void weather_daily_refresh_visuals() {
  for (auto *card : weather_daily_cards()) weather_daily_render_card(card);
  weather_daily_refresh_modal();
  if (!weather_daily_cards().empty()) notify_dashboard_content_changed();
}
inline WeatherDailyCard *weather_daily_card_for(lv_obj_t *owner) {
  for (auto *card : weather_daily_cards()) if (card->owner == owner) return card;
  return nullptr;
}
inline void weather_daily_card_clicked(lv_event_t *event) {
  weather_daily_open_modal(weather_daily_card_for(static_cast<lv_obj_t *>(lv_event_get_target(event))));
}
inline void weather_daily_card_resized(lv_event_t *event) {
  const auto code = lv_event_get_code(event);
  if (code != LV_EVENT_SIZE_CHANGED && code != LV_EVENT_STYLE_CHANGED) return;
  weather_daily_render_card(weather_daily_card_for(static_cast<lv_obj_t *>(lv_event_get_target(event))));
}
inline void weather_daily_cleanup(lv_obj_t *owner) {
  if (auto *card = weather_daily_card_for(owner)) lv_obj_del(card->root);
}
inline void weather_daily_reset_cards() {
  // Delete old home and subpage roots before any replacement source is allocated.
  // Their delete callbacks release sources, modal ownership and resize handlers.
  while (!weather_daily_cards().empty()) lv_obj_del(weather_daily_cards().back()->root);
}
inline void weather_daily_unavailable_slot(BtnSlot &slot) {
  lv_label_set_display_text(slot.text_lbl, espcontrol_i18n(std::string("Forecast unavailable")).c_str());
  lv_obj_clear_flag(slot.text_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_display_text(slot.icon_lbl, weather_icon_for_state("unavailable"));
  lv_obj_clear_flag(slot.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
}
inline void weather_daily_setup(BtnSlot &slot, const ParsedCfg &config,
                                const DisplayProfile &display) {
  if (!espcontrol::weather::entity_id_safe(config.entity) || weather_daily_cards().size() >= 16) {
    weather_daily_unavailable_slot(slot); return;
  }
  auto *source = weather_daily_acquire_source(config.entity);
  if (!source) {
    weather_daily_unavailable_slot(slot);
    ESP_LOGW("weather_forecast", "Daily forecast source limit reached (8)"); return;
  }
  auto *card = new (std::nothrow) WeatherDailyCard;
  if (!card) { weather_daily_release_source(source); weather_daily_unavailable_slot(slot); return; }
  card->owner = slot.btn; card->source = source; card->caption = slot.text_lbl;
  card->view = config.precision == "today" ? WeatherForecastView::TODAY
    : config.precision == "tomorrow" || config.type == "weather_forecast"
      ? WeatherForecastView::TOMORROW : WeatherForecastView::DAILY;
  card->placeholder_icon = slot.icon_lbl;
  card->font = lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  if (!card->font) card->font = LV_FONT_DEFAULT;
  card->temperature_font = display_small_text_font(display, card->font);
  card->icon_font = display_icon_font(display);
  if (!card->icon_font) card->icon_font = card->font;
  card->summary_font = display_media_title_font(display);
  if (!card->summary_font) card->summary_font = card->font;
  card->detail_font = card->font; // Match card labels; volume-label fonts are larger on some panels.
  card->root = lv_obj_create(slot.btn);
  if (!card->root) { weather_daily_release_source(source); delete card; weather_daily_unavailable_slot(slot); return; }
  lv_obj_set_style_bg_opa(card->root, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(card->root, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(card->root, 0, LV_PART_MAIN);
  lv_obj_clear_flag(card->root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(card->root, LV_OBJ_FLAG_CLICKABLE);
  weather_daily_cards().push_back(card);
  lv_obj_add_event_cb(card->root, [](lv_event_t *event) {
    auto *card = static_cast<WeatherDailyCard *>(lv_event_get_user_data(event));
    if (weather_daily_modal().owner == card->owner) control_modal_force_close_active();
    auto &cards = weather_daily_cards(); cards.erase(std::remove(cards.begin(), cards.end(), card), cards.end());
    lv_obj_remove_event_cb(card->owner, weather_daily_card_clicked);
    lv_obj_remove_event_cb(card->owner, weather_daily_card_resized);
    weather_daily_release_source(card->source); delete card;
    if (cards.empty()) std::vector<WeatherDailyCard *>().swap(cards);
  }, LV_EVENT_DELETE, card);
  // Register once per owner; subsequent configuration changes reuse this callback.
  lv_obj_add_event_cb(slot.btn, weather_daily_card_resized, LV_EVENT_ALL, nullptr);
  weather_daily_render_card(card);
}
