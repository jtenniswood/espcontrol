#pragma once

#include <cstdint>

// Forecast widgets exist only on configured forecast cards. The modal allocates
// only visible forecast columns and one detail view, sharing the owner's source.
enum class WeatherForecastView : uint8_t { DAILY, TODAY, TOMORROW };
struct WeatherDailyColumn { lv_obj_t *icon, *day, *high; };
struct WeatherDailyCard {
  lv_obj_t *owner = nullptr, *root = nullptr, *caption = nullptr, *placeholder_icon = nullptr;
  WeatherDailySource *source = nullptr;
  WeatherForecastView view = WeatherForecastView::DAILY;
  const lv_font_t *font = nullptr, *icon_font = nullptr, *summary_font = nullptr, *detail_font = nullptr;
  std::vector<WeatherDailyColumn> columns;
  int width = 0, height = 0;
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
inline std::string weather_daily_date_label(const espcontrol::weather::Day &day,
                                           const espcontrol::weather::Forecast &forecast) {
  if (std::strcmp(day.date, forecast.today) == 0) return espcontrol_i18n(std::string("Today"));
  static const char *weekdays[] = {"Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"};
  const int index = espcontrol::weather::weekday(day.date);
  if (index < 0 || index >= 7) return "";
  return espcontrol_i18n(std::string(weekdays[index]));
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
inline int weather_daily_minimum_width(const lv_font_t *font, const lv_font_t *icon_font, int padding) {
  lv_point_t measured;
  lv_text_get_size(&measured, "-188\u00B0", font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  int width = std::max(static_cast<int>(measured.x), static_cast<int>(lv_font_get_line_height(icon_font)));
  static const char *const labels[] = {"Today", "Sun", "Mon", "Tues", "Weds", "Thurs", "Fri", "Sat"};
  for (const char *key : labels) {
    const auto text = espcontrol_i18n(std::string(key));
    lv_text_get_size(&measured, text.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    width = std::max(width, static_cast<int>(measured.x));
  }
  return width + padding;
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
  const int day_gap = std::max(8, font_height);
  const int icon_height = lv_font_get_line_height(card->icon_font);
  const int minimum_day_width = weather_daily_minimum_width(card->font, card->icon_font, 24);
  const size_t count = espcontrol::weather::visible_days(width, minimum_day_width,
    card->view == WeatherForecastView::DAILY ? forecast.count : 1, day_gap);
  const int caption_height = card->source->stale ? font_height + 2 : 0;
  const int high_y = std::max(icon_height + font_height + 4, height - caption_height - font_height);
  const int day_y = high_y - font_height - 2;
  // Rebuild only when geometry or the number of visible days changes. Refreshes
  // and unit changes reuse the labels instead of allocating new widget trees.
  if (card->width != width || card->height != height || card->columns.size() != count) {
    lv_obj_clean(card->root); card->columns.clear();
    card->width = width; card->height = height;
    lv_obj_set_size(card->root, width, height);
    lv_obj_align(card->root, LV_ALIGN_TOP_LEFT, 0, 0);
    const int available = width - day_gap * static_cast<int>(count - 1);
    const int column_width = available / static_cast<int>(count);
    const int remainder = available % static_cast<int>(count);
    for (size_t i = 0; i < count; ++i) {
      WeatherDailyColumn col{};
      auto make = [&](const lv_font_t *font, int y) {
        auto *label = weather_daily_label(card->root, font);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
        // Share leftover pixels between the first columns so the last one reaches the edge.
        const int index = static_cast<int>(i);
        const int x = index * (column_width + day_gap) + std::min(index, remainder);
        lv_obj_set_width(label, column_width + (index < remainder ? 1 : 0));
        lv_obj_set_pos(label, x, y); return label;
      };
      col.icon = make(card->icon_font, 0);
      col.day = make(card->font, day_y);
      col.high = make(card->font, high_y);
      card->columns.push_back(col);
    }
  }
  std::string caption;
  if (card->source->stale) caption = espcontrol_i18n(std::string("Last forecast"));
  lv_label_set_display_text(card->caption, caption.c_str());
  for (size_t i = 0; i < card->columns.size(); ++i) {
    const auto &col = card->columns[i];
    const auto &day = forecast.days[first + i];
    // Reserve the bottom status row only for stale data, without rebuilding labels.
    lv_obj_set_y(col.day, day_y);
    lv_obj_set_y(col.high, high_y);
    std::string date = card->view == WeatherForecastView::TOMORROW
      ? espcontrol_i18n(std::string("Tomorrow")) : weather_daily_date_label(day, forecast);
    lv_label_set_display_text(col.day, date.c_str());
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    std::string high = weather_daily_temperature(day.high, forecast.temperature_unit) + "\u00B0";
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
  weather_daily_render_card(weather_daily_card_for(static_cast<lv_obj_t *>(lv_event_get_target(event))));
}
inline void weather_daily_cleanup(lv_obj_t *owner) {
  if (auto *card = weather_daily_card_for(owner)) lv_obj_del(card->root);
}
inline void weather_daily_setup(BtnSlot &slot, const ParsedCfg &config,
                                const DisplayProfile &display) {
  if (config.entity.empty() || weather_daily_cards().size() >= 16) {
    lv_label_set_display_text(slot.text_lbl, espcontrol_i18n(std::string("Forecast unavailable")).c_str());
    lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE); return;
  }
  auto *source = weather_daily_acquire_source(config.entity);
  if (!source) {
    lv_label_set_display_text(slot.text_lbl, espcontrol_i18n(std::string("Forecast unavailable")).c_str());
    lv_obj_clear_flag(slot.btn, LV_OBJ_FLAG_CLICKABLE);
    ESP_LOGW("weather_forecast", "Daily forecast source limit reached (8)"); return;
  }
  auto *card = new (std::nothrow) WeatherDailyCard;
  if (!card) { weather_daily_release_source(source); return; }
  card->owner = slot.btn; card->source = source; card->caption = slot.text_lbl;
  card->view = config.precision == "today" ? WeatherForecastView::TODAY
    : config.precision == "tomorrow" || config.type == "weather_forecast"
      ? WeatherForecastView::TOMORROW : WeatherForecastView::DAILY;
  card->placeholder_icon = slot.icon_lbl;
  card->font = lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  if (!card->font) card->font = LV_FONT_DEFAULT;
  card->icon_font = display_icon_font(display);
  if (!card->icon_font) card->icon_font = card->font;
  card->summary_font = display_media_title_font(display);
  if (!card->summary_font) card->summary_font = card->font;
  card->detail_font = display_volume_label_font(display, card->font);
  card->root = lv_obj_create(slot.btn);
  if (!card->root) { weather_daily_release_source(source); delete card; return; }
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
  lv_obj_add_event_cb(slot.btn, weather_daily_card_resized, LV_EVENT_SIZE_CHANGED, nullptr);
  weather_daily_render_card(card);
}
