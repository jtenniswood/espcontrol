#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Solar card: context struct, Net-cascade hero helper, and card-face render.
// No modal and no grid wiring (those are Tasks 7 and 8).

constexpr uint32_t SOLAR_CTX_MAGIC = 0x504C5253;  // "PLRS"

constexpr size_t SOLAR_FIELD_VALUE_MAX_LEN = 32;
constexpr size_t SOLAR_FIELD_UNIT_MAX_LEN = 16;

struct SolarField {
  std::string entity_id;
  std::string value;    // raw state string from HA
  std::string unit;     // unit_of_measurement
  bool available = false;
};

struct SolarCardCtx {
  uint32_t magic = SOLAR_CTX_MAGIC;
  lv_obj_t *btn = nullptr;
  lv_obj_t *value_lbl = nullptr;   // sensor_lbl: hero value
  lv_obj_t *unit_lbl = nullptr;    // hero unit
  lv_obj_t *label_lbl = nullptr;   // bottom label: hero type name
  lv_obj_t *icon_lbl = nullptr;    // solar glyph top-left
  lv_obj_t *corner_lbl = nullptr;  // battery % top-right
  const lv_font_t *value_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  int width_compensation_percent = 100;
  std::string mode;  // "live" or "today"
  SolarField production, consumption, net, battery, from_grid, to_grid;
  bool available = false;
};

inline bool solar_ctx_valid(SolarCardCtx *c) {
  return c && c->magic == SOLAR_CTX_MAGIC;
}

// ---------------------------------------------------------------------------
// Net-cascade hero computation
// ---------------------------------------------------------------------------

struct SolarHero {
  std::string text;
  std::string unit;
  std::string label;
  bool has = false;
  int sign = 1;   // 1 = green (surplus), -1 = red (importing)
};

inline SolarHero solar_compute_hero(const SolarCardCtx *c) {
  SolarHero h;

  // 1. Net entity has its own entity — use it directly
  if (c->net.available && !c->net.value.empty()) {
    h.text = c->net.value;
    h.unit = c->net.unit;
    h.label = "Net";
    h.sign = (c->net.value.find('-') == std::string::npos) ? 1 : -1;
    h.has = true;
    return h;
  }

  // 2. production − consumption (only when both available AND same unit)
  char *ep = nullptr;
  char *ec = nullptr;
  double p = std::strtod(c->production.value.c_str(), &ep);
  double cons = std::strtod(c->consumption.value.c_str(), &ec);
  if (c->production.available && c->consumption.available &&
      ep != c->production.value.c_str() && ec != c->consumption.value.c_str() &&
      c->production.unit == c->consumption.unit) {
    double d = p - cons;
    char buf[24];
    std::snprintf(buf, sizeof(buf), d >= 0 ? "+%.1f" : "%.1f", d);
    h.text = buf;
    h.unit = c->production.unit;
    h.label = "Net";
    h.sign = d >= 0 ? 1 : -1;
    h.has = true;
    return h;
  }

  // 3. production alone
  if (c->production.available && !c->production.value.empty()) {
    h.text = c->production.value;
    h.unit = c->production.unit;
    h.label = "Production";
    h.sign = 1;
    h.has = true;
    return h;
  }

  // 4. first available field among the remaining
  for (const SolarField *f : {&c->consumption, &c->battery, &c->from_grid, &c->to_grid}) {
    if (f->available && !f->value.empty()) {
      h.text = f->value;
      h.unit = f->unit;
      h.label = "Solar";
      h.sign = 1;
      h.has = true;
      return h;
    }
  }

  return h;  // has=false → show placeholder
}

// ---------------------------------------------------------------------------
// Card-face render
// ---------------------------------------------------------------------------

inline void solar_apply_card_face(SolarCardCtx *ctx) {
  if (!ctx || !ctx->btn) return;

  // Reset background
  lv_style_selector_t sel =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT;
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(ctx->off_color), sel);
  lv_obj_set_style_bg_grad_dir(ctx->btn, LV_GRAD_DIR_NONE, sel);

  SolarHero hero = solar_compute_hero(ctx);

  uint32_t hero_color = (hero.sign >= 0) ? 0x5BD17A : 0xFF6B6B;

  // Hero value label
  if (ctx->value_lbl) {
    if (hero.has) {
      lv_label_set_text(ctx->value_lbl, hero.text.c_str());
      if (ctx->value_font)
        lv_obj_set_style_text_font(ctx->value_lbl, ctx->value_font, LV_PART_MAIN);
      lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(hero_color), LV_PART_MAIN);
      lv_obj_clear_flag(ctx->value_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_label_set_text(ctx->value_lbl, "--");
      lv_obj_set_style_text_color(ctx->value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    }
  }

  // Unit label
  if (ctx->unit_lbl)
    lv_label_set_text(ctx->unit_lbl, hero.has ? hero.unit.c_str() : "");

  // Bottom label: hero type name
  if (ctx->label_lbl) {
    lv_label_set_text(ctx->label_lbl,
      hero.has ? hero.label.c_str()
               : (ctx->mode == "today" ? "Solar Today" : "Solar"));
    lv_obj_set_style_text_color(ctx->label_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }

  // Solar glyph top-left
  if (ctx->icon_lbl) {
    lv_obj_clear_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ctx->icon_lbl, find_icon("Solar Power"));
    if (ctx->icon_font)
      lv_obj_set_style_text_font(ctx->icon_lbl, ctx->icon_font, LV_PART_MAIN);
    lv_obj_set_style_text_color(ctx->icon_lbl, lv_color_hex(ctx->accent_color), LV_PART_MAIN);
    lv_obj_align(ctx->icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
  }

  // Battery % corner (top-right)
  if (ctx->corner_lbl) {
    if (ctx->battery.available && !ctx->battery.value.empty()) {
      std::string batt_text = ctx->battery.value;
      if (!ctx->battery.unit.empty()) batt_text += ctx->battery.unit;
      lv_label_set_text(ctx->corner_lbl, batt_text.c_str());
      lv_obj_set_style_text_color(ctx->corner_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      lv_obj_align(ctx->corner_lbl, LV_ALIGN_TOP_RIGHT, 0, 0);
      lv_obj_clear_flag(ctx->corner_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(ctx->corner_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }
}

// ---------------------------------------------------------------------------
// Context creation and subscriptions
// ---------------------------------------------------------------------------

// subscribe_sensor_value() only updates LVGL labels directly and has no
// generic-callback overload.  For the solar card we use ha_subscribe_state()
// for the entity state and ha_subscribe_attribute() for unit_of_measurement,
// exactly as the todo card subscribes to state and friendly_name.

inline void solar_subscribe_field(SolarField &field, SolarCardCtx *ctx) {
  if (field.entity_id.empty()) return;

  // Subscribe to state
  ha_subscribe_state(
    field.entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx, &field](esphome::StringRef state) {
        bool unavailable = ha_state_unavailable_ref(state);
        field.available = !unavailable;
        field.value = unavailable ? "" : string_ref_limited(state, SOLAR_FIELD_VALUE_MAX_LEN);
        solar_apply_card_face(ctx);
      }
    )
  );

  // Subscribe to unit_of_measurement attribute
  ha_subscribe_attribute(
    field.entity_id,
    std::string("unit_of_measurement"),
    std::function<void(esphome::StringRef)>(
      [&field](esphome::StringRef unit) {
        field.unit = string_ref_limited(unit, SOLAR_FIELD_UNIT_MAX_LEN);
      }
    )
  );
}

// ---------------------------------------------------------------------------
// Breakdown-list modal
// ---------------------------------------------------------------------------

inline void solar_open_modal(SolarCardCtx *ctx) {
  if (!solar_ctx_valid(ctx)) return;

  // Delete any existing SOLAR modal first
  lv_obj_t *existing_overlay = nullptr;
  control_modal_delete_overlay(ControlModalKind::SOLAR, existing_overlay);

  // Open the shared shell (back-arrow button, top-left, closes on click)
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::SOLAR,
    ctx->btn,
    ctx->width_compensation_percent,
    ctx->icon_font,
    "\U000F0141",   // mdi:arrow-left, same chevron used by todo/climate
    false,          // button_top_right = false → top-left
    []() {          // close_callback: delete the active overlay
      ControlModalActive &a = control_modal_active();
      if (a.overlay) {
        lv_obj_t *o = a.overlay;
        a.overlay = nullptr;
        lv_obj_del(o);
      }
      control_modal_reset_active();
    }
  );
  if (!shell.panel) return;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;

  // --- Title ---
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  lv_coord_t title_y = layout.inset + layout.back_size / 2;

  std::string title = (ctx->mode == "today") ? "Solar Today" : "Solar Now";
  lv_obj_t *title_lbl = control_modal_create_title(
    shell.panel, title,
    content_w - layout.back_size - gap,
    ctx->label_font,
    ctx->width_compensation_percent);
  lv_obj_set_style_text_color(title_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_update_layout(title_lbl);
  lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, title_y - lv_obj_get_height(title_lbl) / 2);

  // --- Hero row ---
  SolarHero hero = solar_compute_hero(ctx);
  lv_coord_t hero_h = 0;
  lv_coord_t after_title_y = layout.inset + layout.back_size + gap;

  if (hero.has) {
    uint32_t hero_color = (hero.sign >= 0) ? 0x5BD17A : 0xFF6B6B;

    // Hero container row (transparent, sized to content)
    lv_obj_t *hero_row = lv_obj_create(shell.panel);
    lv_obj_set_width(hero_row, content_w);
    lv_obj_set_height(hero_row, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(hero_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(hero_row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(hero_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(hero_row, 0, LV_PART_MAIN);
    lv_obj_clear_flag(hero_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(hero_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_layout(hero_row, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(hero_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
    lv_obj_set_style_flex_main_place(hero_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(hero_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_pad_column(hero_row, 4, LV_PART_MAIN);

    lv_obj_t *hero_val = lv_label_create(hero_row);
    lv_label_set_text(hero_val, hero.text.c_str());
    lv_obj_set_style_text_color(hero_val, lv_color_hex(hero_color), LV_PART_MAIN);
    if (ctx->value_font) lv_obj_set_style_text_font(hero_val, ctx->value_font, LV_PART_MAIN);
    apply_width_compensation(hero_val, ctx->width_compensation_percent);

    if (!hero.unit.empty()) {
      lv_obj_t *hero_unit = lv_label_create(hero_row);
      lv_label_set_text(hero_unit, hero.unit.c_str());
      lv_obj_set_style_text_color(hero_unit, lv_color_hex(hero_color), LV_PART_MAIN);
      if (ctx->label_font) lv_obj_set_style_text_font(hero_unit, ctx->label_font, LV_PART_MAIN);
      apply_width_compensation(hero_unit, ctx->width_compensation_percent);
    }

    lv_obj_align(hero_row, LV_ALIGN_TOP_MID, 0, after_title_y);
    lv_obj_update_layout(hero_row);
    hero_h = lv_obj_get_height(hero_row);
  }

  // --- Scroll list of field rows ---
  lv_coord_t row_gap = control_modal_scaled_px(10, layout.short_side);
  if (row_gap < 6) row_gap = 6;
  lv_coord_t list_y = after_title_y + hero_h + (hero.has ? gap : 0);
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < 40) list_h = 40;

  lv_coord_t list_pad = control_modal_scaled_px(10, layout.short_side);
  if (list_pad < 4) list_pad = 4;
  lv_coord_t list_w = content_w - list_pad * 2;
  if (list_w < 80) { list_w = content_w; list_pad = 0; }

  lv_obj_t *list = control_modal_create_scroll_list(shell.panel, list_w, list_h, row_gap);
  lv_obj_align(list, LV_ALIGN_TOP_LEFT, layout.inset + list_pad, list_y);

  // Row height based on label font
  lv_coord_t row_h = ctx->label_font && ctx->label_font->line_height > 0
    ? ctx->label_font->line_height + control_modal_scaled_px(8, layout.short_side)
    : control_modal_scaled_px(28, layout.short_side);
  if (row_h < 24) row_h = 24;

  // Icon column width
  lv_coord_t icon_w = ctx->icon_font && ctx->icon_font->line_height > 0
    ? ctx->icon_font->line_height + 4
    : control_modal_scaled_px(22, layout.short_side);
  if (icon_w < 18) icon_w = 18;

  // Per-field row builder
  struct RowSpec { const char *icon_name; const char *row_label; const SolarField *field; };
  RowSpec rows[] = {
    { "Solar Power",         "Production",     &ctx->production  },
    { "Home Lightning Bolt", "Consumption",    &ctx->consumption },
    { "Solar Power",         "Net",            &ctx->net         },
    { "Arrow Up",            "To Grid",        &ctx->to_grid     },
    { "Arrow Down",          "From Grid",      &ctx->from_grid   },
    { "Battery",             "Battery",        &ctx->battery     },
  };

  for (auto &rs : rows) {
    if (rs.field->entity_id.empty()) continue;

    // Row container
    lv_obj_t *row = lv_obj_create(list);
    lv_obj_set_width(row, lv_pct(100));
    lv_obj_set_height(row, row_h);
    lv_obj_set_style_radius(row, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_CLICKABLE);

    // Icon
    lv_obj_t *icon = lv_label_create(row);
    lv_label_set_text(icon, find_icon(rs.icon_name));
    lv_obj_set_style_text_color(icon, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    if (ctx->icon_font) lv_obj_set_style_text_font(icon, ctx->icon_font, LV_PART_MAIN);
    lv_obj_set_width(icon, icon_w);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 0, 0);

    // Field label (e.g. "Production")
    lv_coord_t label_x = icon_w + 4;
    lv_coord_t value_w = list_w / 3;
    lv_coord_t label_w = list_w - label_x - value_w;
    if (label_w < 30) label_w = 30;

    lv_obj_t *name_lbl = lv_label_create(row);
    lv_label_set_text(name_lbl, rs.row_label);
    lv_label_set_long_mode(name_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name_lbl, label_w);
    lv_obj_set_style_text_color(name_lbl, lv_color_hex(DARK_TEXT_SOFT), LV_PART_MAIN);
    lv_obj_set_style_text_align(name_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(name_lbl, ctx->label_font, LV_PART_MAIN);
    apply_width_compensation(name_lbl, ctx->width_compensation_percent);
    lv_obj_align(name_lbl, LV_ALIGN_LEFT_MID, label_x, 0);

    // Value + unit (right-aligned)
    std::string val_str = rs.field->available ? rs.field->value : "--";
    std::string unit_str = (rs.field->available && !rs.field->unit.empty()) ? rs.field->unit : "";
    std::string val_with_unit = unit_str.empty() ? val_str : (val_str + " " + unit_str);

    lv_obj_t *val_lbl = lv_label_create(row);
    lv_label_set_text(val_lbl, val_with_unit.c_str());
    lv_label_set_long_mode(val_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(val_lbl, value_w);
    lv_obj_set_style_text_color(val_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(val_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(val_lbl, ctx->label_font, LV_PART_MAIN);
    apply_width_compensation(val_lbl, ctx->width_compensation_percent);
    lv_obj_align(val_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
  }

  lv_obj_move_foreground(shell.overlay);
}

inline SolarCardCtx *create_solar_card_context(
    BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    uint32_t off_color,
    const lv_font_t *value_font,
    const lv_font_t *label_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {

  SolarCardCtx *ctx = new SolarCardCtx();
  ctx->mode = cfg_option_value(p.options, "mode");
  if (ctx->mode.empty()) ctx->mode = "live";
  ctx->accent_color = accent_color;
  ctx->off_color = off_color;
  ctx->btn = s.btn;
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->icon_lbl = s.icon_lbl;
  ctx->value_font = value_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);

  // Create corner label for battery %
  ctx->corner_lbl = lv_label_create(s.btn);
  if (label_font) lv_obj_set_style_text_font(ctx->corner_lbl, label_font, LV_PART_MAIN);
  lv_label_set_text(ctx->corner_lbl, "");
  lv_obj_add_flag(ctx->corner_lbl, LV_OBJ_FLAG_HIDDEN);

  // Populate entity IDs from config options
  ctx->production.entity_id  = cfg_option_value(p.options, "production");
  ctx->consumption.entity_id = cfg_option_value(p.options, "consumption");
  ctx->net.entity_id         = cfg_option_value(p.options, "net");
  ctx->battery.entity_id     = cfg_option_value(p.options, "battery");
  ctx->from_grid.entity_id   = cfg_option_value(p.options, "from_grid");
  ctx->to_grid.entity_id     = cfg_option_value(p.options, "to_grid");

  // Subscribe to each configured field
  solar_subscribe_field(ctx->production,  ctx);
  solar_subscribe_field(ctx->consumption, ctx);
  solar_subscribe_field(ctx->net,         ctx);
  solar_subscribe_field(ctx->battery,     ctx);
  solar_subscribe_field(ctx->from_grid,   ctx);
  solar_subscribe_field(ctx->to_grid,     ctx);

  solar_apply_card_face(ctx);

  // Tap handler: open the breakdown-list modal
  lv_obj_add_event_cb(s.btn, [](lv_event_t *e) {
    SolarCardCtx *ctx = static_cast<SolarCardCtx *>(lv_event_get_user_data(e));
    solar_open_modal(ctx);
  }, LV_EVENT_CLICKED, ctx);

  return ctx;
}
