#pragma once

// Forecast widgets exist only on configured daily cards. The modal allocates
// one day's detail view on demand and reads the same source as its owner.
struct WeatherDailyColumn { lv_obj_t *day, *icon, *value, *rain; };
struct WeatherDailyCard {
  lv_obj_t *owner = nullptr, *root = nullptr, *caption = nullptr;
  WeatherDailySource *source = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr;
  std::vector<WeatherDailyColumn> columns;
  int width = 0, height = 0;
  std::string label;
};
inline std::vector<WeatherDailyCard *> &weather_daily_cards() {
  static std::vector<WeatherDailyCard *> cards;
  return cards;
}
struct WeatherDailyModal {
  lv_obj_t *overlay = nullptr, *owner = nullptr, *date = nullptr, *icon = nullptr, *details = nullptr;
  lv_obj_t *previous = nullptr, *next = nullptr;
  WeatherDailySource *source = nullptr;
  size_t day = 0;
};
inline WeatherDailyModal &weather_daily_modal() { static WeatherDailyModal ui; return ui; }
inline void weather_daily_hide_modal() {
  auto &ui = weather_daily_modal();
  auto *source = ui.source;
  control_modal_delete_overlay(ControlModalKind::WEATHER_FORECAST, ui.overlay);
  ui = WeatherDailyModal{};
  weather_daily_release_source(source);
}
inline std::string weather_daily_temperature(float value, const char *unit) {
  if (!std::isfinite(value)) return "--";
  char text[16];
  std::snprintf(text, sizeof(text), "%d", static_cast<int>(std::round(convert_temperature_value_for_display_float(value, unit))));
  return text;
}
inline std::string weather_daily_date_label(const espcontrol::weather::Day &day,
                                           const espcontrol::weather::Forecast &forecast) {
  if (std::strcmp(day.date, forecast.today) == 0) return espcontrol_i18n(std::string("Today"));
  // Dates remain explicit even when the provider starts tomorrow or skips days.
  char text[12]; std::snprintf(text, sizeof(text), "%d/%d", std::atoi(day.date + 8), std::atoi(day.date + 5));
  return text;
}
inline std::string weather_daily_reading(float value, const char *unit) {
  if (!std::isfinite(value)) return "--";
  char text[32]; std::snprintf(text, sizeof(text), "%.1f%s", value, unit); return text;
}
inline void weather_daily_refresh_modal() {
  auto &ui = weather_daily_modal();
  if (!ui.overlay || !ui.source) return;
  const auto &forecast = ui.source->forecast;
  if (!forecast.count) {
    lv_label_set_display_text(ui.date, espcontrol_i18n(std::string("Forecast unavailable")).c_str());
    lv_label_set_display_text(ui.icon, weather_icon_for_state("unavailable"));
    lv_label_set_display_text(ui.details, "--/--");
    lv_obj_add_state(ui.previous, LV_STATE_DISABLED); lv_obj_add_state(ui.next, LV_STATE_DISABLED);
    return;
  }
  ui.day = std::min(ui.day, forecast.count - 1);
  const auto &day = forecast.days[ui.day];
  std::string title = std::string(day.date);
  lv_label_set_display_text(ui.date, title.c_str());
  lv_label_set_display_text(ui.icon, weather_icon_for_state(day.condition));
  std::string details = weather_label_for_state(day.condition) + "\n" +
    weather_daily_temperature(day.high, forecast.temperature_unit) + "/" +
    weather_daily_temperature(day.low, forecast.temperature_unit) + display_temperature_unit_symbol();
  if (std::isfinite(day.rain_probability)) details += "\n" + espcontrol_i18n(std::string("Rain chance")) + ": " + weather_daily_reading(day.rain_probability, "%");
  if (std::isfinite(day.precipitation)) details += "\n" + espcontrol_i18n(std::string("Rainfall")) + ": " + weather_daily_reading(day.precipitation, forecast.precipitation_unit);
  if (std::isfinite(day.wind)) details += "\n" + espcontrol_i18n(std::string("Wind")) + ": " + weather_daily_reading(day.wind, forecast.wind_unit);
  if (std::isfinite(day.humidity)) details += "\n" + espcontrol_i18n(std::string("Humidity")) + ": " + weather_daily_reading(day.humidity, "%");
  if (ui.source->stale) details += "\n" + espcontrol_i18n(std::string("Last forecast"));
  lv_label_set_display_text(ui.details, details.c_str());
  if (ui.day == 0) lv_obj_add_state(ui.previous, LV_STATE_DISABLED); else lv_obj_clear_state(ui.previous, LV_STATE_DISABLED);
  if (ui.day + 1 >= forecast.count) lv_obj_add_state(ui.next, LV_STATE_DISABLED); else lv_obj_clear_state(ui.next, LV_STATE_DISABLED);
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
inline void weather_daily_open_modal(WeatherDailyCard *card) {
  if (!card || !card->source || !card->owner) return;
  auto shell = control_modal_open_shell(ControlModalKind::WEATHER_FORECAST,
    card->owner, 100, card->icon_font, weather_daily_hide_modal);
  if (!shell.overlay || !shell.panel) return;
  auto &ui = weather_daily_modal();
  ui.overlay = shell.overlay; ui.owner = card->owner; ui.source = card->source;
  ++ui.source->users;
  set_clock_bar_modal_label(espcontrol_i18n(std::string("Weather")));
  // A scrollable detail area keeps long translations and small/rotated panels usable.
  const int top = shell.layout.back_size + shell.layout.inset * 2;
  auto *list = control_modal_create_scroll_list(shell.panel, shell.content_w,
    std::max(60, static_cast<int>(shell.layout.panel_h) - top - 60), 8);
  lv_obj_align(list, LV_ALIGN_TOP_MID, 0, top);
  ui.date = weather_daily_label(list, card->font); lv_obj_set_width(ui.date, lv_pct(100));
  ui.icon = weather_daily_label(list, card->icon_font); lv_obj_set_width(ui.icon, lv_pct(100));
  ui.details = weather_daily_label(list, card->font); lv_obj_set_width(ui.details, lv_pct(100));
  lv_label_set_long_mode(ui.details, LV_LABEL_LONG_WRAP);
  const int button_size = std::max(40, static_cast<int>(shell.layout.back_size));
  auto create_nav = [&](bool next) {
    auto *button = lv_btn_create(shell.panel);
    lv_obj_set_size(button, button_size, button_size);
    lv_obj_align(button, next ? LV_ALIGN_BOTTOM_RIGHT : LV_ALIGN_BOTTOM_LEFT,
      next ? -shell.layout.inset : shell.layout.inset, -shell.layout.inset);
    lv_obj_set_style_bg_color(button, lv_color_hex(current_theme().surface_secondary), LV_PART_MAIN);
    control_modal_apply_pressed_fill(button);
    auto *label = weather_daily_label(button, card->font);
    lv_label_set_text(label, next ? ">" : "<"); lv_obj_center(label);
    lv_obj_add_event_cb(button, [](lv_event_t *event) {
      auto &modal = weather_daily_modal();
      bool next = lv_event_get_user_data(event) != nullptr;
      if (!modal.source) return;
      if (next && modal.day + 1 < modal.source->forecast.count) ++modal.day;
      else if (!next && modal.day > 0) --modal.day;
      weather_daily_refresh_modal();
    }, LV_EVENT_CLICKED, next ? reinterpret_cast<void *>(1) : nullptr);
    return button;
  };
  ui.previous = create_nav(false); ui.next = create_nav(true);
  weather_daily_refresh_modal();
}
inline void weather_daily_render_card(WeatherDailyCard *card) {
  if (!card || !card->source || !card->root) return;
  auto &forecast = card->source->forecast;
  const int width = lv_obj_get_content_width(card->owner);
  const int height = lv_obj_get_content_height(card->owner);
  if (width <= 0 || height <= 0) return;
  const int font_height = lv_font_get_line_height(card->font);
  lv_point_t measured;
  lv_text_get_size(&measured, "-12/-12", card->font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  const size_t count = espcontrol::weather::visible_days(width, measured.x + 16,
    forecast.count ? forecast.count : 1);
  // Rebuild only when geometry or the number of visible days changes. Refreshes
  // and unit changes reuse the labels instead of allocating new widget trees.
  if (card->width != width || card->height != height || card->columns.size() != count) {
    lv_obj_clean(card->root); card->columns.clear();
    card->width = width; card->height = height;
    lv_obj_set_size(card->root, width, std::max(1, height - font_height - 4));
    lv_obj_align(card->root, LV_ALIGN_TOP_LEFT, 0, 0);
    const int column_width = width / count;
    const int icon_height = lv_font_get_line_height(card->icon_font);
    const int content_height = std::max(1, height - font_height - 4);
    const bool show_rain = content_height >= font_height * 3 + icon_height + 8;
    for (size_t i = 0; i < count; ++i) {
      WeatherDailyColumn col{};
      auto make = [&](const lv_font_t *font, int y) {
        auto *label = weather_daily_label(card->root, font);
        lv_obj_set_width(label, column_width); lv_obj_set_pos(label, i * column_width, y); return label;
      };
      col.day = make(card->font, 0);
      // Compact height uses a text-only strip; no icon is clipped into the title.
      if (content_height >= font_height * 2 + icon_height) col.icon = make(card->icon_font, font_height + 2);
      col.value = make(card->font, col.icon ? font_height + icon_height + 4 : font_height + 2);
      if (show_rain) col.rain = make(card->font, font_height * 2 + icon_height + 6);
      card->columns.push_back(col);
    }
  }
  std::string caption = (card->label.empty() ? "" : card->label + " · ") + display_temperature_unit_symbol();
  if (!forecast.count) caption = espcontrol_i18n(std::string("Forecast unavailable"));
  else if (card->source->stale) caption += " · " + espcontrol_i18n(std::string("Last forecast"));
  lv_label_set_display_text(card->caption, caption.c_str());
  for (size_t i = 0; i < card->columns.size(); ++i) {
    const auto &col = card->columns[i];
    const auto &day = forecast.days[i];
    std::string date = forecast.count ? weather_daily_date_label(day, forecast) : "--";
    lv_label_set_display_text(col.day, date.c_str());
    if (col.icon) lv_label_set_display_text(col.icon, weather_icon_for_state(forecast.count ? day.condition : "unavailable"));
    std::string temperature = weather_daily_temperature(day.high, forecast.temperature_unit) + "/" + weather_daily_temperature(day.low, forecast.temperature_unit);
    lv_label_set_display_text(col.value, temperature.c_str());
    if (col.rain) {
      std::string rain = std::isfinite(day.rain_probability) ? weather_daily_reading(day.rain_probability, "%") : "";
      lv_label_set_display_text(col.rain, rain.c_str());
    }
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
  card->owner = slot.btn; card->source = source; card->caption = slot.text_lbl; card->label = config.label;
  card->font = lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN);
  if (!card->font) card->font = LV_FONT_DEFAULT;
  card->icon_font = display_icon_font(display);
  if (!card->icon_font) card->icon_font = card->font;
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
