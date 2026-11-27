#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Flights card ──────────────────────────────────────────────────────
// Shows aircraft near home from a Home Assistant template sensor built on the
// Flightradar24 integration (see docs/card-types/flights.md). The sensor state
// is the number of flights in the area; its `flights` attribute is the compact
// list parsed by flights_data.h. Tapping the card opens a list of the nearest
// flights.

#include "flights_data.h"

struct FlightsCardCtx;

struct FlightsModalRow {
  lv_obj_t *row = nullptr;
  lv_obj_t *title = nullptr;
  lv_obj_t *route = nullptr;
  lv_obj_t *details = nullptr;
};

struct FlightsModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *empty_lbl = nullptr;
  FlightsCardCtx *active = nullptr;
  std::vector<FlightsModalRow> rows;
  lv_coord_t row_radius = 0;
  lv_coord_t row_pad = 0;
};

inline FlightsModalUi &flights_modal_ui() {
  static FlightsModalUi ui;
  return ui;
}

struct FlightsCardCtx {
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  lv_obj_t *btn = nullptr;
  lv_obj_t *count_lbl = nullptr;
  lv_obj_t *name_lbl = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  bool imperial = false;
  bool available = false;
  std::string count;
  std::vector<espcontrol::flights::Flight> flights;
  void detach();
  ~FlightsCardCtx() { detach(); }
};

inline void flights_hide_modal() {
  FlightsModalUi &ui = flights_modal_ui();
  control_modal_delete_overlay(ControlModalKind::FLIGHTS, ui.overlay);
  ui = FlightsModalUi();
}

inline void FlightsCardCtx::detach() {
  ha_release_callbacks_for_owner(this);
  if (flights_modal_ui().active == this) flights_hide_modal();
  btn = nullptr;
  count_lbl = nullptr;
  name_lbl = nullptr;
}

inline std::string flights_card_title(const FlightsCardCtx *ctx) {
  if (ctx && !ctx->configured_label.empty()) return ctx->configured_label;
  if (ctx && !ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Flights"));
}

inline const char *flights_vertical_text(espcontrol::flights::Vertical vertical) {
  using espcontrol::flights::Vertical;
  switch (vertical) {
    case Vertical::ON_GROUND: return espcontrol_i18n("On the ground");
    case Vertical::CLIMBING: return espcontrol_i18n("Climbing");
    case Vertical::DESCENDING: return espcontrol_i18n("Descending");
    case Vertical::LEVEL: return espcontrol_i18n("Level");
    case Vertical::UNKNOWN: break;
  }
  return "";
}

inline std::string flights_row_title(const espcontrol::flights::Flight &flight) {
  std::string aircraft = flight.aircraft;
  if (aircraft.empty()) aircraft = espcontrol_i18n(std::string("Unknown aircraft"));
  if (flight.flight.empty()) return aircraft;
  return flight.flight + " \xC2\xB7 " + aircraft;
}

inline std::string flights_row_route(const espcontrol::flights::Flight &flight) {
  return espcontrol::flights::join(
    {flight.airline, espcontrol::flights::route(flight)}, " \xC2\xB7 ");
}

inline std::string flights_row_details(const espcontrol::flights::Flight &flight, bool imperial) {
  using namespace espcontrol::flights;
  return join({format_distance(flight.distance_km, imperial),
               flight.on_ground ? std::string() : format_altitude(flight.altitude_ft, imperial),
               format_speed(flight.speed_kt, imperial),
               flights_vertical_text(vertical_state(flight))}, " \xC2\xB7 ");
}

inline void flights_apply_card_text(FlightsCardCtx *ctx) {
  if (!ctx) return;
  if (ctx->count_lbl) {
    const auto &count = ctx->count;
    lv_label_set_display_text(
      ctx->count_lbl, ctx->available && !count.empty() ? count.c_str() : "--");
  }
  if (ctx->name_lbl) {
    lv_label_set_display_text(ctx->name_lbl, ctx->friendly_name.empty()
      ? espcontrol_i18n("Flights") : ctx->friendly_name.c_str());
  }
  if (ctx->btn) set_card_checked_state(ctx->btn, ctx->available && !ctx->flights.empty());
}

inline lv_obj_t *flights_modal_label(lv_obj_t *parent, const lv_font_t *font, uint32_t color) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
  lv_obj_set_width(label, lv_pct(100));
  lv_obj_set_style_text_color(label, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  apply_text_width_compensation(label);
  return label;
}

inline FlightsModalRow flights_modal_create_row(FlightsModalUi &ui, const lv_font_t *font) {
  FlightsModalRow row;
  row.row = lv_obj_create(ui.list);
  lv_obj_set_width(row.row, lv_pct(100));
  lv_obj_set_height(row.row, LV_SIZE_CONTENT);
  lv_obj_set_style_radius(row.row, ui.row_radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(
    row.row, lv_color_hex(theme_display_color(current_theme().surface_primary)), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(row.row, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(row.row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(row.row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(row.row, ui.row_pad, LV_PART_MAIN);
  lv_obj_set_style_pad_row(row.row, ui.row_pad / 3, LV_PART_MAIN);
  lv_obj_set_layout(row.row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(row.row, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_clear_flag(row.row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(row.row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(row.row, LV_OBJ_FLAG_SCROLL_CHAIN);
  theme_set_content_background(row.row, false);
  row.title = flights_modal_label(row.row, font, current_theme().text_primary);
  row.route = flights_modal_label(row.row, font, current_theme().text_muted);
  row.details = flights_modal_label(row.row, font, current_theme().text_muted);
  return row;
}

inline void flights_set_row_text(lv_obj_t *label, const std::string &text) {
  lv_label_set_display_text(label, text.c_str());
  if (text.empty()) lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_clear_flag(label, LV_OBJ_FLAG_HIDDEN);
}

inline void flights_refresh_modal(FlightsCardCtx *ctx) {
  FlightsModalUi &ui = flights_modal_ui();
  if (!ctx || ui.active != ctx || !ui.list) return;
  const size_t count = ctx->available ? ctx->flights.size() : 0;
  if (ui.rows.size() != count) {
    // Rows only change when flights enter or leave; keep the scroll position.
    const lv_coord_t scroll_y = lv_obj_get_scroll_y(ui.list);
    lv_obj_clean(ui.list);
    ui.rows.clear();
    ui.empty_lbl = nullptr;
    for (size_t i = 0; i < count; ++i)
      ui.rows.push_back(flights_modal_create_row(ui, ctx->label_font));
    if (count == 0) {
      ui.empty_lbl = flights_modal_label(ui.list, ctx->label_font, current_theme().text_muted);
      lv_label_set_long_mode(ui.empty_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_style_text_align(ui.empty_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    }
    lv_obj_update_layout(ui.list);
    lv_obj_scroll_to_y(ui.list, scroll_y, LV_ANIM_OFF);
  }
  if (ui.empty_lbl) {
    const char *key = ctx->available ? "No flights nearby" : "Flight data unavailable";
    lv_label_set_display_text(ui.empty_lbl, espcontrol_i18n(key));
  }
  for (size_t i = 0; i < count; ++i) {
    const auto &flight = ctx->flights[i];
    flights_set_row_text(ui.rows[i].title, flights_row_title(flight));
    flights_set_row_text(ui.rows[i].route, flights_row_route(flight));
    flights_set_row_text(ui.rows[i].details, flights_row_details(flight, ctx->imperial));
  }
}

inline void flights_open_modal(FlightsCardCtx *ctx) {
  if (!ctx || !ctx->btn) return;
  const int width_compensation = icon_width_compensation_percent();
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::FLIGHTS, ctx->btn, width_compensation,
    ctx->icon_font, flights_hide_modal);
  if (!shell.overlay || !shell.panel) return;
  set_clock_bar_modal_label(flights_card_title(ctx));
  FlightsModalUi &ui = flights_modal_ui();
  ui.overlay = shell.overlay;
  ui.active = ctx;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  ui.row_pad = gap;
  ui.row_radius = control_modal_scaled_px(16, layout.short_side);
  const lv_coord_t title_y = layout.inset + layout.back_size / 2;
  const lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 60) list_h = 60;

  lv_obj_t *title = control_modal_create_title(
    shell.panel, flights_card_title(ctx), shell.content_w - layout.back_size - gap,
    ctx->label_font, width_compensation);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, title_y - layout.back_size / 2);

  ui.list = control_modal_create_scroll_list(shell.panel, shell.content_w, list_h, gap);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset, list_y);
  flights_refresh_modal(ctx);
  lv_obj_move_foreground(ui.overlay);
}

inline void handle_flights_card_click(FlightsCardCtx *ctx) {
  flights_open_modal(ctx);
}

inline void subscribe_flights_card(FlightsCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  HaCallbackOwnerScope callback_owner(ctx);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      ctx->available = !ha_entity_state_unavailable_ref(ctx->entity_id, state);
      ctx->count = ctx->available ? string_ref_limited(state, 8) : "";
      flights_apply_card_text(ctx);
      flights_refresh_modal(ctx);
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("flights"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef list) {
      auto next = espcontrol::flights::parse_list(
        string_ref_limited(list, espcontrol::flights::MAX_LIST_TEXT_LEN));
      if (next == ctx->flights) return;
      ctx->flights = std::move(next);
      flights_apply_card_text(ctx);
      flights_refresh_modal(ctx);
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      flights_apply_card_text(ctx);
    })
  );
}
