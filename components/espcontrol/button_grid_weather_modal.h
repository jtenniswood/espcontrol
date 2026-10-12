#pragma once

// Included by the weather widget layer after its shared formatting helpers.
// Allocate only the visible forecast row; release live metadata callbacks on close.
struct WeatherDailyModalColumn { lv_obj_t *button, *day, *icon, *high, *low; };
struct WeatherDailyModal {
  lv_obj_t *overlay = nullptr, *owner = nullptr, *condition = nullptr, *name = nullptr;
  lv_obj_t *icon = nullptr, *temperature = nullptr, *range = nullptr, *details = nullptr;
  lv_obj_t *days = nullptr, *navigation = nullptr, *previous = nullptr, *next = nullptr;
  WeatherDailySource *source = nullptr;
  const lv_font_t *font = nullptr, *icon_font = nullptr;
  std::vector<WeatherDailyModalColumn> columns;
  size_t day = 0, first = 0, capacity = 1;
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
  if (ui.first + column >= ui.source->forecast.count) return;
  ui.day = ui.first + column;
  weather_daily_refresh_modal();
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
    lv_obj_add_flag(ui.navigation, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_set_style_text_font(ui.condition, lv_obj_get_style_text_font(ui.temperature, LV_PART_MAIN), LV_PART_MAIN);
  lv_label_set_display_text(ui.condition, weather_label_for_state(current).c_str());
  lv_label_set_display_text(ui.icon, weather_icon_for_state(current));
  ui.day = std::min(ui.day, forecast.count - 1);
  ui.first = (ui.day / ui.capacity) * ui.capacity;
  const size_t count = std::min(ui.capacity, forecast.count - ui.first);
  lv_obj_clear_flag(ui.days, LV_OBJ_FLAG_HIDDEN);
  // Only visible day widgets exist. Page changes/updates reuse them when the count is unchanged.
  if (ui.columns.size() != count) {
    lv_obj_clean(ui.days); ui.columns.clear();
    const int gap = 12;
    const int available = ui.row_width - gap * static_cast<int>(count - 1);
    const int width = available / static_cast<int>(count);
    const int remainder = available % static_cast<int>(count);
    const int text_height = lv_font_get_line_height(ui.font);
    const int icon_y = text_height + 12;
    const int high_y = icon_y + lv_font_get_line_height(ui.icon_font) + 12;
    for (size_t i = 0; i < count; ++i) {
      const int index = static_cast<int>(i);
      const int column_width = width + (index < remainder ? 1 : 0);
      auto *button = weather_daily_modal_container(ui.days, column_width, ui.row_height);
      lv_obj_set_pos(button, index * (width + gap) + std::min(index, remainder), 0);
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
    const auto &day = forecast.days[ui.first + i];
    const auto &col = ui.columns[i];
    lv_label_set_display_text(col.day, weather_daily_date_label(day, forecast).c_str());
    lv_obj_set_style_text_opa(col.day, ui.first + i == ui.day ? LV_OPA_COVER : LV_OPA_70, LV_PART_MAIN);
    lv_label_set_display_text(col.icon, weather_icon_for_state(day.condition));
    const std::string high = weather_daily_temperature(day.high, forecast.temperature_unit) + "\u00B0";
    const std::string low = weather_daily_temperature(day.low, forecast.temperature_unit) + "\u00B0";
    lv_label_set_display_text(col.high, high.c_str()); lv_label_set_display_text(col.low, low.c_str());
  }
  if (forecast.count > ui.capacity) lv_obj_clear_flag(ui.navigation, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(ui.navigation, LV_OBJ_FLAG_HIDDEN);
  if (ui.first == 0) lv_obj_add_state(ui.previous, LV_STATE_DISABLED);
  else lv_obj_clear_state(ui.previous, LV_STATE_DISABLED);
  if (ui.first + count >= forecast.count) lv_obj_add_state(ui.next, LV_STATE_DISABLED);
  else lv_obj_clear_state(ui.next, LV_STATE_DISABLED);
  const auto &day = forecast.days[ui.day];
  std::string details;
  auto add = [&](float value, const char *key, const char *unit) {
    if (!std::isfinite(value)) return;
    if (details.empty()) details = weather_daily_date_label(day, forecast);
    details += "\n" + espcontrol_i18n(std::string(key)) + ": " + weather_daily_reading(value, unit);
  };
  add(day.rain_probability, "Rain chance", "%");
  add(day.precipitation, "Rainfall", forecast.precipitation_unit);
  add(day.wind, "Wind", forecast.wind_unit);
  add(day.humidity, "Humidity", "%");
  if (ui.source->stale) details += (details.empty() ? "" : "\n") + espcontrol_i18n(std::string("Last forecast"));
  lv_label_set_display_text(ui.details, details.c_str());
  if (details.empty()) lv_obj_add_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(ui.details, LV_OBJ_FLAG_HIDDEN);
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
  ++ui.source->users;
  set_clock_bar_modal_label(espcontrol_i18n(std::string("Weather")));
  const int top = shell.layout.back_size + shell.layout.inset * 2;
  auto *list = control_modal_create_scroll_list(shell.panel, shell.content_w,
    std::max(60, static_cast<int>(shell.layout.panel_h) - top - shell.layout.inset), 12);
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
  ui.capacity = espcontrol::weather::visible_days(ui.row_width, minimum, espcontrol::weather::MAX_DAYS, 12);
  ui.row_height = text_height * 3 + icon_height + 28;
  ui.days = weather_daily_modal_container(list, ui.row_width, ui.row_height);
  const int button_size = std::max(40, static_cast<int>(shell.layout.back_size));
  ui.navigation = weather_daily_modal_container(list, shell.content_w, button_size);
  auto create_nav = [&](bool next) {
    auto *button = lv_btn_create(ui.navigation);
    lv_obj_set_size(button, button_size, button_size);
    lv_obj_set_pos(button, next ? shell.content_w - button_size : 0, 0);
    lv_obj_set_style_bg_color(button, lv_color_hex(current_theme().surface_secondary), LV_PART_MAIN);
    control_modal_apply_pressed_fill(button);
    auto *label = weather_daily_label(button, card->font);
    lv_label_set_text(label, next ? ">" : "<"); lv_obj_center(label);
    lv_obj_add_event_cb(button, [](lv_event_t *event) {
      auto &modal = weather_daily_modal();
      if (!modal.source) return;
      const bool next = lv_event_get_user_data(event) != nullptr;
      const size_t count = modal.source->forecast.count;
      if (next && modal.first + modal.capacity < count) modal.day = modal.first + modal.capacity;
      else if (!next && modal.first > 0) modal.day = modal.first - modal.capacity;
      weather_daily_refresh_modal();
    }, LV_EVENT_CLICKED, next ? reinterpret_cast<void *>(1) : nullptr);
    return button;
  };
  ui.previous = create_nav(false); ui.next = create_nav(true);
  ui.details = weather_daily_label(list, ui.font); lv_obj_set_width(ui.details, shell.content_w);
  lv_obj_set_style_text_align(ui.details, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  lv_label_set_long_mode(ui.details, LV_LABEL_LONG_WRAP);
  weather_daily_refresh_modal();
  weather_daily_modal_bind_current_conditions(ui);
}
