#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Fan card controls ─────────────────────────────────────────────────

constexpr int FAN_PRESET_MAX_OPTIONS = 32;

struct FanCardCtx {
  std::string type;
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string options;
  std::string state;
  std::string direction;
  std::string preset_mode;
  std::vector<std::string> preset_modes;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  TransientStatusLabel *status_label = nullptr;
  const char *icon_off_glyph = nullptr;
  const char *icon_on_glyph = nullptr;
  uint32_t on_color = DEFAULT_SLIDER_COLOR;
  uint32_t off_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  bool available = false;
  bool on = false;
  bool oscillating = false;
  bool oscillation_known = false;
  bool direction_known = false;
  bool percentage_known = false;
  int percentage = 0;
  bool percentage_step_known = false;
  int percentage_step = 10;
  bool supported_features_known = false;
  int supported_features = 0;
  bool dragging_speed = false;
  bool updating_speed = false;
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

enum class FanControlTab : uint8_t {
  POWER = 0,
  SPEED = 1,
  PRESET = 2,
  OSCILLATION = 3,
  DIRECTION = 4,
};

struct FanControlVisibleTabs {
  FanControlTab tabs[5] = {
    FanControlTab::POWER,
    FanControlTab::SPEED,
    FanControlTab::PRESET,
    FanControlTab::OSCILLATION,
    FanControlTab::DIRECTION,
  };
  uint8_t count = 0;

  bool contains(FanControlTab tab) const {
    for (uint8_t i = 0; i < count; i++) {
      if (tabs[i] == tab) return true;
    }
    return false;
  }

  void add(FanControlTab tab) {
    if (count >= 5 || contains(tab)) return;
    tabs[count++] = tab;
  }
};

struct FanControlPresetClick {
  FanCardCtx *ctx = nullptr;
  std::string mode;
};

struct FanControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *power_tab = nullptr;
  lv_obj_t *speed_tab = nullptr;
  lv_obj_t *preset_tab = nullptr;
  lv_obj_t *oscillation_tab = nullptr;
  lv_obj_t *direction_tab = nullptr;
  lv_obj_t *power_group = nullptr;
  lv_obj_t *power_on_btn = nullptr;
  lv_obj_t *power_off_btn = nullptr;
  lv_obj_t *speed_group = nullptr;
  lv_obj_t *speed_slider = nullptr;
  lv_obj_t *speed_fill = nullptr;
  lv_obj_t *speed_handle = nullptr;
  lv_obj_t *speed_value_lbl = nullptr;
  lv_obj_t *speed_minus_btn = nullptr;
  lv_obj_t *speed_plus_btn = nullptr;
  lv_obj_t *preset_list = nullptr;
  lv_obj_t *preset_empty_lbl = nullptr;
  lv_obj_t *oscillation_group = nullptr;
  lv_obj_t *oscillation_on_btn = nullptr;
  lv_obj_t *oscillation_off_btn = nullptr;
  lv_obj_t *direction_group = nullptr;
  lv_obj_t *direction_forward_btn = nullptr;
  lv_obj_t *direction_reverse_btn = nullptr;
  FanCardCtx *active = nullptr;
  FanControlTab tab = FanControlTab::POWER;
  FanControlPresetClick preset_clicks[FAN_PRESET_MAX_OPTIONS];
};

inline FanControlModalUi &fan_control_modal_ui() {
  static FanControlModalUi ui;
  return ui;
}

inline bool fan_non_speed_card_type(const std::string &type) {
  return type == "fan_switch" ||
         type == "fan_oscillate" ||
         type == "fan_direction" ||
         type == "fan_preset";
}

inline bool fan_control_card_type(const std::string &type) {
  return type == "fan_control";
}

constexpr int FAN_FEATURE_SET_SPEED = 1;
constexpr int FAN_FEATURE_OSCILLATE = 2;
constexpr int FAN_FEATURE_DIRECTION = 4;
constexpr int FAN_FEATURE_PRESET_MODE = 8;
constexpr int FAN_FEATURE_TURN_OFF = 16;
constexpr int FAN_FEATURE_TURN_ON = 32;

inline bool fan_feature_supported(FanCardCtx *ctx, int feature, bool fallback) {
  if (!ctx || !ctx->available) return false;
  if (!ctx->supported_features_known) return fallback;
  return (ctx->supported_features & feature) != 0;
}

inline bool fan_power_supported(FanCardCtx *ctx) {
  return ctx && ctx->available;
}

inline bool fan_speed_supported(FanCardCtx *ctx) {
  return fan_feature_supported(ctx, FAN_FEATURE_SET_SPEED, ctx && ctx->percentage_known);
}

inline bool fan_preset_supported(FanCardCtx *ctx) {
  return fan_feature_supported(ctx, FAN_FEATURE_PRESET_MODE, ctx && !ctx->preset_modes.empty()) &&
         ctx && !ctx->preset_modes.empty();
}

inline bool fan_oscillation_supported(FanCardCtx *ctx) {
  return fan_feature_supported(ctx, FAN_FEATURE_OSCILLATE, ctx && ctx->oscillation_known);
}

inline bool fan_direction_supported(FanCardCtx *ctx) {
  return fan_feature_supported(ctx, FAN_FEATURE_DIRECTION, ctx && ctx->direction_known);
}

inline bool fan_control_tab_from_token(const std::string &value, FanControlTab &tab) {
  if (value == "power") {
    tab = FanControlTab::POWER;
    return true;
  }
  if (value == "speed") {
    tab = FanControlTab::SPEED;
    return true;
  }
  if (value == "preset") {
    tab = FanControlTab::PRESET;
    return true;
  }
  if (value == "oscillation") {
    tab = FanControlTab::OSCILLATION;
    return true;
  }
  if (value == "direction") {
    tab = FanControlTab::DIRECTION;
    return true;
  }
  return false;
}

inline bool fan_control_tab_supported(FanCardCtx *ctx, FanControlTab tab) {
  switch (tab) {
    case FanControlTab::POWER: return fan_power_supported(ctx);
    case FanControlTab::SPEED: return fan_speed_supported(ctx);
    case FanControlTab::PRESET: return fan_preset_supported(ctx);
    case FanControlTab::OSCILLATION: return fan_oscillation_supported(ctx);
    case FanControlTab::DIRECTION: return fan_direction_supported(ctx);
  }
  return false;
}

inline FanControlVisibleTabs fan_control_visible_tabs(FanCardCtx *ctx) {
  FanControlVisibleTabs visible;
  std::string value = cfg_option_value(ctx ? ctx->options : "", FAN_CONTROL_TABS_OPTION);
  if (value.empty()) value = FAN_CONTROL_DEFAULT_TABS_VALUE;
  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find('|', start);
    std::string token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
    FanControlTab tab = FanControlTab::POWER;
    if (fan_control_tab_from_token(token, tab) && fan_control_tab_supported(ctx, tab)) visible.add(tab);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (visible.count == 0 && ctx && ctx->available) {
    if (fan_power_supported(ctx)) visible.add(FanControlTab::POWER);
    else if (fan_speed_supported(ctx)) visible.add(FanControlTab::SPEED);
    else if (fan_preset_supported(ctx)) visible.add(FanControlTab::PRESET);
    else if (fan_oscillation_supported(ctx)) visible.add(FanControlTab::OSCILLATION);
    else if (fan_direction_supported(ctx)) visible.add(FanControlTab::DIRECTION);
  }
  return visible;
}

inline bool fan_control_tab_visible(FanCardCtx *ctx, FanControlTab tab) {
  FanControlVisibleTabs tabs = fan_control_visible_tabs(ctx);
  return tabs.contains(tab);
}

inline FanControlTab fan_control_first_visible_tab(FanCardCtx *ctx) {
  FanControlVisibleTabs tabs = fan_control_visible_tabs(ctx);
  return tabs.count == 0 ? FanControlTab::POWER : tabs.tabs[0];
}

inline void fan_control_ensure_visible_tab(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (fan_control_tab_visible(ctx, ui.tab)) return;
  ui.tab = fan_control_first_visible_tab(ctx);
}

inline lv_obj_t *fan_control_tab_button(FanControlModalUi &ui, FanControlTab tab) {
  switch (tab) {
    case FanControlTab::POWER: return ui.power_tab;
    case FanControlTab::SPEED: return ui.speed_tab;
    case FanControlTab::PRESET: return ui.preset_tab;
    case FanControlTab::OSCILLATION: return ui.oscillation_tab;
    case FanControlTab::DIRECTION: return ui.direction_tab;
  }
  return nullptr;
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
  if (p.type == "fan_control") return espcontrol_i18n(std::string("Fan"));
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

inline void setup_fan_control_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, find_icon(fan_card_icon_name(p)));
  lv_label_set_text(s.text_lbl, fan_card_label(p).c_str());
  apply_push_button_transition(s.btn);
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
  bool active = false;
  if (ctx->type == "fan_switch") active = ctx->on;
  else if (ctx->type == "fan_control") active = ctx->on;
  else if (ctx->type == "fan_oscillate") active = ctx->oscillating;
  else if (ctx->type == "fan_direction") active = ctx->direction == "reverse";
  else if (ctx->type == "fan_preset") active = fan_preset_active(ctx->preset_mode);

  set_card_checked_state(ctx->btn, active);

  if (ctx->icon_lbl) {
    if ((ctx->type == "fan_switch" || ctx->type == "fan_control") && active && ctx->icon_on_glyph) {
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
  if (ctx->type == "fan_control") {
    if (ctx->percentage_known) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%d%%", ctx->on ? ctx->percentage : 0);
      return std::string(buf);
    }
    if (fan_preset_active(ctx->preset_mode)) return fan_option_label(ctx->preset_mode);
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

inline void fan_control_apply_tab_visibility();
inline void fan_control_layout_modal(FanCardCtx *ctx);
inline void fan_control_rebuild_preset_list(FanCardCtx *ctx);

inline void send_fan_percentage_action(FanCardCtx *ctx, int pct) {
  if (!ctx) return;
  pct = slider_clamp_pct(pct);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", pct);
  send_fan_action(ctx->entity_id, "fan.set_percentage", "percentage", buf);
}

inline void send_fan_step_action(FanCardCtx *ctx, bool increase) {
  if (!ctx) return;
  const char *service = increase ? "fan.increase_speed" : "fan.decrease_speed";
  if (ctx->percentage_step_known && ctx->percentage_step > 0) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", ctx->percentage_step);
    send_fan_action(ctx->entity_id, service, "percentage_step", buf);
  } else {
    send_fan_action(ctx->entity_id, service);
  }
}

inline void fan_control_style_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
  if (!btn) return;
  (void) accent_color;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? TERTIARY_GREY : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline lv_obj_t *fan_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                               const lv_font_t *font,
                                               FanControlTab tab) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(label, 180, LV_PART_MAIN);
    light_control_center_icon_label(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    FanControlTab tab = static_cast<FanControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    FanControlModalUi &ui = fan_control_modal_ui();
    ui.tab = tab;
    fan_control_ensure_visible_tab(ui.active);
    fan_control_apply_tab_visibility();
    fan_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *fan_control_create_icon_button(lv_obj_t *parent, const char *icon,
                                                const lv_font_t *font) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_text(label, icon);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_center(label);
  }
  return btn;
}

inline lv_obj_t *fan_control_create_tile_button(lv_obj_t *parent, const char *icon,
                                                const std::string &text,
                                                const lv_font_t *icon_font,
                                                const lv_font_t *label_font) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_left(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_row(btn, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_column(btn, 0, LV_PART_MAIN);
  lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(btn, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  if (icon && icon[0]) {
    lv_obj_t *icon_lbl = lv_label_create(btn);
    if (icon_lbl) {
      lv_label_set_text(icon_lbl, icon);
      lv_obj_set_style_text_color(icon_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      lv_obj_set_style_text_align(icon_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (icon_font) lv_obj_set_style_text_font(icon_lbl, icon_font, LV_PART_MAIN);
    }
  }

  if (!text.empty()) {
    lv_obj_t *label = lv_label_create(btn);
    if (label) {
      lv_label_set_text(label, text.c_str());
      lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(label, lv_pct(100));
      lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (label_font) lv_obj_set_style_text_font(label, label_font, LV_PART_MAIN);
    }
  }
  return btn;
}

inline void fan_control_style_tile_button(lv_obj_t *btn, bool active,
                                          uint32_t active_color,
                                          uint32_t inactive_color) {
  if (!btn) return;
  uint32_t bg_color = active ? active_color : inactive_color;
  uint32_t text_color = active ? DARK_TEXT_PRIMARY : readable_text_color_for_bg(bg_color);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  uint32_t child_count = lv_obj_get_child_count(btn);
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(btn, i);
    if (child) lv_obj_set_style_text_color(child, lv_color_hex(text_color), LV_PART_MAIN);
  }
}

inline void fan_control_apply_power(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  fan_control_style_tile_button(ui.power_on_btn, ctx->on, ctx->on_color, SECONDARY_GREY);
  fan_control_style_tile_button(ui.power_off_btn, !ctx->on, ctx->on_color, SECONDARY_GREY);
}

inline void fan_control_apply_oscillation(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  fan_control_style_tile_button(ui.oscillation_on_btn, ctx->oscillating, ctx->on_color, SECONDARY_GREY);
  fan_control_style_tile_button(ui.oscillation_off_btn, !ctx->oscillating, ctx->on_color, SECONDARY_GREY);
}

inline void fan_control_apply_direction(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  bool reverse = ctx->direction == "reverse";
  fan_control_style_tile_button(ui.direction_forward_btn, !reverse, ctx->on_color, SECONDARY_GREY);
  fan_control_style_tile_button(ui.direction_reverse_btn, reverse, ctx->on_color, SECONDARY_GREY);
}

inline void fan_control_set_speed_value(FanCardCtx *ctx, int pct) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  if (ctx->dragging_speed) return;
  pct = slider_clamp_pct(pct);
  if (ui.speed_slider) {
    ctx->updating_speed = true;
    lv_slider_set_value(ui.speed_slider, pct, LV_ANIM_OFF);
    ctx->updating_speed = false;
  }
  light_control_update_slider_fill(
    ui.speed_slider, ui.speed_fill, ui.speed_handle, pct, lv_color_hex(ctx->on_color));
  light_control_update_slider_handle(ui.speed_slider, ui.speed_handle, pct);
  if (ui.speed_value_lbl) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(ui.speed_value_lbl, buf);
  }
}

inline std::string fan_control_card_title(FanCardCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Fan"));
  if (!ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Fan"));
}

inline void fan_control_refresh_card(FanCardCtx *ctx) {
  if (!ctx) return;
  fan_apply_card_visual(ctx);
  transient_status_label_set_steady(ctx->status_label, fan_control_card_title(ctx));
  transient_status_label_show_if_changed(ctx->status_label, fan_status_text(ctx), false);
}

inline void fan_control_refresh_modal(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx || !ui.panel) return;
  fan_control_ensure_visible_tab(ctx);
  fan_control_set_speed_value(ctx, ctx->on ? ctx->percentage : 0);
  fan_control_apply_power(ctx);
  fan_control_apply_oscillation(ctx);
  fan_control_apply_direction(ctx);
  fan_control_rebuild_preset_list(ctx);
  fan_control_apply_tab_visibility();
  fan_control_layout_modal(ctx);
}

inline void fan_control_apply_tab_visibility() {
  FanControlModalUi &ui = fan_control_modal_ui();
  FanCardCtx *ctx = ui.active;
  if (!ctx) return;
  FanControlVisibleTabs visible_tabs = fan_control_visible_tabs(ctx);
  if (visible_tabs.count > 0 && !visible_tabs.contains(ui.tab)) ui.tab = visible_tabs.tabs[0];
  bool show_tab_bar = visible_tabs.count > 1;
  bool show_power = visible_tabs.contains(FanControlTab::POWER) && ui.tab == FanControlTab::POWER;
  bool show_speed = visible_tabs.contains(FanControlTab::SPEED) && ui.tab == FanControlTab::SPEED;
  bool show_preset = visible_tabs.contains(FanControlTab::PRESET) && ui.tab == FanControlTab::PRESET;
  bool show_oscillation = visible_tabs.contains(FanControlTab::OSCILLATION) && ui.tab == FanControlTab::OSCILLATION;
  bool show_direction = visible_tabs.contains(FanControlTab::DIRECTION) && ui.tab == FanControlTab::DIRECTION;
  if (ui.tab_row) {
    if (show_tab_bar) lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
  }
  struct TabVisibility {
    lv_obj_t *btn;
    FanControlTab tab;
  };
  TabVisibility buttons[] = {
    {ui.power_tab, FanControlTab::POWER},
    {ui.speed_tab, FanControlTab::SPEED},
    {ui.preset_tab, FanControlTab::PRESET},
    {ui.oscillation_tab, FanControlTab::OSCILLATION},
    {ui.direction_tab, FanControlTab::DIRECTION},
  };
  for (auto &item : buttons) {
    if (!item.btn) continue;
    if (show_tab_bar && visible_tabs.contains(item.tab)) lv_obj_clear_flag(item.btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(item.btn, LV_OBJ_FLAG_HIDDEN);
    fan_control_style_tab(item.btn, item.tab == ui.tab, ctx->on_color);
  }
  if (ui.power_group) {
    if (show_power) lv_obj_clear_flag(ui.power_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.power_group, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.speed_group) {
    if (show_speed) lv_obj_clear_flag(ui.speed_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.speed_group, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.preset_list) {
    if (show_preset) lv_obj_clear_flag(ui.preset_list, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.preset_list, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.oscillation_group) {
    if (show_oscillation) lv_obj_clear_flag(ui.oscillation_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.oscillation_group, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.direction_group) {
    if (show_direction) lv_obj_clear_flag(ui.direction_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.direction_group, LV_OBJ_FLAG_HIDDEN);
  }
  fan_control_apply_power(ctx);
  fan_control_apply_oscillation(ctx);
  fan_control_apply_direction(ctx);
}

inline lv_coord_t fan_control_tile_gap(const ControlModalLayout &layout) {
  lv_coord_t gap = control_modal_scaled_px(layout.short_side < 520 ? 10 : 12, layout.short_side);
  return gap < 8 ? 8 : gap;
}

inline lv_coord_t fan_control_tile_width(const ControlModalLayout &layout,
                                         lv_coord_t content_w,
                                         uint32_t tile_count) {
  if (tile_count == 0) return content_w;
  lv_coord_t gap = fan_control_tile_gap(layout);
  lv_coord_t min_tile_w = layout.short_side < 520 ? 118 : 138;
  lv_coord_t max_tile_w = layout.short_side < 520 ? 168 : 220;
  uint32_t columns = (content_w + gap) / (min_tile_w + gap);
  if (columns < 1) columns = 1;
  if (columns > tile_count) columns = tile_count;
  if (columns > 3 && layout.short_side < 520) columns = 3;
  lv_coord_t tile_w = (content_w - gap * (columns - 1)) / columns;
  if (tile_w > max_tile_w) tile_w = max_tile_w;
  if (tile_w < min_tile_w && content_w >= min_tile_w) tile_w = min_tile_w;
  return tile_w;
}

inline void fan_control_layout_tile_children(FanCardCtx *ctx,
                                             lv_obj_t *parent,
                                             const ControlModalLayout &layout,
                                             lv_coord_t content_w) {
  if (!ctx || !parent) return;
  uint32_t child_count = lv_obj_get_child_count(parent);
  uint32_t tile_count = 0;
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *child = lv_obj_get_child(parent, i);
    if (child && lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) tile_count++;
  }
  lv_coord_t tile_w = fan_control_tile_width(layout, content_w, tile_count);
  lv_coord_t tile_h = tile_w;
  for (uint32_t i = 0; i < child_count; i++) {
    lv_obj_t *tile = lv_obj_get_child(parent, i);
    if (!tile || !lv_obj_has_flag(tile, LV_OBJ_FLAG_CLICKABLE)) continue;
    lv_obj_set_size(tile, tile_w, tile_h);
    lv_obj_set_style_radius(tile, control_modal_card_radius(ctx->btn), LV_PART_MAIN);
    uint32_t tile_child_count = lv_obj_get_child_count(tile);
    for (uint32_t child_index = 0; child_index < tile_child_count; child_index++) {
      lv_obj_t *label = lv_obj_get_child(tile, child_index);
      if (label) lv_obj_set_width(label, lv_pct(100));
    }
  }
}

inline void fan_control_layout_tile_group(FanCardCtx *ctx,
                                          lv_obj_t *group,
                                          lv_coord_t content_w,
                                          lv_coord_t content_h,
                                          lv_coord_t top,
                                          const ControlModalLayout &layout) {
  if (!group) return;
  lv_coord_t gap = fan_control_tile_gap(layout);
  lv_obj_set_size(group, content_w, content_h);
  lv_obj_align(group, LV_ALIGN_TOP_LEFT, layout.inset, top);
  lv_obj_set_style_pad_all(group, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(group, gap, LV_PART_MAIN);
  lv_obj_set_style_pad_column(group, gap, LV_PART_MAIN);
  lv_obj_set_layout(group, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(group, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(group, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(group, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_scroll_dir(group, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(group, LV_SCROLLBAR_MODE_OFF);
  fan_control_layout_tile_children(ctx, group, layout, content_w);
}

inline void fan_control_layout_modal(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || !ui.panel) return;
  fan_control_ensure_visible_tab(ctx);
  FanControlVisibleTabs visible_tabs = fan_control_visible_tabs(ctx);
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  int tab_count = static_cast<int>(visible_tabs.count);
  bool show_tab_bar = visible_tabs.count > 1;
  ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);
  control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);
  for (int i = 0; show_tab_bar && i < tab_count; i++) {
    lv_obj_t *tab_btn = fan_control_tab_button(ui, visible_tabs.tabs[i]);
    if (!tab_btn) continue;
    bool active = visible_tabs.tabs[i] == ui.tab;
    control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);
  }

  lv_coord_t content_top = show_tab_bar
    ? layout.inset + tabs_layout.tab_frame_h + tabs_layout.content_gap
    : layout.inset * 2;
  lv_coord_t chrome_safe_top = layout.back_inset_y + layout.back_size + layout.inset / 2;
  if (!show_tab_bar && content_top < chrome_safe_top) content_top = chrome_safe_top;
  lv_coord_t content_bottom = layout.panel_h - layout.inset;
  lv_coord_t content_h = content_bottom - content_top;
  if (content_h < 160) content_h = layout.panel_h / 2;
  lv_coord_t control_w = control_modal_home_card_width(ctx->btn, layout);
  if (control_w > layout.panel_w - layout.inset * 3) control_w = layout.panel_w - layout.inset * 3;
  if (control_modal_uses_jc1060p470_tuning(layout)) {
    lv_coord_t max_width = content_h * 9 / 16;
    if (control_w > max_width) control_w = max_width;
  }
  lv_coord_t center_y = content_top + content_h / 2 - layout.panel_h / 2;
  lv_coord_t tile_content_w = layout.panel_w - layout.inset * 2;

  fan_control_layout_tile_group(ctx, ui.power_group, tile_content_w, content_h, content_top, layout);
  fan_control_layout_tile_group(ctx, ui.oscillation_group, tile_content_w, content_h, content_top, layout);
  fan_control_layout_tile_group(ctx, ui.direction_group, tile_content_w, content_h, content_top, layout);

  if (ui.speed_group) {
    lv_obj_set_size(ui.speed_group, control_w, content_h);
    lv_obj_align(ui.speed_group, LV_ALIGN_CENTER, 0, center_y);
    lv_coord_t value_h = control_modal_scaled_px(38, layout.short_side);
    if (value_h < 30) value_h = 30;
    lv_coord_t step_h = control_modal_scaled_px(56, layout.short_side);
    if (step_h < 44) step_h = 44;
    lv_coord_t gap = control_modal_scaled_px(10, layout.short_side);
    if (gap < 8) gap = 8;
    lv_coord_t slider_h = content_h - value_h - step_h - gap * 2;
    if (slider_h < 100) slider_h = content_h * 2 / 3;
    lv_coord_t slider_w = control_w;
    if (ui.speed_value_lbl) {
      lv_obj_set_size(ui.speed_value_lbl, slider_w, value_h);
      lv_obj_align(ui.speed_value_lbl, LV_ALIGN_TOP_MID, 0, 0);
    }
    light_control_layout_slider(ui.speed_slider, slider_w, slider_h,
      -content_h / 2 + value_h + gap + slider_h / 2,
      ctx->width_compensation_percent);
    light_control_update_slider_fill(
      ui.speed_slider, ui.speed_fill, ui.speed_handle, ctx->on ? ctx->percentage : 0,
      lv_color_hex(ctx->on_color));
    light_control_update_slider_handle(ui.speed_slider, ui.speed_handle, ctx->on ? ctx->percentage : 0);
    lv_coord_t btn_w = (slider_w - gap) / 2;
    if (ui.speed_minus_btn) {
      lv_obj_set_size(ui.speed_minus_btn, btn_w, step_h);
      lv_obj_set_style_radius(ui.speed_minus_btn, step_h / 2, LV_PART_MAIN);
      lv_obj_align(ui.speed_minus_btn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
      lv_obj_t *label = lv_obj_get_child(ui.speed_minus_btn, 0);
      if (label) lv_obj_center(label);
    }
    if (ui.speed_plus_btn) {
      lv_obj_set_size(ui.speed_plus_btn, btn_w, step_h);
      lv_obj_set_style_radius(ui.speed_plus_btn, step_h / 2, LV_PART_MAIN);
      lv_obj_align(ui.speed_plus_btn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
      lv_obj_t *label = lv_obj_get_child(ui.speed_plus_btn, 0);
      if (label) lv_obj_center(label);
    }
  }

  if (ui.preset_list) {
    lv_coord_t gap = fan_control_tile_gap(layout);
    lv_obj_set_size(ui.preset_list, tile_content_w, content_h);
    lv_obj_align(ui.preset_list, LV_ALIGN_TOP_LEFT, layout.inset, content_top);
    lv_obj_set_style_pad_all(ui.preset_list, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(ui.preset_list, gap, LV_PART_MAIN);
    lv_obj_set_style_pad_column(ui.preset_list, gap, LV_PART_MAIN);
    lv_obj_set_layout(ui.preset_list, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(ui.preset_list, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN);
    lv_obj_set_style_flex_main_place(ui.preset_list, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(ui.preset_list, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    fan_control_layout_tile_children(ctx, ui.preset_list, layout, tile_content_w);
  }
}

inline void fan_control_rebuild_preset_list(FanCardCtx *ctx) {
  FanControlModalUi &ui = fan_control_modal_ui();
  if (!ctx || ui.active != ctx || !ui.preset_list) return;
  lv_obj_clean(ui.preset_list);
  int count = ctx->preset_modes.size() > FAN_PRESET_MAX_OPTIONS
    ? FAN_PRESET_MAX_OPTIONS
    : static_cast<int>(ctx->preset_modes.size());
  std::string current = fan_lower(fan_trim(ctx->preset_mode));
  for (int i = 0; i < count; i++) {
    const std::string &mode = ctx->preset_modes[i];
    bool selected = fan_lower(fan_trim(mode)) == current;
    lv_obj_t *btn = fan_control_create_tile_button(
      ui.preset_list, "", fan_option_label(mode),
      ctx->icon_font, ctx->label_font);
    if (!btn) continue;
    fan_control_style_tile_button(btn, selected, ctx->on_color, SECONDARY_GREY);
    ui.preset_clicks[i].ctx = ctx;
    ui.preset_clicks[i].mode = mode;
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      FanControlPresetClick *click = (FanControlPresetClick *)lv_event_get_user_data(e);
      if (click && click->ctx && click->ctx->available) send_fan_preset_action(click->ctx, click->mode);
    }, LV_EVENT_CLICKED, &ui.preset_clicks[i]);
  }
  if (count == 0) {
    lv_obj_t *empty = lv_label_create(ui.preset_list);
    lv_label_set_text(empty, espcontrol_i18n("No presets"));
    lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(empty, lv_pct(100));
    lv_obj_set_style_text_color(empty, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(empty, ctx->label_font, LV_PART_MAIN);
    ui.preset_empty_lbl = empty;
  }
}

inline void fan_control_hide_modal() {
  FanControlModalUi &ui = fan_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = FanControlModalUi();
  control_modal_delete_overlay(ControlModalKind::FAN_CONTROL, overlay);
}

inline void fan_control_open_modal(FanCardCtx *ctx) {
  if (!ctx || !ctx->available) return;
  FanControlVisibleTabs visible_tabs = fan_control_visible_tabs(ctx);
  if (visible_tabs.count == 0) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::FAN_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, fan_control_hide_modal);
  FanControlModalUi &ui = fan_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = fan_control_first_visible_tab(ctx);
  if (!ui.panel) return;

  ui.tab_row = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.tab_row, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.tab_row, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_SCROLLABLE);
  ui.power_tab = fan_control_create_tab_button(ui.tab_row, find_icon("Power"), ctx->icon_font, FanControlTab::POWER);
  ui.speed_tab = fan_control_create_tab_button(ui.tab_row, find_icon("Fan Speed 2"), ctx->icon_font, FanControlTab::SPEED);
  ui.preset_tab = fan_control_create_tab_button(ui.tab_row, find_icon("Fan Auto"), ctx->icon_font, FanControlTab::PRESET);
  ui.oscillation_tab = fan_control_create_tab_button(ui.tab_row, find_icon("Fan"), ctx->icon_font, FanControlTab::OSCILLATION);
  ui.direction_tab = fan_control_create_tab_button(ui.tab_row, find_icon("Swap Horizontal"), ctx->icon_font, FanControlTab::DIRECTION);

  ui.power_group = lv_obj_create(ui.panel);
  ui.oscillation_group = lv_obj_create(ui.panel);
  ui.direction_group = lv_obj_create(ui.panel);
  lv_obj_t *binary_groups[] = {ui.power_group, ui.oscillation_group, ui.direction_group};
  for (lv_obj_t *group : binary_groups) {
    lv_obj_set_style_bg_opa(group, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(group, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(group, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(group, 0, LV_PART_MAIN);
    lv_obj_add_flag(group, LV_OBJ_FLAG_SCROLLABLE);
  }
  ui.power_on_btn = fan_control_create_tile_button(
    ui.power_group, find_icon("Power"), "",
    ctx->icon_font, ctx->label_font);
  ui.power_off_btn = fan_control_create_tile_button(
    ui.power_group, find_icon("Circle Outline"), "",
    ctx->icon_font, ctx->label_font);
  ui.oscillation_on_btn = fan_control_create_tile_button(
    ui.oscillation_group, find_icon("Fan"), "",
    ctx->icon_font, ctx->label_font);
  ui.oscillation_off_btn = fan_control_create_tile_button(
    ui.oscillation_group, find_icon("Circle Outline"), "",
    ctx->icon_font, ctx->label_font);
  ui.direction_forward_btn = fan_control_create_tile_button(
    ui.direction_group, find_icon("Arrow Up"), "",
    ctx->icon_font, ctx->label_font);
  ui.direction_reverse_btn = fan_control_create_tile_button(
    ui.direction_group, find_icon("Arrow Down"), "",
    ctx->icon_font, ctx->label_font);

  if (ui.power_on_btn) lv_obj_add_event_cb(ui.power_on_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.turn_on");
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.power_off_btn) lv_obj_add_event_cb(ui.power_off_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.turn_off");
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.oscillation_on_btn) lv_obj_add_event_cb(ui.oscillation_on_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.oscillate", "oscillating", "true");
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.oscillation_off_btn) lv_obj_add_event_cb(ui.oscillation_off_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.oscillate", "oscillating", "false");
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.direction_forward_btn) lv_obj_add_event_cb(ui.direction_forward_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.set_direction", "direction", "forward");
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.direction_reverse_btn) lv_obj_add_event_cb(ui.direction_reverse_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_action(ui.active->entity_id, "fan.set_direction", "direction", "reverse");
  }, LV_EVENT_CLICKED, nullptr);

  ui.speed_group = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.speed_group, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.speed_group, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.speed_group, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.speed_group, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.speed_group, LV_OBJ_FLAG_SCROLLABLE);
  ui.speed_value_lbl = lv_label_create(ui.speed_group);
  lv_obj_set_style_text_color(ui.speed_value_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.speed_value_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.speed_value_lbl, ctx->label_font, LV_PART_MAIN);
  ui.speed_slider = lv_slider_create(ui.speed_group);
  light_control_style_slider(ui.speed_slider, ctx->on_color);
  lv_slider_set_value(ui.speed_slider, slider_clamp_pct(ctx->percentage), LV_ANIM_OFF);
  ui.speed_fill = light_control_create_slider_fill(ui.speed_slider, lv_color_hex(ctx->on_color));
  ui.speed_handle = light_control_create_slider_handle(ui.speed_slider);
  ui.speed_minus_btn = fan_control_create_icon_button(ui.speed_group, find_icon("Minus"), ctx->icon_font);
  ui.speed_plus_btn = fan_control_create_icon_button(ui.speed_group, find_icon("Plus"), ctx->icon_font);
  lv_obj_add_event_cb(ui.speed_slider, [](lv_event_t *e) {
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active) ui.active->dragging_speed = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.speed_slider, [](lv_event_t *e) {
    FanControlModalUi &ui = fan_control_modal_ui();
    if (!ui.active || ui.active->updating_speed) return;
    ui.active->dragging_speed = true;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->percentage = pct;
    ui.active->percentage_known = true;
    light_control_update_slider_fill(
      slider, ui.speed_fill, ui.speed_handle, pct, lv_color_hex(ui.active->on_color));
    light_control_update_slider_handle(slider, ui.speed_handle, pct);
    if (ui.speed_value_lbl) {
      char buf[8];
      snprintf(buf, sizeof(buf), "%d%%", pct);
      lv_label_set_text(ui.speed_value_lbl, buf);
    }
    fan_control_refresh_card(ui.active);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.speed_slider, [](lv_event_t *e) {
    FanControlModalUi &ui = fan_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_speed = false;
    if (!ui.active->available) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->percentage = pct;
    ui.active->percentage_known = true;
    fan_control_set_speed_value(ui.active, pct);
    fan_control_refresh_card(ui.active);
    send_fan_percentage_action(ui.active, pct);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.speed_slider, [](lv_event_t *e) {
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active) ui.active->dragging_speed = false;
  }, LV_EVENT_PRESS_LOST, nullptr);
  if (ui.speed_minus_btn) lv_obj_add_event_cb(ui.speed_minus_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_step_action(ui.active, false);
  }, LV_EVENT_CLICKED, nullptr);
  if (ui.speed_plus_btn) lv_obj_add_event_cb(ui.speed_plus_btn, [](lv_event_t *e) {
    (void) e;
    FanControlModalUi &ui = fan_control_modal_ui();
    if (ui.active && ui.active->available) send_fan_step_action(ui.active, true);
  }, LV_EVENT_CLICKED, nullptr);

  ui.preset_list = control_modal_create_scroll_list(
    ui.panel, shell.content_w, shell.layout.panel_h - shell.layout.inset * 2,
    control_modal_scaled_px(12, shell.layout.short_side));
  fan_control_rebuild_preset_list(ctx);
  fan_control_apply_tab_visibility();
  fan_control_layout_modal(ctx);
  fan_control_set_speed_value(ctx, ctx->on ? ctx->percentage : 0);
  lv_obj_move_foreground(ui.overlay);
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
      ctx->on_color, SECONDARY_GREY,
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
  FanCardCtx *ctx = new FanCardCtx();
  ctx->type = p.type;
  ctx->entity_id = p.entity;
  ctx->label = p.type == "fan_control" ? p.label : fan_card_label(p);
  ctx->options = p.options;
  ctx->has_custom_label = !p.label.empty();
  ctx->btn = slot.btn;
  ctx->icon_lbl = slot.icon_lbl;
  ctx->label_lbl = slot.text_lbl;
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
  if (ctx->type == "fan_control") fan_control_refresh_card(ctx);
  else fan_refresh_card(ctx);
  return ctx;
}

inline void subscribe_fan_card_state(FanCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  auto refresh = [ctx]() {
    if (ctx->type == "fan_control") {
      fan_control_refresh_card(ctx);
      fan_control_refresh_modal(ctx);
    } else {
      fan_refresh_card(ctx);
    }
  };
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
          if (ctx->type == "fan_control") fan_control_refresh_card(ctx);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("supported_features"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        std::string text = string_ref_limited(value, HA_SHORT_STATE_MAX_LEN);
        if (text.empty() || text == "unknown" || text == "unavailable") {
          ctx->supported_features_known = false;
        } else {
          ctx->supported_features = atoi(text.c_str());
          ctx->supported_features_known = true;
        }
        refresh();
        if (ctx->type == "fan_control") {
          FanControlModalUi &ui = fan_control_modal_ui();
          if (ui.active == ctx && fan_control_visible_tabs(ctx).count == 0) fan_control_hide_modal();
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("percentage"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        int pct = 0;
        if (slider_parse_pct(value, pct)) {
          ctx->percentage = pct;
          ctx->percentage_known = true;
          if (ctx->type == "fan_control") fan_control_set_speed_value(ctx, ctx->on ? pct : 0);
          refresh();
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("percentage_step"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        int pct = 0;
        if (slider_parse_pct(value, pct) && pct > 0) {
          ctx->percentage_step = pct;
          ctx->percentage_step_known = true;
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
