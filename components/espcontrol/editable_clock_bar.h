#pragma once

#include "clock_bar.h"
#include "clock_bar_layout.h"
#include "clock_bar_configuration.h"

// Included after button_grid.h: uses its shared HA binding and numeric parsing helpers.
namespace espcontrol::clockbar {
constexpr uint32_t SUBSCRIPTION_SCOPE = 1u << 4;
struct Widget { Element config; lv_obj_t *box = nullptr; lv_obj_t *label = nullptr; float value = NAN; };
struct Runtime {
  std::string serialized;
  std::vector<Widget> widgets;
  lv_obj_t *title = nullptr;
  lv_obj_t *temperature = nullptr, *clock = nullptr, *network = nullptr, *night = nullptr, *settings = nullptr, *battery = nullptr;
  int height = 40;
  bool active = false, night_active = false;
  bool rebuilding = false, raising_settings = false;
  lv_obj_t *settings_layer = nullptr;
  uint32_t generation = 0;
};
inline Runtime &runtime() { static Runtime value; return value; }
inline void hide() {
  auto &state = runtime();
  for (auto &widget : state.widgets) clock_bar_set_widget_hidden(widget.box, true);
  clock_bar_set_widget_hidden(state.title, true);
}
// One coordinator owns all editable settings shortcuts. Moving a child emits another
// CHILD_CHANGED event, so suppress re-entry while restoring their shared foreground order.
inline void raise_settings() {
  auto &state = runtime();
  if (!state.active || state.rebuilding || state.raising_settings) return;
  state.raising_settings = true;
  for (auto &widget : state.widgets) {
    if (widget.config.type == "network") clock_bar_raise_settings_button(widget.box);
  }
  state.raising_settings = false;
}
inline void register_settings_access(lv_obj_t *layer) {
  auto &state = runtime();
  if (!layer || state.settings_layer == layer) return;
  state.settings_layer = layer;
  lv_obj_add_event_cb(layer, [](lv_event_t *event) {
    if (lv_event_get_target(event) == runtime().settings_layer) raise_settings();
  }, LV_EVENT_CHILD_CHANGED, nullptr);
}
inline void bind() {
  auto &state = runtime();
  ha_reset_subscription_callbacks(SUBSCRIPTION_SCOPE);
  state.generation = ha_subscription_generation();
  bool subscribed = false;
  for (size_t index = 0; index < state.widgets.size(); index++) {
    const auto &item = state.widgets[index].config;
    if (!sensor_type(item.type) || item.source != "ha" || item.entity.empty()) continue;
    subscribed = ha_subscribe_state(item.entity, [index](esphome::StringRef value) {
      auto &current = runtime();
      if (index >= current.widgets.size()) return;
      float number = NAN;
      if (!parse_float_ref(value, number) || !std::isfinite(number)) number = NAN;
      current.widgets[index].value = number;
    }, SUBSCRIPTION_SCOPE, true) || subscribed;
  }
  if (subscribed) ha_reannounce_state_subscriptions();
}
inline void rebuild(const std::string &serialized, const std::vector<Element> &items) {
  auto &state = runtime();
  state.rebuilding = true;
  ha_reset_subscription_callbacks(SUBSCRIPTION_SCOPE);
  for (auto &widget : state.widgets) if (widget.box) lv_obj_del(widget.box);
  state.widgets.clear();
  state.serialized = serialized;
  state.active = !serialized.empty();
  if (!state.active) { hide(); state.rebuilding = false; return; }
  lv_obj_t *layer = lv_obj_get_parent(state.settings);
  if (!state.title) {
    state.title = lv_label_create(layer);
    lv_obj_clear_flag(state.title, LV_OBJ_FLAG_CLICKABLE);
    lv_label_set_long_mode(state.title, LV_LABEL_LONG_DOT);
  }
  state.widgets.reserve(items.size());
  for (const auto &item : items) {
    Widget widget; widget.config = item;
    widget.box = lv_obj_create(layer);
    lv_obj_remove_style_all(widget.box);
    lv_obj_set_style_bg_opa(widget.box, LV_OPA_TRANSP, 0);
    lv_obj_clear_flag(widget.box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(widget.box, LV_OBJ_FLAG_CLICKABLE);
    widget.label = lv_label_create(widget.box);
    lv_label_set_long_mode(widget.label, LV_LABEL_LONG_DOT);
    lv_obj_clear_flag(widget.label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_width(widget.label, lv_pct(100));
    lv_obj_align(widget.label, LV_ALIGN_CENTER, 0, 0);
    if (item.type == "network") {
      lv_obj_add_flag(widget.box, LV_OBJ_FLAG_CLICKABLE);
      lv_obj_add_event_cb(widget.box, [](lv_event_t *) {
        // Preserve the established Settings navigation, including modal dismissal.
        lv_obj_send_event(runtime().settings, LV_EVENT_PRESSED, nullptr);
      }, LV_EVENT_PRESSED, nullptr);
    }
    clock_bar_set_widget_hidden(widget.box, true);
    state.widgets.push_back(std::move(widget));
  }
  state.rebuilding = false;
  bind();
}
inline int text_width(lv_obj_t *label, const std::string &text) {
  lv_point_t size;
  lv_text_get_size(&size, text.c_str(), lv_obj_get_style_text_font(label, LV_PART_MAIN),
                  lv_obj_get_style_text_letter_space(label, LV_PART_MAIN), 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  return size.x;
}
inline void refresh(bool visible) {
  auto &state = runtime();
  if (!state.active) return;
  // The existing widgets remain data/theme sources; only the configurable widgets are shown.
  for (lv_obj_t *legacy : {state.temperature, state.clock, state.settings, state.night ? lv_obj_get_parent(state.night) : nullptr, state.battery ? lv_obj_get_parent(state.battery) : nullptr})
    clock_bar_set_widget_hidden(legacy, true);
  if (!visible) { hide(); return; }
  if (state.generation != ha_subscription_generation()) bind();
  const int margin = 6;
  const int width = std::max(0, static_cast<int>(clock_bar_current_screen_width(480)) - 2 * margin);
  const int gap = std::max(4, width * 6 / 1000);
  const int max_item_width = width * 35 / 100;
  std::vector<MeasuredItem> measured;
  for (size_t index = 0; index < state.widgets.size(); index++) {
    auto &widget = state.widgets[index];
    const auto &item = widget.config;
    lv_obj_t *prototype = state.clock;
    std::string text;
    bool icon = false;
    if (sensor_type(item.type)) {
      if (item.source == "local") {
        widget.value = NAN;
#ifdef USE_SENSOR
        for (auto *sensor : esphome::App.get_sensors()) {
          char object_id[128];
          if (std::string(sensor->get_object_id_to(object_id).c_str()) == item.entity) { widget.value = sensor->state; break; }
        }
#endif
      }
      char number[32];
      if (std::isfinite(widget.value)) snprintf(number, sizeof(number), "%.*f", item.precision, widget.value);
      else snprintf(number, sizeof(number), "--");
      text = (item.label.empty() ? "" : item.label + " ") + number + item.unit;
    } else if (item.type == "time") text = lv_label_get_text(state.clock);
    else if (item.type == "network") { prototype = state.network; text = lv_label_get_text(prototype); icon = true; }
    else if (item.type == "night") {
      if (!state.night_active) continue;
      prototype = state.night; text = lv_label_get_text(prototype); icon = true;
    } else if (item.type == "device_battery") {
      if (!state.battery) continue;
      prototype = state.battery; text = lv_label_get_text(prototype); icon = true;
    }
    const lv_font_t *font = lv_obj_get_style_text_font(prototype, LV_PART_MAIN);
    lv_obj_set_style_text_font(widget.label, font, LV_PART_MAIN);
    // DOT mode needs a bounded height; content height would wrap long readings into the card area.
    lv_obj_set_height(widget.label, std::min(state.height, static_cast<int>(lv_font_get_line_height(font))));
    lv_obj_set_style_text_color(widget.label, lv_obj_get_style_text_color(prototype, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_text_align(widget.label, icon ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_label_set_display_text(widget.label, text.c_str());
    const int desired = std::min(max_item_width, std::max(icon ? 44 : 24, text_width(widget.label, text) + (icon ? 12 : 2)));
    measured.push_back({index, item.group, desired});
  }
  const auto &title = clock_bar_left_title();
  const lv_font_t *title_font = lv_obj_get_style_text_font(state.clock, LV_PART_MAIN);
  const int title_height = std::min(state.height, static_cast<int>(lv_font_get_line_height(title_font)));
  lv_obj_set_style_text_font(state.title, title_font, LV_PART_MAIN);
  lv_obj_set_height(state.title, title_height);
  lv_obj_set_style_text_color(state.title, lv_obj_get_style_text_color(state.clock, LV_PART_MAIN), LV_PART_MAIN);
  lv_label_set_display_text(state.title, title.c_str());
  const auto layout = place(measured, width, gap, title.empty() ? 0 : std::min(width, text_width(state.title, title)));
  clock_bar_set_widget_hidden(state.title, layout.title_width == 0);
  for (size_t index = 0; index < state.widgets.size(); index++) {
    const bool shown = std::any_of(layout.items.begin(), layout.items.end(), [index](const Placement &item) { return item.index == index; });
    clock_bar_set_widget_hidden(state.widgets[index].box, !shown);
  }
  if (layout.title_width > 0) {
    lv_obj_set_width(state.title, layout.title_width);
    lv_obj_align(state.title, LV_ALIGN_TOP_LEFT, margin, (state.height - title_height) / 2);
    clock_bar_set_widget_hidden(state.title, false);
  }
  for (const auto &placement : layout.items) {
    auto &widget = state.widgets[placement.index];
    lv_obj_set_size(widget.box, placement.width, state.height);
    lv_obj_align(widget.box, LV_ALIGN_TOP_LEFT, margin + placement.x, 0);
    clock_bar_set_widget_hidden(widget.box, false);
  }
  raise_settings();
}
inline bool apply(const std::string &serialized, bool visible, int height, lv_obj_t *temperature, lv_obj_t *clock,
                  lv_obj_t *network, lv_obj_t *night, lv_obj_t *settings, bool night_active, lv_obj_t *battery = nullptr) {
  auto &state = runtime();
  state.temperature = temperature; state.clock = clock; state.network = network; state.night = night;
  state.settings = settings; state.height = height; state.night_active = night_active; state.battery = battery;
  if (!serialized.empty()) register_settings_access(lv_obj_get_parent(settings));
  if (state.serialized != serialized) {
    std::vector<Element> items;
    if (!parse(serialized, items)) return state.active;
    rebuild(serialized, items);
  }
  refresh(visible);
  return state.active;
}
}  // namespace espcontrol::clockbar
