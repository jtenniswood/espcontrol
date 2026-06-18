#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Fan card controls ─────────────────────────────────────────────────

constexpr int FAN_PRESET_MAX_OPTIONS = 32;

struct FanCardCtx {
  std::string type;
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string state;
  std::string direction;
  std::string preset_mode;
  std::vector<std::string> preset_modes;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  TransientStatusLabel *status_label = nullptr;
  const char *icon_off_glyph = nullptr;
  const char *icon_on_glyph = nullptr;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  bool available = false;
  bool on = false;
  bool oscillating = false;
  bool oscillation_known = false;
  bool direction_known = false;
  bool has_custom_label = false;
};

struct FanPresetClick {
  FanCardCtx *ctx = nullptr;
  std::string mode;
};

struct FanPresetUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *empty_lbl = nullptr;
  FanCardCtx *active = nullptr;
  FanPresetClick option_clicks[FAN_PRESET_MAX_OPTIONS];
};

inline FanPresetUi &fan_preset_ui() {
  static FanPresetUi ui;
  return ui;
}

inline bool fan_non_speed_card_type(const std::string &type) {
  return type == "fan_switch" ||
         type == "fan_oscillate" ||
         type == "fan_direction" ||
         type == "fan_preset";
}

inline const char *fan_card_icon_name(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return p.icon.c_str();
  return fan_card_default_icon_name(p.type);
}

inline const char *fan_card_icon_on_name(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return p.icon_on.c_str();
  return p.type == "fan_switch" ? "Fan" : fan_card_icon_name(p);
}

inline std::string fan_card_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label;
  if (p.type == "fan_switch") return espcontrol_i18n(std::string("Fan"));
  if (p.type == "fan_oscillate") return espcontrol_i18n(std::string("Oscillation"));
  if (p.type == "fan_direction") return espcontrol_i18n(std::string("Direction"));
  if (p.type == "fan_preset") return espcontrol_i18n(std::string("Preset"));
  return espcontrol_i18n(std::string("Fan"));
}

inline void setup_fan_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, find_icon(fan_card_icon_name(p)));
  lv_label_set_text(s.text_lbl, fan_card_label(p).c_str());
  if (p.type != "fan_switch") apply_push_button_transition(s.btn);
}

inline std::string fan_trim(const std::string &value) {
  size_t start = 0;
  while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) start++;
  size_t end = value.size();
  while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) end--;
  return value.substr(start, end - start);
}

inline std::string fan_lower(const std::string &value) {
  std::string out = value;
  for (char &c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

inline bool fan_parse_bool_value(const std::string &value, bool &out) {
  std::string lower = fan_lower(fan_trim(value));
  if (lower == "true" || lower == "on" || lower == "yes" || lower == "1") {
    out = true;
    return true;
  }
  if (lower == "false" || lower == "off" || lower == "no" || lower == "0") {
    out = false;
    return true;
  }
  return false;
}

inline bool fan_token_is_header(const std::string &value) {
  std::string lower = fan_lower(value);
  return lower == "presetmode" || lower == "presetmodes" ||
         lower == "direction" || lower == "oscillating";
}

inline std::vector<std::string> fan_parse_options(esphome::StringRef value) {
  std::string raw = string_ref_limited(value, HA_TEXT_SENSOR_STATE_MAX_LEN);
  std::vector<std::string> out;
  std::string cur;
  bool quoted = false;
  char quote_char = 0;
  for (char ch : raw) {
    if ((ch == '\'' || ch == '"')) {
      if (quoted && ch == quote_char) {
        quoted = false;
        quote_char = 0;
      } else if (!quoted) {
        quoted = true;
        quote_char = ch;
      } else {
        cur.push_back(ch);
      }
      continue;
    }
    if (!quoted && (ch == '[' || ch == ']')) continue;
    if (!quoted && ch == ',') {
      std::string item = fan_trim(cur);
      if (!item.empty() && !fan_token_is_header(item)) {
        std::string lower = fan_lower(item);
        bool duplicate = false;
        for (const auto &existing : out) {
          if (fan_lower(existing) == lower) {
            duplicate = true;
            break;
          }
        }
        if (!duplicate) out.push_back(item);
      }
      cur.clear();
      continue;
    }
    cur.push_back(ch);
  }
  std::string item = fan_trim(cur);
  if (!item.empty() && !fan_token_is_header(item)) {
    std::string lower = fan_lower(item);
    bool duplicate = false;
    for (const auto &existing : out) {
      if (fan_lower(existing) == lower) {
        duplicate = true;
        break;
      }
    }
    if (!duplicate) out.push_back(item);
  }
  return out;
}

inline std::string fan_option_label(const std::string &value) {
  if (value.empty()) return espcontrol_i18n(std::string("None"));
  return espcontrol_i18n(sentence_cap_text(value));
}

inline bool fan_preset_active(const std::string &value) {
  std::string lower = fan_lower(fan_trim(value));
  return !lower.empty() && lower != "none" && lower != "off";
}

inline bool fan_control_supported(FanCardCtx *ctx) {
  if (!ctx || !ctx->available) return false;
  if (ctx->type == "fan_oscillate") return ctx->oscillation_known;
  if (ctx->type == "fan_direction") return ctx->direction_known;
  if (ctx->type == "fan_preset") return !ctx->preset_modes.empty();
  return true;
}

inline void fan_apply_card_visual(FanCardCtx *ctx) {
  if (!ctx || !ctx->btn) return;
  bool supported = fan_control_supported(ctx);
  apply_control_availability(ctx->btn, ctx->btn, supported);

  bool active = false;
  if (ctx->type == "fan_switch") active = ctx->on;
  else if (ctx->type == "fan_oscillate") active = ctx->oscillating;
  else if (ctx->type == "fan_direction") active = ctx->direction == "reverse";
  else if (ctx->type == "fan_preset") active = fan_preset_active(ctx->preset_mode);

  set_card_checked_state(ctx->btn, active);

  if (ctx->icon_lbl) {
    if (ctx->type == "fan_switch" && active && ctx->icon_on_glyph) {
      lv_label_set_text(ctx->icon_lbl, ctx->icon_on_glyph);
    } else if (ctx->icon_off_glyph) {
      lv_label_set_text(ctx->icon_lbl, ctx->icon_off_glyph);
    }
  }
}

inline std::string fan_status_text(FanCardCtx *ctx) {
  if (!ctx || !ctx->available) return espcontrol_i18n(std::string("Unavailable"));
  if (ctx->type == "fan_switch") {
    return ctx->on ? espcontrol_i18n(std::string("On")) : espcontrol_i18n(std::string("Off"));
  }
  if (ctx->type == "fan_oscillate") {
    if (!ctx->oscillation_known) return espcontrol_i18n(std::string("Unsupported"));
    return ctx->oscillating ? espcontrol_i18n(std::string("Oscillating")) : espcontrol_i18n(std::string("Still"));
  }
  if (ctx->type == "fan_direction") {
    if (!ctx->direction_known) return espcontrol_i18n(std::string("Unsupported"));
    return fan_option_label(ctx->direction);
  }
  if (ctx->type == "fan_preset") {
    if (ctx->preset_modes.empty()) return espcontrol_i18n(std::string("Unsupported"));
    return fan_preset_active(ctx->preset_mode) ? fan_option_label(ctx->preset_mode) : espcontrol_i18n(std::string("Preset"));
  }
  return espcontrol_i18n(std::string("Fan"));
}

inline void fan_refresh_card(FanCardCtx *ctx) {
  if (!ctx) return;
  fan_apply_card_visual(ctx);
  bool persistent = ctx->type == "fan_direction" || ctx->type == "fan_preset";
  transient_status_label_show_if_changed(ctx->status_label, fan_status_text(ctx), !persistent);
}

inline void send_fan_action(const std::string &entity_id,
                            const char *service,
                            const char *data_key = nullptr,
                            const char *data_value = nullptr) {
  ha_send_entity_action(entity_id, service, data_key, data_value);
}

inline void send_fan_switch_action(FanCardCtx *ctx) {
  if (!ctx) return;
  send_fan_action(ctx->entity_id, ctx->on ? "fan.turn_off" : "fan.turn_on");
}

inline void send_fan_oscillate_action(FanCardCtx *ctx) {
  if (!ctx) return;
  send_fan_action(ctx->entity_id, "fan.oscillate", "oscillating", ctx->oscillating ? "false" : "true");
}

inline void send_fan_direction_action(FanCardCtx *ctx) {
  if (!ctx) return;
  const char *next = ctx->direction == "reverse" ? "forward" : "reverse";
  send_fan_action(ctx->entity_id, "fan.set_direction", "direction", next);
}

inline void send_fan_preset_action(FanCardCtx *ctx, const std::string &mode) {
  if (!ctx || mode.empty()) return;
  send_fan_action(ctx->entity_id, "fan.set_preset_mode", "preset_mode", mode.c_str());
}

inline void fan_preset_close() {
  FanPresetUi &ui = fan_preset_ui();
  control_modal_delete_overlay(ControlModalKind::FAN_PRESET, ui.overlay);
  ui = FanPresetUi();
}

inline void fan_preset_close_if_unsupported(FanCardCtx *ctx) {
  FanPresetUi &ui = fan_preset_ui();
  if (ui.active == ctx && !fan_control_supported(ctx)) fan_preset_close();
}

inline void fan_preset_open(FanCardCtx *ctx) {
  if (!fan_control_supported(ctx)) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::FAN_PRESET, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0156", true, fan_preset_close);
  FanPresetUi &ui = fan_preset_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.close_btn = shell.close_btn;

  ControlModalLayout &layout = shell.layout;
  lv_coord_t content_w = shell.content_w;
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  lv_coord_t row_h = control_modal_scaled_px(48, layout.short_side);
  if (row_h < 34) row_h = 34;
  lv_coord_t row_radius = row_h / 3;
  lv_coord_t title_y = layout.inset + layout.back_size / 2;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < row_h) list_h = row_h;

  ui.title_lbl = control_modal_create_title(
    ui.panel, espcontrol_i18n("Preset"), content_w - layout.back_size - gap,
    ctx->label_font, ctx->width_compensation_percent);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, title_y - layout.back_size / 2);

  ui.list = control_modal_create_scroll_list(ui.panel, content_w, list_h, gap);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset, list_y);

  int count = ctx->preset_modes.size() > FAN_PRESET_MAX_OPTIONS
    ? FAN_PRESET_MAX_OPTIONS
    : static_cast<int>(ctx->preset_modes.size());
  std::string current = fan_lower(fan_trim(ctx->preset_mode));
  for (int i = 0; i < count; i++) {
    const std::string &mode = ctx->preset_modes[i];
    bool selected = fan_lower(fan_trim(mode)) == current;
    lv_obj_t *btn = control_modal_create_list_row(
      ui.list, fan_option_label(mode), selected, row_h, row_radius,
      ctx->on_color, DARK_BACKGROUND_SECONDARY,
      ctx->label_font, ctx->width_compensation_percent);
    ui.option_clicks[i].ctx = ctx;
    ui.option_clicks[i].mode = mode;
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      FanPresetClick *click = (FanPresetClick *)lv_event_get_user_data(e);
      if (click && click->ctx) send_fan_preset_action(click->ctx, click->mode);
      fan_preset_close();
    }, LV_EVENT_CLICKED, &ui.option_clicks[i]);
  }

  if (count == 0) {
    ui.empty_lbl = lv_label_create(ui.list);
    lv_label_set_text(ui.empty_lbl, espcontrol_i18n("No presets"));
    lv_label_set_long_mode(ui.empty_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui.empty_lbl, lv_pct(100));
    lv_obj_set_style_text_color(ui.empty_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.empty_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.empty_lbl, ctx->label_font, LV_PART_MAIN);
  }

  lv_obj_move_foreground(ui.overlay);
}

inline void fan_card_handle_click(FanCardCtx *ctx) {
  if (!fan_control_supported(ctx)) return;
  if (ctx->type == "fan_switch") send_fan_switch_action(ctx);
  else if (ctx->type == "fan_oscillate") send_fan_oscillate_action(ctx);
  else if (ctx->type == "fan_direction") send_fan_direction_action(ctx);
  else if (ctx->type == "fan_preset") fan_preset_open(ctx);
}

inline FanCardCtx *create_fan_card_context(
    BtnSlot &slot, const ParsedCfg &p,
    uint32_t on_color, uint32_t off_color, uint32_t tertiary_color,
    const lv_font_t *label_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  FanCardCtx *ctx = grid_own_context(slot.btn, new FanCardCtx());
  ctx->type = p.type;
  ctx->entity_id = p.entity;
  ctx->label = fan_card_label(p);
  ctx->has_custom_label = !p.label.empty();
  ctx->btn = slot.btn;
  ctx->icon_lbl = slot.icon_lbl;
  ctx->icon_off_glyph = find_icon(fan_card_icon_name(p));
  ctx->icon_on_glyph = find_icon(fan_card_icon_on_name(p));
  ctx->on_color = on_color;
  ctx->off_color = off_color;
  ctx->tertiary_color = tertiary_color;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->status_label = create_transient_status_label(slot.text_lbl, ctx->label);
  lv_obj_set_user_data(slot.btn, ctx);
  fan_refresh_card(ctx);
  return ctx;
}

inline void subscribe_fan_card_state(FanCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  auto refresh = [ctx]() { fan_refresh_card(ctx); };
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef state) {
        ctx->state = fan_lower(fan_trim(string_ref_limited(state, HA_SHORT_STATE_MAX_LEN)));
        ctx->available = !ha_state_unavailable_ref(state);
        ctx->on = ctx->available && is_entity_on_ref(state);
        refresh();
        fan_preset_close_if_unsupported(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
        if (!ctx->has_custom_label) {
          transient_status_label_set_steady(ctx->status_label, ctx->friendly_name);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("oscillating"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        bool parsed = false;
        std::string text = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
        ctx->oscillation_known = fan_parse_bool_value(text, parsed);
        if (ctx->oscillation_known) ctx->oscillating = parsed;
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("direction"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->direction = fan_lower(fan_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        ctx->direction_known = ctx->direction == "forward" || ctx->direction == "reverse";
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("preset_mode"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->preset_mode = fan_lower(fan_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("preset_modes"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->preset_modes = fan_parse_options(value);
        refresh();
        fan_preset_close_if_unsupported(ctx);
      })
  );
}
