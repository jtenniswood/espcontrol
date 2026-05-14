#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Fan card controls ─────────────────────────────────────────────────

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
  bool available = false;
  bool on = false;
  bool oscillating = false;
  bool oscillation_known = false;
  bool direction_known = false;
  bool has_custom_label = false;
};

struct FanPresetUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  FanCardCtx *active = nullptr;
};

struct FanPresetClick {
  FanCardCtx *ctx = nullptr;
  std::string mode;
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
  if (p.type == "fan_switch") return "Fan";
  if (p.type == "fan_oscillate") return "Oscillation";
  if (p.type == "fan_direction") return "Direction";
  if (p.type == "fan_preset") return "Preset";
  return "Fan";
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
  if (value.empty()) return "None";
  return sentence_cap_text(value);
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

  if (active) lv_obj_add_state(ctx->btn, LV_STATE_CHECKED);
  else lv_obj_clear_state(ctx->btn, LV_STATE_CHECKED);

  if (ctx->icon_lbl) {
    if (ctx->type == "fan_switch" && active && ctx->icon_on_glyph) {
      lv_label_set_text(ctx->icon_lbl, ctx->icon_on_glyph);
    } else if (ctx->icon_off_glyph) {
      lv_label_set_text(ctx->icon_lbl, ctx->icon_off_glyph);
    }
  }
}

inline std::string fan_status_text(FanCardCtx *ctx) {
  if (!ctx || !ctx->available) return "Unavailable";
  if (ctx->type == "fan_switch") return ctx->on ? "On" : "Off";
  if (ctx->type == "fan_oscillate") {
    if (!ctx->oscillation_known) return "Unsupported";
    return ctx->oscillating ? "Oscillating" : "Still";
  }
  if (ctx->type == "fan_direction") {
    if (!ctx->direction_known) return "Unsupported";
    return fan_option_label(ctx->direction);
  }
  if (ctx->type == "fan_preset") {
    if (ctx->preset_modes.empty()) return "Unsupported";
    return fan_preset_active(ctx->preset_mode) ? fan_option_label(ctx->preset_mode) : "Preset";
  }
  return "Fan";
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
  if (entity_id.empty() || service == nullptr || esphome::api::global_api_server == nullptr) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(data_key && data_value ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(entity_id.c_str());
  if (data_key && data_value) {
    auto &data_kv = req.data.emplace_back();
    data_kv.key = decltype(data_kv.key)(data_key);
    data_kv.value = decltype(data_kv.value)(data_value);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
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
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui = FanPresetUi();
}

inline void fan_preset_open(FanCardCtx *ctx) {
  if (!ctx || ctx->preset_modes.empty()) return;
  fan_preset_close();
  FanPresetUi &ui = fan_preset_ui();
  ui.active = ctx;
  ui.overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.overlay, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.overlay, LV_OBJ_FLAG_SCROLLABLE);

  ui.panel = lv_obj_create(ui.overlay);
  lv_obj_set_size(ui.panel, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(ui.panel, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui.panel, 58, LV_PART_MAIN);
  lv_obj_set_style_pad_left(ui.panel, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_right(ui.panel, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui.panel, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.panel, 8, LV_PART_MAIN);
  lv_obj_set_layout(ui.panel, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.panel, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_clear_flag(ui.panel, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *back_btn = lv_btn_create(ui.overlay);
  lv_obj_set_size(back_btn, 48, 48);
  lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 10, 10);
  lv_obj_set_style_radius(back_btn, 24, LV_PART_MAIN);
  lv_obj_set_style_bg_color(back_btn, lv_color_hex(ctx->tertiary_color), LV_PART_MAIN);
  lv_obj_set_style_border_width(back_btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(back_btn, [](lv_event_t *) { fan_preset_close(); }, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *back_icon = lv_label_create(back_btn);
  lv_label_set_text(back_icon, "\U000F0141");
  lv_obj_center(back_icon);

  lv_obj_t *title = lv_label_create(ui.overlay);
  lv_label_set_text(title, "Preset");
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(title, ctx->label_font, LV_PART_MAIN);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 24);

  for (const auto &mode : ctx->preset_modes) {
    bool selected = fan_lower(mode) == fan_lower(ctx->preset_mode);
    lv_obj_t *btn = lv_btn_create(ui.panel);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, 54);
    lv_obj_set_style_radius(btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, lv_color_hex(selected ? ctx->on_color : ctx->tertiary_color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, fan_option_label(mode).c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(label, ctx->label_font, LV_PART_MAIN);
    lv_obj_center(label);

    FanPresetClick *click = new FanPresetClick();
    click->ctx = ctx;
    click->mode = mode;
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      FanPresetClick *click = (FanPresetClick *)lv_event_get_user_data(e);
      if (click) send_fan_preset_action(click->ctx, click->mode);
      fan_preset_close();
    }, LV_EVENT_CLICKED, click);
  }
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
    const lv_font_t *label_font) {
  FanCardCtx *ctx = new FanCardCtx();
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
  ctx->status_label = create_transient_status_label(slot.text_lbl, ctx->label);
  lv_obj_set_user_data(slot.btn, ctx);
  fan_refresh_card(ctx);
  return ctx;
}

inline void subscribe_fan_card_state(FanCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  auto refresh = [ctx]() { fan_refresh_card(ctx); };
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef state) {
        ctx->state = fan_lower(fan_trim(string_ref_limited(state, HA_SHORT_STATE_MAX_LEN)));
        ctx->available = !ha_state_unavailable_ref(state);
        ctx->on = ctx->available && is_entity_on_ref(state);
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
        if (!ctx->has_custom_label) {
          transient_status_label_set_steady(ctx->status_label, ctx->friendly_name);
        }
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
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
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("direction"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->direction = fan_lower(fan_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        ctx->direction_known = ctx->direction == "forward" || ctx->direction == "reverse";
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("preset_mode"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->preset_mode = fan_lower(fan_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("preset_modes"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->preset_modes = fan_parse_options(value);
        refresh();
      })
  );
}
