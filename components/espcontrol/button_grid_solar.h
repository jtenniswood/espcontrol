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
      std::string batt_text = ctx->battery.value + "%";
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

inline void solar_subscribe_field(SolarField &field, lv_obj_t *btn, SolarCardCtx *ctx) {
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

inline SolarCardCtx *create_solar_card_context(
    BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    uint32_t off_color,
    const lv_font_t *value_font,
    const lv_font_t *label_font,
    const lv_font_t *icon_font) {

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
  solar_subscribe_field(ctx->production,  s.btn, ctx);
  solar_subscribe_field(ctx->consumption, s.btn, ctx);
  solar_subscribe_field(ctx->net,         s.btn, ctx);
  solar_subscribe_field(ctx->battery,     s.btn, ctx);
  solar_subscribe_field(ctx->from_grid,   s.btn, ctx);
  solar_subscribe_field(ctx->to_grid,     s.btn, ctx);

  solar_apply_card_face(ctx);
  return ctx;
}
