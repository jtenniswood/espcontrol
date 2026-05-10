#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Climate control card helpers ─────────────────────────────────────

constexpr uint32_t CLIMATE_HEATING_COLOR = 0xA44A1C;
constexpr uint32_t CLIMATE_COOLING_COLOR = 0x1565C0;
constexpr uint32_t CLIMATE_POPUP_COLOR = 0x242424;
constexpr uint32_t CLIMATE_TEXT_COLOR = 0xD8D8D8;
constexpr uint32_t CLIMATE_NEUTRAL_COLOR = 0xBDBDBD;
constexpr int CLIMATE_DEFAULT_TARGET_TENTHS = 200;
constexpr int CLIMATE_DEFAULT_LOW_TENTHS = 180;
constexpr int CLIMATE_DEFAULT_HIGH_TENTHS = 220;
constexpr int CLIMATE_DEFAULT_MIN_TENTHS = 50;
constexpr int CLIMATE_DEFAULT_MAX_TENTHS = 350;
constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;
constexpr uint32_t CLIMATE_TEMP_DEBOUNCE_MS = 450;

struct ClimateControlCtx {
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  std::string hvac_mode = "off";
  std::string hvac_action = "idle";
  std::vector<std::string> hvac_modes;
  std::string fan_mode;
  std::vector<std::string> fan_modes;
  std::string swing_mode;
  std::vector<std::string> swing_modes;
  std::string preset_mode;
  std::vector<std::string> preset_modes;
  bool available = false;
  bool has_target = false;
  bool has_current = false;
  bool has_low = false;
  bool has_high = false;
  bool edit_high = false;
  int current_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  int target_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  int low_tenths = CLIMATE_DEFAULT_LOW_TENTHS;
  int high_tenths = CLIMATE_DEFAULT_HIGH_TENTHS;
  int min_tenths = CLIMATE_DEFAULT_MIN_TENTHS;
  int max_tenths = CLIMATE_DEFAULT_MAX_TENTHS;
  bool custom_min = false;
  bool custom_max = false;
  int step_tenths = CLIMATE_DEFAULT_STEP_TENTHS;
  int precision = 0;
  int pending_target_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  bool pending_temp_send = false;
  lv_timer_t *debounce_timer = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  lv_obj_t *btn = nullptr;
  lv_obj_t *label_lbl = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  int width_compensation_percent = 100;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *unit_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
};

struct ClimateControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *mode_btn = nullptr;
  lv_obj_t *menu_view = nullptr;
  lv_obj_t *menu_close_btn = nullptr;
  lv_obj_t *menu_mode_btn = nullptr;
  lv_obj_t *menu_preset_btn = nullptr;
  lv_obj_t *option_list_view = nullptr;
  lv_obj_t *arc = nullptr;
  lv_obj_t *current_dot = nullptr;
  lv_obj_t *target_row = nullptr;
  lv_obj_t *target_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *status_lbl = nullptr;
  lv_obj_t *hint_lbl = nullptr;
  lv_obj_t *low_btn = nullptr;
  lv_obj_t *high_btn = nullptr;
  lv_obj_t *minus_btn = nullptr;
  lv_obj_t *plus_btn = nullptr;
  lv_obj_t *chips = nullptr;
  lv_obj_t *fan_chip = nullptr;
  lv_obj_t *swing_chip = nullptr;
  lv_obj_t *menu_overlay = nullptr;
  ClimateControlCtx *active = nullptr;
  bool updating_arc = false;
  bool dragging_arc = false;
  bool action_menu_open = false;
};

struct ClimateOptionClick {
  ClimateControlCtx *ctx = nullptr;
  std::string kind;
  std::string value;
};

inline ClimateControlModalUi &climate_control_modal_ui() {
  static ClimateControlModalUi ui;
  return ui;
}

inline ClimateControlCtx **climate_control_refs() {
  static ClimateControlCtx *refs[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return refs;
}

inline int &climate_control_ref_count() {
  static int count = 0;
  return count;
}

inline void reset_climate_control_refs() {
  climate_control_ref_count() = 0;
}

inline std::string climate_lower(std::string value) {
  for (char &ch : value) ch = (char)std::tolower((unsigned char)ch);
  return value;
}

inline std::string climate_trim(const std::string &value) {
  size_t start = 0;
  while (start < value.size() && std::isspace((unsigned char)value[start])) start++;
  size_t end = value.size();
  while (end > start && std::isspace((unsigned char)value[end - 1])) end--;
  return value.substr(start, end - start);
}

inline bool climate_unavailable_value(const std::string &value) {
  return value.empty() || value == "unknown" || value == "unavailable";
}

inline bool climate_parse_tenths(esphome::StringRef value, int &out) {
  float parsed = 0.0f;
  if (!parse_float_ref(value, parsed) || !std::isfinite(parsed)) return false;
  out = (int)(parsed * 10.0f + (parsed >= 0.0f ? 0.5f : -0.5f));
  return true;
}

inline bool climate_parse_tenths_string(const std::string &value, int &out) {
  char *end = nullptr;
  float parsed = strtof(value.c_str(), &end);
  if (end == value.c_str() || !std::isfinite(parsed)) return false;
  out = (int)(parsed * 10.0f + (parsed >= 0.0f ? 0.5f : -0.5f));
  return true;
}

inline void climate_apply_saved_range(ClimateControlCtx *ctx, const std::string &precision) {
  if (!ctx) return;
  size_t first = precision.find(':');
  if (first == std::string::npos) return;
  size_t second = precision.find(':', first + 1);
  std::string min_value = second == std::string::npos
    ? precision.substr(first + 1)
    : precision.substr(first + 1, second - first - 1);
  std::string max_value = second == std::string::npos ? "" : precision.substr(second + 1);
  int tenths = 0;
  if (!min_value.empty() && climate_parse_tenths_string(min_value, tenths)) {
    ctx->min_tenths = tenths;
    ctx->custom_min = true;
  }
  if (!max_value.empty() && climate_parse_tenths_string(max_value, tenths)) {
    ctx->max_tenths = tenths;
    ctx->custom_max = true;
  }
  if (ctx->max_tenths <= ctx->min_tenths) ctx->max_tenths = ctx->min_tenths + 10;
}

inline int climate_clamp_tenths(ClimateControlCtx *ctx, int value) {
  if (!ctx) return value;
  if (ctx->max_tenths <= ctx->min_tenths) ctx->max_tenths = ctx->min_tenths + 10;
  if (value < ctx->min_tenths) value = ctx->min_tenths;
  if (value > ctx->max_tenths) value = ctx->max_tenths;
  return value;
}

inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {
  if (!ctx) return value;
  int step = ctx->step_tenths > 0 && ctx->step_tenths <= 100 ? ctx->step_tenths : CLIMATE_DEFAULT_STEP_TENTHS;
  int base = ctx->min_tenths;
  int delta = value - base;
  int rounded = base + ((delta >= 0 ? delta + step / 2 : delta - step / 2) / step) * step;
  return climate_clamp_tenths(ctx, rounded);
}

inline bool climate_dual_target(ClimateControlCtx *ctx) {
  return ctx && ctx->hvac_mode == "heat_cool" && ctx->has_low && ctx->has_high;
}

inline int climate_selected_target(ClimateControlCtx *ctx) {
  if (!ctx) return CLIMATE_DEFAULT_TARGET_TENTHS;
  if (climate_dual_target(ctx)) return ctx->edit_high ? ctx->high_tenths : ctx->low_tenths;
  if (ctx->has_target) return ctx->target_tenths;
  if (ctx->has_low) return ctx->low_tenths;
  if (ctx->has_high) return ctx->high_tenths;
  return climate_clamp_tenths(ctx, CLIMATE_DEFAULT_TARGET_TENTHS);
}

inline std::string climate_format_tenths(int value, int precision) {
  char buf[20];
  if (precision <= 0) {
    int whole = value >= 0 ? (value + 5) / 10 : (value - 5) / 10;
    snprintf(buf, sizeof(buf), "%d", whole);
  } else {
    int sign = value < 0 ? -1 : 1;
    int abs_v = value < 0 ? -value : value;
    int whole = abs_v / 10;
    int tenth = abs_v % 10;
    if (precision == 1) snprintf(buf, sizeof(buf), "%s%d.%d", sign < 0 ? "-" : "", whole, tenth);
    else if (precision == 2) snprintf(buf, sizeof(buf), "%s%d.%d0", sign < 0 ? "-" : "", whole, tenth);
    else snprintf(buf, sizeof(buf), "%s%d.%d00", sign < 0 ? "-" : "", whole, tenth);
  }
  return buf;
}

inline std::string climate_option_label(const std::string &raw) {
  std::string value = climate_lower(climate_trim(raw));
  if (value == "off") return "Off";
  if (value == "heat") return "Heat";
  if (value == "cool") return "Cool";
  if (value == "heat_cool") return "Heat/Cool";
  if (value == "auto") return "Auto";
  if (value == "dry") return "Dry";
  if (value == "fan_only") return "Fan";
  return sentence_cap_text(value);
}

inline std::string climate_clean_option_token(std::string v) {
  v = climate_trim(v);
  while (!v.empty() && (v.front() == '\'' || v.front() == '"' ||
                        v.front() == '[' || v.front() == '<')) v.erase(v.begin());
  while (!v.empty() && (v.back() == '\'' || v.back() == '"' ||
                        v.back() == ']' || v.back() == '>')) v.pop_back();
  v = climate_trim(v);

  size_t colon = v.find(':');
  if (colon != std::string::npos) {
    std::string left = climate_trim(v.substr(0, colon));
    size_t dot = left.rfind('.');
    if (dot != std::string::npos && dot + 1 < left.size()) {
      v = climate_trim(left.substr(dot + 1));
    } else {
      v = climate_trim(v.substr(colon + 1));
    }
  } else {
    size_t dot = v.rfind('.');
    if (dot != std::string::npos && dot + 1 < v.size()) {
      std::string prefix = climate_lower(v.substr(0, dot));
      if (prefix.find("mode") != std::string::npos) v = climate_trim(v.substr(dot + 1));
    }
  }

  while (!v.empty() && (v.front() == '\'' || v.front() == '"' ||
                        v.front() == '<')) v.erase(v.begin());
  while (!v.empty() && (v.back() == '\'' || v.back() == '"' ||
                        v.back() == '>')) v.pop_back();
  return climate_trim(v);
}

inline std::string climate_hvac_service_value(const std::string &raw) {
  std::string value = climate_lower(climate_clean_option_token(raw));
  for (char &ch : value) {
    if (ch == ' ' || ch == '-' || ch == '/') ch = '_';
  }
  if (value == "heatcool") return "heat_cool";
  if (value == "heat__cool") return "heat_cool";
  if (value == "fan" || value == "fanonly") return "fan_only";
  return value;
}

inline std::string climate_action_label(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return "Unavailable";
  if (ctx->hvac_action == "heating") return "Heating";
  if (ctx->hvac_action == "cooling") return "Cooling";
  if (ctx->hvac_action == "drying") return "Drying";
  if (ctx->hvac_action == "fan") return "Fan";
  if (ctx->hvac_mode == "off") return "Off";
  return "Idle";
}

inline bool climate_is_active(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available || ctx->hvac_mode == "off") return false;
  return !(ctx->hvac_action.empty() || ctx->hvac_action == "idle" ||
           ctx->hvac_action == "off" || ctx->hvac_action == "unknown" ||
           ctx->hvac_action == "unavailable");
}

inline bool climate_temperature_controls_enabled(ClimateControlCtx *ctx) {
  return ctx && ctx->available && ctx->hvac_mode != "off";
}

inline int climate_arc_angle_for_tenths(ClimateControlCtx *ctx, int value) {
  if (!ctx || ctx->max_tenths <= ctx->min_tenths) return 135;
  value = climate_clamp_tenths(ctx, value);
  int span = ctx->max_tenths - ctx->min_tenths;
  int offset = (value - ctx->min_tenths) * 270 / span;
  return (135 + offset) % 360;
}

inline void climate_layout_current_dot(ClimateControlCtx *ctx, const ControlModalLayout &layout) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || !ui.current_dot) return;
  lv_coord_t dot_size = control_modal_scaled_px(10, layout.short_side);
  if (dot_size < 8) dot_size = 8;
  lv_coord_t radius = layout.arc_size / 2 - layout.arc_stroke - dot_size / 2;
  if (radius < 0) radius = layout.arc_size / 2;
  int angle = climate_arc_angle_for_tenths(ctx, ctx->current_tenths);
  float radians = (float)angle * 3.14159265f / 180.0f;
  lv_coord_t arc_left = layout.panel_w / 2 + layout.arc_center_x - layout.arc_size / 2;
  lv_coord_t arc_top = layout.panel_h / 2 + layout.arc_center_y - layout.arc_size / 2;
  lv_coord_t center_x = arc_left + layout.arc_size / 2;
  lv_coord_t center_y = arc_top + layout.arc_size / 2;
  float x_radius = radius;
  float y_radius = radius;
  int width_percent = normalize_width_compensation_percent(ctx->width_compensation_percent);
  if (width_compensation_vertical_axis()) y_radius = y_radius * width_percent / 100.0f;
  else x_radius = x_radius * width_percent / 100.0f;
  lv_obj_set_size(ui.current_dot, dot_size, dot_size);
  lv_obj_set_style_radius(ui.current_dot, dot_size / 2, LV_PART_MAIN);
  lv_obj_set_pos(ui.current_dot,
    center_x + (lv_coord_t)(std::cos(radians) * x_radius) - dot_size / 2,
    center_y + (lv_coord_t)(std::sin(radians) * y_radius) - dot_size / 2);
}

inline uint32_t climate_active_color(ClimateControlCtx *ctx) {
  if (!ctx) return DEFAULT_SLIDER_COLOR;
  if (ctx->hvac_action == "heating") return CLIMATE_HEATING_COLOR;
  if (ctx->hvac_action == "cooling") return CLIMATE_COOLING_COLOR;
  return ctx->accent_color;
}

inline std::string climate_card_value(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return "--";
  if (ctx->hvac_mode == "off") return "Off";
  if (ctx->has_low && ctx->has_high)
    return climate_format_tenths(ctx->low_tenths, ctx->precision) + "-" +
           climate_format_tenths(ctx->high_tenths, ctx->precision);
  if (ctx->has_target) return climate_format_tenths(ctx->target_tenths, ctx->precision);
  if (ctx->has_low) return climate_format_tenths(ctx->low_tenths, ctx->precision);
  if (ctx->has_high) return climate_format_tenths(ctx->high_tenths, ctx->precision);
  return "--";
}

inline std::string climate_card_label(ClimateControlCtx *ctx) {
  if (!ctx) return "Climate";
  if (climate_is_active(ctx)) return climate_action_label(ctx);
  if (!ctx->configured_label.empty()) return ctx->configured_label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  if (!ctx->entity_id.empty()) return ctx->entity_id;
  return "Climate";
}

inline void climate_update_card(ClimateControlCtx *ctx) {
  if (!ctx) return;
  std::string value = climate_card_value(ctx);
  if (ctx->value_lbl) lv_label_set_text(ctx->value_lbl, value.c_str());
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, (value == "--" || value == "Off") ? "" : display_temperature_unit_symbol());
  if (ctx->label_lbl) lv_label_set_text(ctx->label_lbl, climate_card_label(ctx).c_str());
  if (ctx->btn) {
    if (climate_is_active(ctx)) lv_obj_add_state(ctx->btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(ctx->btn, LV_STATE_CHECKED);
  }
}

inline void climate_send_action(const std::string &entity_id,
                                const char *service,
                                const std::vector<std::pair<const char *, std::string>> &data) {
  if (entity_id.empty() || service == nullptr || service[0] == '\0') return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(data.size() + 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(entity_id.c_str());
  for (const auto &item : data) {
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)(item.first);
    kv.value = decltype(kv.value)(item.second.c_str());
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline std::string climate_service_temp_value(int tenths) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%d.%d", tenths / 10, std::abs(tenths % 10));
  return buf;
}

inline void climate_send_temperature(ClimateControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !climate_temperature_controls_enabled(ctx)) return;
  if (climate_dual_target(ctx)) {
    climate_send_action(ctx->entity_id, "climate.set_temperature", {
      {"target_temp_low", climate_service_temp_value(ctx->low_tenths)},
      {"target_temp_high", climate_service_temp_value(ctx->high_tenths)},
    });
  } else {
    climate_send_action(ctx->entity_id, "climate.set_temperature", {
      {"temperature", climate_service_temp_value(ctx->target_tenths)},
    });
  }
  ctx->pending_temp_send = false;
}

inline void climate_debounce_timer_cb(lv_timer_t *timer) {
  ClimateControlCtx *ctx = static_cast<ClimateControlCtx *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  climate_send_temperature(ctx);
  lv_timer_pause(timer);
}

inline void climate_schedule_temperature_send(ClimateControlCtx *ctx) {
  if (!ctx) return;
  ctx->pending_temp_send = true;
  if (!ctx->debounce_timer) {
    ctx->debounce_timer = lv_timer_create(climate_debounce_timer_cb, CLIMATE_TEMP_DEBOUNCE_MS, ctx);
  }
  if (ctx->debounce_timer) {
    lv_timer_set_period(ctx->debounce_timer, CLIMATE_TEMP_DEBOUNCE_MS);
    lv_timer_reset(ctx->debounce_timer);
    lv_timer_resume(ctx->debounce_timer);
  }
}

inline void climate_apply_selected_target(ClimateControlCtx *ctx, int value, bool send_now, bool debounce);
inline void climate_control_set_modal_value(ClimateControlCtx *ctx);

inline void climate_apply_selected_target(ClimateControlCtx *ctx, int value, bool send_now, bool debounce) {
  if (!ctx) return;
  if (!climate_temperature_controls_enabled(ctx)) {
    climate_control_set_modal_value(ctx);
    return;
  }
  value = climate_round_to_step(ctx, value);
  if (climate_dual_target(ctx)) {
    int gap = ctx->step_tenths > 0 ? ctx->step_tenths : CLIMATE_DEFAULT_STEP_TENTHS;
    if (ctx->edit_high) {
      if (value <= ctx->low_tenths) value = ctx->low_tenths + gap;
      ctx->high_tenths = climate_clamp_tenths(ctx, value);
      ctx->has_high = true;
    } else {
      if (value >= ctx->high_tenths) value = ctx->high_tenths - gap;
      ctx->low_tenths = climate_clamp_tenths(ctx, value);
      ctx->has_low = true;
    }
  } else {
    ctx->target_tenths = value;
    ctx->has_target = true;
  }
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ui.dragging_arc) climate_update_card(ctx);
  climate_control_set_modal_value(ctx);
  if (send_now) climate_send_temperature(ctx);
  else if (debounce) climate_schedule_temperature_send(ctx);
}

inline std::vector<std::string> climate_parse_options(esphome::StringRef value) {
  std::string raw = string_ref_limited(value, HA_TEXT_SENSOR_STATE_MAX_LEN);
  std::vector<std::string> out;
  std::string token;
  auto flush = [&]() {
    std::string v = climate_clean_option_token(token);
    token.clear();
    if (v.empty()) return;
    std::string lower = climate_lower(v);
    if (lower == "hvacmode" || lower == "fanmode" || lower == "swingmode" || lower == "presetmode") return;
    bool all_caps = true;
    bool has_alpha = false;
    for (char ch : v) {
      if (std::isalpha((unsigned char)ch)) {
        has_alpha = true;
        if (!std::isupper((unsigned char)ch)) all_caps = false;
      }
    }
    if (has_alpha && all_caps) v = lower;
    for (const auto &existing : out) {
      if (climate_lower(existing) == climate_lower(v)) return;
    }
    out.push_back(v);
  };
  for (char ch : raw) {
    if (ch == ',' || ch == '[' || ch == ']') flush();
    else token.push_back(ch);
  }
  flush();
  return out;
}

inline void climate_set_obj_visible(lv_obj_t *obj, bool visible);

inline void climate_hide_option_menu() {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.menu_overlay) lv_obj_del(ui.menu_overlay);
  ui.menu_overlay = nullptr;
}

inline void climate_hide_inline_option_list() {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.option_list_view) lv_obj_del(ui.option_list_view);
  ui.option_list_view = nullptr;
  climate_set_obj_visible(ui.menu_mode_btn, ui.action_menu_open);
  climate_set_obj_visible(ui.menu_preset_btn, ui.action_menu_open);
}

inline void climate_update_chip(lv_obj_t *chip, const char *title, const std::string &value,
                                bool visible) {
  if (!chip) return;
  if (!visible) {
    lv_obj_add_flag(chip, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *label = lv_obj_get_child(chip, 0);
  if (!label) return;
  std::string text = std::string(title) + "\n" + (value.empty() ? "None" : climate_option_label(value));
  lv_label_set_text(label, text.c_str());
}

inline void climate_send_option(ClimateControlCtx *ctx, const std::string &kind, const std::string &value) {
  if (!ctx || value.empty()) return;
  if (kind == "hvac") {
    std::string service_value = climate_hvac_service_value(value);
    ctx->hvac_mode = service_value;
    climate_send_action(ctx->entity_id, "climate.set_hvac_mode", {{"hvac_mode", service_value}});
  } else if (kind == "fan") {
    ctx->fan_mode = value;
    climate_send_action(ctx->entity_id, "climate.set_fan_mode", {{"fan_mode", value}});
  } else if (kind == "swing") {
    ctx->swing_mode = value;
    climate_send_action(ctx->entity_id, "climate.set_swing_mode", {{"swing_mode", value}});
  } else if (kind == "preset") {
    ctx->preset_mode = value;
    climate_send_action(ctx->entity_id, "climate.set_preset_mode", {{"preset_mode", value}});
  }
  climate_update_card(ctx);
  climate_control_set_modal_value(ctx);
}

inline std::string climate_option_current_value(ClimateControlCtx *ctx, const std::string &kind) {
  if (!ctx) return "";
  if (kind == "hvac") return ctx->hvac_mode;
  if (kind == "fan") return ctx->fan_mode;
  if (kind == "swing") return ctx->swing_mode;
  if (kind == "preset") return ctx->preset_mode;
  return "";
}

inline bool climate_option_selected(ClimateControlCtx *ctx,
                                    const std::string &kind,
                                    const std::string &value) {
  if (!ctx) return false;
  if (kind == "hvac") return climate_hvac_service_value(value) == ctx->hvac_mode;
  return value == climate_option_current_value(ctx, kind);
}

inline const char *climate_option_icon(const std::string &kind, const std::string &value) {
  if (kind == "preset") return find_icon("Air Filter");
  if (kind == "fan") return find_icon("Fan");
  std::string mode = kind == "hvac" ? climate_hvac_service_value(value) : climate_lower(value);
  if (mode == "off") return find_icon("Power");
  if (mode == "heat") return find_icon("Fire");
  if (mode == "cool") return find_icon("Snowflake");
  if (mode == "heat_cool" || mode == "auto") return find_icon("Thermostat Auto");
  if (mode == "dry") return find_icon("Water");
  if (mode == "fan_only" || mode == "fan") return find_icon("Fan");
  return find_icon("Thermostat");
}

inline lv_obj_t *climate_create_chip(lv_obj_t *parent, const char *title,
                                     const lv_font_t *font,
                                     uint32_t bg_color,
                                     int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 96, 52);
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_radius(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(0x454545), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, title);
  lv_obj_set_style_text_color(label, lv_color_hex(CLIMATE_TEXT_COLOR), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
}

inline void climate_set_obj_visible(lv_obj_t *obj, bool visible) {
  if (!obj) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void climate_set_dial_controls_visible(bool visible) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  climate_set_obj_visible(ui.back_btn, visible);
  climate_set_obj_visible(ui.mode_btn, visible);
  climate_set_obj_visible(ui.arc, visible);
  climate_set_obj_visible(ui.target_row, visible);
  climate_set_obj_visible(ui.status_lbl, visible);
  climate_set_obj_visible(ui.hint_lbl, visible);
  climate_set_obj_visible(ui.low_btn, visible);
  climate_set_obj_visible(ui.high_btn, visible);
  climate_set_obj_visible(ui.minus_btn, visible);
  climate_set_obj_visible(ui.plus_btn, visible);
  climate_set_obj_visible(ui.chips, visible);
}

inline lv_obj_t *climate_create_menu_tile(lv_obj_t *parent, const char *icon,
                                          const char *title,
                                          const lv_font_t *icon_font,
                                          const lv_font_t *label_font,
                                          int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 240, 94);
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_radius(btn, 22, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x2B2B2B), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(btn, 26, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 22, LV_PART_MAIN);
  lv_obj_set_style_pad_column(btn, 24, LV_PART_MAIN);
  lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(btn, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_START, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  lv_obj_t *icon_lbl = lv_label_create(btn);
  lv_label_set_text(icon_lbl, icon);
  lv_obj_set_style_text_color(icon_lbl, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
  if (icon_font) lv_obj_set_style_text_font(icon_lbl, icon_font, LV_PART_MAIN);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, title);
  lv_obj_set_style_text_color(label, lv_color_hex(0xE8E8E8), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(label, label_font, LV_PART_MAIN);
  return btn;
}

inline void climate_update_menu_tile(lv_obj_t *btn, const char *title,
                                     const std::string &value, bool visible) {
  if (!btn) return;
  climate_set_obj_visible(btn, visible);
  lv_obj_t *label = lv_obj_get_child(btn, 1);
  if (!label) return;
  std::string text = std::string(title) + "\n" +
    (value.empty() ? "None" : climate_option_label(value));
  lv_label_set_text(label, text.c_str());
}

inline void climate_hide_action_menu();
inline void climate_control_layout_modal(ClimateControlCtx *ctx);

inline void climate_open_inline_option_list(ClimateControlCtx *ctx, const std::string &kind) {
  if (!ctx) return;
  ClimateControlModalUi &ui = climate_control_modal_ui();
  const std::vector<std::string> *options = nullptr;
  const char *title = nullptr;
  bool combined = kind == "all";
  if (kind == "hvac") {
    options = &ctx->hvac_modes;
    title = "";
  } else if (kind == "preset") {
    options = &ctx->preset_modes;
    title = "Preset";
  }
  if (!ui.menu_view) return;
  if (combined) {
    if (ctx->hvac_modes.empty()) return;
  } else if (!options || options->empty()) {
    return;
  }

  climate_hide_inline_option_list();
  climate_set_obj_visible(ui.menu_mode_btn, false);
  climate_set_obj_visible(ui.menu_preset_btn, false);

  ui.option_list_view = lv_obj_create(ui.menu_view);
  lv_obj_set_size(ui.option_list_view, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(ui.option_list_view, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.option_list_view, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.option_list_view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_top(ui.option_list_view, 58, LV_PART_MAIN);
  lv_obj_set_style_pad_left(ui.option_list_view, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_right(ui.option_list_view, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(ui.option_list_view, 10, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.option_list_view, 8, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.option_list_view, 10, LV_PART_MAIN);
  lv_obj_set_layout(ui.option_list_view, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.option_list_view, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_clear_flag(ui.option_list_view, LV_OBJ_FLAG_SCROLLABLE);

  auto add_section = [&](lv_obj_t *parent,
                         const char *section_title,
                         const std::string &section_kind,
                         const std::vector<std::string> &section_options) {
    if (section_options.empty()) return;

    if (section_title && section_title[0]) {
      lv_obj_t *title_lbl = lv_label_create(parent);
      lv_label_set_text(title_lbl, section_title);
      lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
      lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (ctx->label_font) lv_obj_set_style_text_font(title_lbl, ctx->label_font, LV_PART_MAIN);
      lv_obj_set_width(title_lbl, lv_pct(100));
    }

    for (const auto &option : section_options) {
      bool selected = climate_option_selected(ctx, section_kind, option);
      lv_obj_t *btn = lv_btn_create(parent);
      lv_obj_set_width(btn, lv_pct(100));
      lv_obj_set_height(btn, 70);
      lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
      lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_left(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_right(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_column(btn, 0, LV_PART_MAIN);
      lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
      lv_obj_set_style_flex_flow(btn, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
      lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

      lv_obj_t *label = lv_label_create(btn);
      lv_label_set_text(label, climate_option_label(option).c_str());
      lv_obj_set_style_text_color(label, lv_color_hex(selected ? ctx->accent_color : 0xE8E8E8), LV_PART_MAIN);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (ctx->label_font) lv_obj_set_style_text_font(label, ctx->label_font, LV_PART_MAIN);

      ClimateOptionClick *click = new ClimateOptionClick();
      click->ctx = ctx;
      click->kind = section_kind;
      click->value = option;
      lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        ClimateOptionClick *click = (ClimateOptionClick *)lv_event_get_user_data(e);
        if (click) climate_send_option(click->ctx, click->kind, click->value);
        climate_hide_action_menu();
      }, LV_EVENT_CLICKED, click);
    }
  };

  if (combined) {
    add_section(ui.option_list_view, "", "hvac", ctx->hvac_modes);
  } else {
    add_section(ui.option_list_view, title, kind, *options);
  }
  lv_obj_move_foreground(ui.option_list_view);
  climate_control_layout_modal(ctx);
}

inline void climate_show_action_menu(ClimateControlCtx *ctx);

inline void climate_open_option_menu(ClimateControlCtx *ctx, const std::string &kind) {
  if (!ctx) return;
  climate_hide_option_menu();
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.action_menu_open && (kind == "hvac" || kind == "preset")) {
    climate_open_inline_option_list(ctx, kind);
    return;
  }
  const std::vector<std::string> *options = nullptr;
  if (kind == "hvac") options = &ctx->hvac_modes;
  else if (kind == "fan") options = &ctx->fan_modes;
  else if (kind == "swing") options = &ctx->swing_modes;
  else if (kind == "preset") options = &ctx->preset_modes;
  if (!options || options->empty()) return;

  ui.menu_overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(ui.menu_overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(ui.menu_overlay, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.menu_overlay, kind == "hvac" ? LV_OPA_TRANSP : LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.menu_overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.menu_overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.menu_overlay, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui.menu_overlay, [](lv_event_t *) {
    climate_hide_option_menu();
  }, LV_EVENT_CLICKED, nullptr);

  lv_obj_t *box = lv_obj_create(ui.menu_overlay);
  lv_obj_set_width(box, kind == "hvac" ? 190 : 220);
  lv_obj_set_height(box, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(box, lv_color_hex(ctx->secondary_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(box, kind == "hvac" ? 8 : 14, LV_PART_MAIN);
  lv_obj_set_style_pad_all(box, kind == "hvac" ? 12 : 10, LV_PART_MAIN);
  lv_obj_set_style_pad_row(box, kind == "hvac" ? 0 : 6, LV_PART_MAIN);
  lv_obj_set_layout(box, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(box, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  if (kind == "hvac") lv_obj_align(box, LV_ALIGN_TOP_RIGHT, -10, 64);
  else lv_obj_align(box, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

  for (size_t option_index = 0; option_index < options->size(); option_index++) {
    const auto &option = (*options)[option_index];
    bool selected = climate_option_selected(ctx, kind, option);
    lv_obj_t *btn = lv_btn_create(box);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, kind == "hvac" ? 52 : 42);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(btn, kind == "hvac" ? 8 : 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(btn, kind == "hvac" ? 8 : 0, LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, kind == "hvac" ? 6 : 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, kind == "hvac" ? 6 : 0, LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, climate_option_label(option).c_str());
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(label, ctx->label_font, LV_PART_MAIN);
    if (kind == "hvac") {
      lv_obj_set_width(label, lv_pct(78));
      lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
      if (selected) {
        lv_obj_t *check_lbl = lv_label_create(btn);
        lv_label_set_text(check_lbl, find_icon("Check"));
        lv_obj_set_style_text_color(check_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        if (ctx->icon_font) lv_obj_set_style_text_font(check_lbl, ctx->icon_font, LV_PART_MAIN);
        lv_obj_set_style_transform_zoom(check_lbl, 170, LV_PART_MAIN);
        lv_obj_align(check_lbl, LV_ALIGN_RIGHT_MID, 0, 0);
      }
    } else {
      lv_obj_center(label);
    }
    ClimateOptionClick *click = new ClimateOptionClick();
    click->ctx = ctx;
    click->kind = kind;
    click->value = option;
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      ClimateOptionClick *click = (ClimateOptionClick *)lv_event_get_user_data(e);
      if (click) climate_send_option(click->ctx, click->kind, click->value);
      climate_hide_option_menu();
    }, LV_EVENT_CLICKED, click);
    if (kind == "hvac" && option_index + 1 < options->size()) {
      lv_obj_t *divider = lv_obj_create(box);
      lv_obj_set_width(divider, lv_pct(100));
      lv_obj_set_height(divider, 1);
      lv_obj_set_style_bg_color(divider, lv_color_hex(ctx->tertiary_color), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(divider, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
      lv_obj_clear_flag(divider, LV_OBJ_FLAG_SCROLLABLE);
    }
  }
  lv_obj_move_foreground(ui.menu_overlay);
}

inline void climate_control_set_modal_value(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  bool temp_enabled = climate_temperature_controls_enabled(ctx);
  int target = climate_selected_target(ctx);
  if (ui.arc) {
    climate_set_obj_visible(ui.arc, temp_enabled);
    if (temp_enabled && !ui.dragging_arc) {
      ui.updating_arc = true;
      lv_arc_set_range(ui.arc, ctx->min_tenths, ctx->max_tenths);
      lv_arc_set_value(ui.arc, climate_clamp_tenths(ctx, target));
      lv_obj_set_style_arc_color(ui.arc, lv_color_hex(climate_is_active(ctx) ? climate_active_color(ctx) : ctx->secondary_color), LV_PART_INDICATOR);
      ui.updating_arc = false;
    }
  }
  if (ui.current_dot) {
    bool show_current = temp_enabled && ctx->has_current;
    climate_set_obj_visible(ui.current_dot, show_current);
    if (show_current && ui.panel) climate_layout_current_dot(ctx, control_modal_calc_layout(ctx->width_compensation_percent));
  }
  if (ui.target_row) climate_set_obj_visible(ui.target_row, true);
  if (ui.target_lbl) {
    if (!ctx->available) lv_label_set_text(ui.target_lbl, "--");
    else if (!temp_enabled) lv_label_set_text(ui.target_lbl, "Off");
    else lv_label_set_text(ui.target_lbl, climate_format_tenths(target, ctx->precision).c_str());
    if (ctx->available && ctx->hvac_mode == "off" && !ctx->hvac_modes.empty()) {
      lv_obj_add_flag(ui.target_lbl, LV_OBJ_FLAG_CLICKABLE);
    } else {
      lv_obj_clear_flag(ui.target_lbl, LV_OBJ_FLAG_CLICKABLE);
    }
  }
  if (ui.unit_lbl) {
    lv_label_set_text(ui.unit_lbl, temp_enabled ? display_temperature_unit_symbol() : "");
    climate_set_obj_visible(ui.unit_lbl, temp_enabled);
  }
  if (ui.status_lbl) {
    if (!temp_enabled) {
      if (!ctx->available) lv_label_set_text(ui.status_lbl, "Unavailable");
      else if (!ctx->configured_label.empty()) lv_label_set_text(ui.status_lbl, ctx->configured_label.c_str());
      else if (!ctx->friendly_name.empty()) lv_label_set_text(ui.status_lbl, ctx->friendly_name.c_str());
      else lv_label_set_text(ui.status_lbl, "Climate");
    } else {
      lv_label_set_text(ui.status_lbl, climate_action_label(ctx).c_str());
    }
  }
  bool dual = temp_enabled && climate_dual_target(ctx);
  if (ui.hint_lbl) {
    lv_label_set_text(ui.hint_lbl, dual ? (ctx->edit_high ? "High target" : "Low target") : "");
    if (dual) lv_obj_clear_flag(ui.hint_lbl, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.hint_lbl, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.low_btn) {
    if (dual) lv_obj_clear_flag(ui.low_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.low_btn, LV_OBJ_FLAG_HIDDEN);
    if (!ctx->edit_high) lv_obj_add_state(ui.low_btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(ui.low_btn, LV_STATE_CHECKED);
  }
  if (ui.high_btn) {
    if (dual) lv_obj_clear_flag(ui.high_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.high_btn, LV_OBJ_FLAG_HIDDEN);
    if (ctx->edit_high) lv_obj_add_state(ui.high_btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(ui.high_btn, LV_STATE_CHECKED);
  }
  climate_set_obj_visible(ui.minus_btn, temp_enabled);
  climate_set_obj_visible(ui.plus_btn, temp_enabled);
  climate_update_chip(ui.fan_chip, "Fan", ctx->fan_mode, !ctx->fan_modes.empty());
  climate_update_chip(ui.swing_chip, "Swing", ctx->swing_mode, !ctx->swing_modes.empty());
  climate_set_obj_visible(ui.chips, temp_enabled);
  climate_update_menu_tile(ui.menu_mode_btn, "Mode", ctx->hvac_mode, !ctx->hvac_modes.empty());
  climate_update_menu_tile(ui.menu_preset_btn, "Preset", ctx->preset_mode, !ctx->preset_modes.empty());
  if (ui.option_list_view) {
    climate_set_obj_visible(ui.menu_mode_btn, false);
    climate_set_obj_visible(ui.menu_preset_btn, false);
  }
  if (ui.action_menu_open) climate_set_dial_controls_visible(false);
}

inline void climate_control_layout_modal(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  if (ui.status_lbl) lv_obj_update_layout(ui.status_lbl);
  if (ui.target_row) lv_obj_update_layout(ui.target_row);
  lv_coord_t title_h = ui.status_lbl ? lv_obj_get_height(ui.status_lbl) : 0;
  lv_coord_t value_h = ui.target_row ? lv_obj_get_height(ui.target_row) : 0;
  lv_coord_t title_center_y = layout.value_center_y -
    (value_h / 2 + layout.title_gap + title_h / 2);
  lv_coord_t chip_h = layout.short_side < 520 ? 48 : 56;

  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout,
    control_modal_card_radius(ctx->btn));
  control_modal_apply_back_button_layout(ui.back_btn, layout);
  lv_obj_set_size(ui.mode_btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(ui.mode_btn, layout.back_size / 2, LV_PART_MAIN);
  lv_obj_align(ui.mode_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  if (ui.menu_close_btn) {
    lv_obj_set_size(ui.menu_close_btn, layout.back_size, layout.back_size);
    lv_obj_set_style_radius(ui.menu_close_btn, layout.back_size / 2, LV_PART_MAIN);
    lv_obj_align(ui.menu_close_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  }
  control_modal_apply_arc_layout(ui.arc, layout, ctx->width_compensation_percent);
  if (ui.current_dot) climate_layout_current_dot(ctx, layout);
  lv_obj_align(ui.status_lbl, LV_ALIGN_CENTER, 0, title_center_y);
  lv_obj_align(ui.target_row, LV_ALIGN_CENTER, 0, layout.value_center_y);
  lv_obj_align(ui.hint_lbl, LV_ALIGN_CENTER, 0, layout.controls_center_y - layout.btn_size / 2 - 50);
  lv_obj_set_style_translate_y(ui.unit_lbl,
    control_modal_scaled_px(MEDIA_VOLUME_UNIT_Y_REF_PX, layout.short_side), LV_PART_MAIN);
  control_modal_apply_step_buttons_layout(ui.minus_btn, ui.plus_btn, layout);
  lv_obj_align(ui.low_btn, LV_ALIGN_CENTER, -46, layout.controls_center_y - layout.btn_size / 2 - 24);
  lv_obj_align(ui.high_btn, LV_ALIGN_CENTER, 46, layout.controls_center_y - layout.btn_size / 2 - 24);
  lv_obj_set_height(ui.chips, chip_h);
  lv_obj_align(ui.chips, LV_ALIGN_BOTTOM_MID, 0, -layout.inset);
  if (ui.menu_view) {
    lv_obj_set_size(ui.menu_view, layout.panel_w, layout.panel_h);
    lv_obj_set_pos(ui.menu_view, 0, 0);
    lv_coord_t menu_gap = control_modal_scaled_px(16, layout.short_side);
    lv_coord_t tile_w = layout.panel_w - layout.inset * 2;
    if (tile_w > 280) tile_w = 280;
    lv_coord_t tile_h = layout.short_side < 520 ? 86 : 94;
    lv_obj_set_size(ui.menu_mode_btn, tile_w, tile_h);
    lv_obj_set_size(ui.menu_preset_btn, tile_w, tile_h);
    apply_width_compensation(ui.menu_mode_btn, ctx->width_compensation_percent);
    apply_width_compensation(ui.menu_preset_btn, ctx->width_compensation_percent);
    lv_obj_align(ui.menu_mode_btn, LV_ALIGN_CENTER, 0, -(tile_h + menu_gap) / 2);
    lv_obj_align(ui.menu_preset_btn, LV_ALIGN_CENTER, 0, (tile_h + menu_gap) / 2);
    if (ui.option_list_view) {
      lv_coord_t list_top = layout.inset + layout.back_size + 8;
      lv_coord_t list_bottom = layout.inset;
      lv_coord_t list_content_h = layout.panel_h - list_top - list_bottom;
      if (list_content_h < 120) list_content_h = 120;
      lv_obj_set_size(ui.option_list_view, layout.panel_w, layout.panel_h);
      lv_obj_set_style_pad_top(ui.option_list_view, list_top, LV_PART_MAIN);
      lv_obj_set_style_pad_left(ui.option_list_view, layout.inset, LV_PART_MAIN);
      lv_obj_set_style_pad_right(ui.option_list_view, layout.inset, LV_PART_MAIN);
      lv_obj_set_style_pad_bottom(ui.option_list_view, list_bottom, LV_PART_MAIN);
      lv_coord_t title_row_h = ctx->label_font ? ctx->label_font->line_height : 28;
      lv_coord_t row_gap = layout.short_side < 520 ? 6 : 8;
      lv_coord_t default_row_h = layout.short_side < 520 ? 54 : 68;
      lv_coord_t min_row_h = title_row_h + 6;
      lv_obj_set_style_pad_row(ui.option_list_view, row_gap, LV_PART_MAIN);

      auto fit_option_rows = [&](lv_obj_t *container, lv_coord_t available_h) {
        uint32_t child_count = lv_obj_get_child_count(container);
        uint32_t clickable_count = 0;
        uint32_t title_count = 0;
        for (uint32_t i = 0; i < child_count; i++) {
          lv_obj_t *child = lv_obj_get_child(container, i);
          if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) clickable_count++;
          else title_count++;
        }
        if (clickable_count == 0) return;
        lv_coord_t gaps_h = row_gap * (child_count > 0 ? child_count - 1 : 0);
        lv_coord_t fixed_h = title_row_h * title_count + gaps_h;
        lv_coord_t fitted_row_h = default_row_h;
        lv_coord_t candidate = available_h > fixed_h
          ? (available_h - fixed_h) / clickable_count
          : min_row_h;
        if (candidate < fitted_row_h) fitted_row_h = candidate;
        if (fitted_row_h < min_row_h) fitted_row_h = min_row_h;
        lv_obj_set_style_pad_row(container, row_gap, LV_PART_MAIN);
        for (uint32_t i = 0; i < child_count; i++) {
          lv_obj_t *child = lv_obj_get_child(container, i);
          if (lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
            lv_obj_set_height(child, fitted_row_h);
            lv_obj_set_style_radius(child, 0, LV_PART_MAIN);
          } else {
            lv_obj_set_height(child, title_row_h);
          }
        }
      };

      uint32_t child_count = lv_obj_get_child_count(ui.option_list_view);
      for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *row = lv_obj_get_child(ui.option_list_view, i);
        if (lv_obj_has_flag(row, LV_OBJ_FLAG_CLICKABLE)) {
          fit_option_rows(ui.option_list_view, list_content_h);
          continue;
        }
        uint32_t row_child_count = lv_obj_get_child_count(row);
        if (row_child_count > 0) {
          lv_obj_set_height(row, list_content_h);
          fit_option_rows(row, list_content_h);
        }
      }
    }
  }
  lv_obj_move_foreground(ui.back_btn);
  lv_obj_move_foreground(ui.mode_btn);
  if (ui.menu_view) lv_obj_move_foreground(ui.menu_view);
  if (ui.menu_close_btn) lv_obj_move_foreground(ui.menu_close_btn);
}

inline void climate_show_action_menu(ClimateControlCtx *ctx) {
  if (!ctx) return;
  climate_open_option_menu(ctx, "hvac");
}

inline void climate_hide_action_menu() {
  climate_hide_option_menu();
  ClimateControlModalUi &ui = climate_control_modal_ui();
  ui.action_menu_open = false;
  climate_hide_inline_option_list();
  climate_set_obj_visible(ui.menu_view, false);
  climate_set_obj_visible(ui.menu_close_btn, false);
  climate_set_dial_controls_visible(true);
  if (ui.active) {
    climate_control_layout_modal(ui.active);
    climate_control_set_modal_value(ui.active);
  }
}

inline void climate_control_hide_modal() {
  climate_hide_option_menu();
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui = ClimateControlModalUi();
}

inline void climate_control_open_modal(ClimateControlCtx *ctx) {
  if (!ctx) return;
  climate_control_hide_modal();
  ClimateControlModalUi &ui = climate_control_modal_ui();
  ui.active = ctx;
  ui.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_overlay(ui.overlay);

  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, ctx->tertiary_color,
    control_modal_card_radius(ctx->btn));

  ui.back_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0141", ctx->icon_font,
    0x454545, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_set_style_bg_opa(ui.back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.back_btn, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(ui.back_btn, [](lv_event_t *) { climate_control_hide_modal(); }, LV_EVENT_CLICKED, nullptr);

  ui.mode_btn = control_modal_create_round_button(ui.panel, 32, find_icon("Dots Horizontal"), ctx->icon_font,
    0x454545, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_set_style_bg_opa(ui.mode_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.mode_btn, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(ui.mode_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_show_action_menu(ui.active);
  }, LV_EVENT_CLICKED, nullptr);

  ui.menu_view = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.menu_view, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.menu_view, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.menu_view, LV_OBJ_FLAG_HIDDEN);

  ui.menu_close_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0156", ctx->icon_font,
    0x454545, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_set_style_bg_opa(ui.menu_close_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.menu_close_btn, 0, LV_PART_MAIN);
  lv_obj_add_flag(ui.menu_close_btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.menu_close_btn, [](lv_event_t *) {
    climate_hide_action_menu();
  }, LV_EVENT_CLICKED, nullptr);

  ui.menu_mode_btn = climate_create_menu_tile(ui.menu_view, find_icon("Fire"), "Mode\nNone",
    ctx->icon_font, ctx->label_font, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.menu_mode_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "hvac");
  }, LV_EVENT_CLICKED, nullptr);
  ui.menu_preset_btn = climate_create_menu_tile(ui.menu_view, find_icon("Air Filter"), "Preset\nNone",
    ctx->icon_font, ctx->label_font, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.menu_preset_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "preset");
  }, LV_EVENT_CLICKED, nullptr);

  ui.current_dot = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.current_dot, lv_color_hex(CLIMATE_NEUTRAL_COLOR), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.current_dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.current_dot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.current_dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.current_dot, LV_OBJ_FLAG_HIDDEN);

  ui.arc = lv_arc_create(ui.panel);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, ctx->min_tenths, ctx->max_tenths);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(ctx->secondary_color), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.arc, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_add_flag(ui.arc, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    ui.dragging_arc = true;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    climate_apply_selected_target(ui.active, lv_arc_get_value(arc), false, false);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    ui.dragging_arc = false;
    climate_apply_selected_target(ui.active, lv_arc_get_value(arc), true, false);
  }, LV_EVENT_RELEASED, nullptr);

  ui.target_row = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.target_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_opa(ui.target_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.target_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.target_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.target_row, 4, LV_PART_MAIN);
  lv_obj_set_layout(ui.target_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.target_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.target_row, LV_FLEX_ALIGN_END, LV_PART_MAIN);
  lv_obj_clear_flag(ui.target_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.target_row, LV_OBJ_FLAG_SCROLLABLE);

  ui.target_lbl = lv_label_create(ui.target_row);
  lv_obj_set_style_text_color(ui.target_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.target_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->number_font) lv_obj_set_style_text_font(ui.target_lbl, ctx->number_font, LV_PART_MAIN);
  apply_width_compensation(ui.target_lbl, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.target_lbl, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (!ui.active || !ui.active->available || ui.active->hvac_mode != "off" ||
        ui.active->hvac_modes.empty()) {
      return;
    }
    climate_show_action_menu(ui.active);
  }, LV_EVENT_CLICKED, nullptr);

  ui.unit_lbl = lv_label_create(ui.target_row);
  lv_obj_set_style_text_color(ui.unit_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.unit_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->unit_font) lv_obj_set_style_text_font(ui.unit_lbl, ctx->unit_font, LV_PART_MAIN);
  lv_obj_set_style_translate_y(ui.unit_lbl, MEDIA_VOLUME_UNIT_Y_REF_PX, LV_PART_MAIN);
  apply_width_compensation(ui.unit_lbl, ctx->width_compensation_percent);

  ui.status_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.status_lbl, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.status_lbl, ctx->label_font, LV_PART_MAIN);

  ui.hint_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.hint_lbl, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.hint_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.hint_lbl, ctx->label_font, LV_PART_MAIN);

  ui.low_btn = climate_create_chip(ui.panel, "Low", ctx->label_font, 0x333333, ctx->width_compensation_percent);
  ui.high_btn = climate_create_chip(ui.panel, "High", ctx->label_font, 0x333333, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.low_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) { ui.active->edit_high = false; climate_control_set_modal_value(ui.active); }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.high_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) { ui.active->edit_high = true; climate_control_set_modal_value(ui.active); }
  }, LV_EVENT_CLICKED, nullptr);

  ui.minus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Minus"), ctx->icon_font,
    CLIMATE_NEUTRAL_COLOR, ctx->tertiary_color, ctx->width_compensation_percent);
  ui.plus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Plus"), ctx->icon_font,
    CLIMATE_NEUTRAL_COLOR, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_apply_selected_target(ui.active,
      climate_selected_target(ui.active) - ui.active->step_tenths, false, true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_apply_selected_target(ui.active,
      climate_selected_target(ui.active) + ui.active->step_tenths, false, true);
  }, LV_EVENT_CLICKED, nullptr);

  ui.chips = lv_obj_create(ui.panel);
  lv_obj_set_width(ui.chips, lv_pct(96));
  lv_obj_set_style_bg_opa(ui.chips, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.chips, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.chips, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.chips, 8, LV_PART_MAIN);
  lv_obj_set_layout(ui.chips, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.chips, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.chips, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.chips, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_clear_flag(ui.chips, LV_OBJ_FLAG_SCROLLABLE);

  ui.fan_chip = climate_create_chip(ui.chips, "Fan\nNone", ctx->label_font, 0x333333, ctx->width_compensation_percent);
  ui.swing_chip = climate_create_chip(ui.chips, "Swing\nNone", ctx->label_font, 0x333333, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.fan_chip, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "fan");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.swing_chip, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "swing");
  }, LV_EVENT_CLICKED, nullptr);

  climate_control_set_modal_value(ctx);
  climate_control_layout_modal(ctx);
  climate_control_set_modal_value(ctx);
  lv_obj_move_foreground(ui.overlay);
}

inline void setup_climate_control_button(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                         lv_obj_t *sensor_container,
                                         lv_obj_t *sensor_lbl,
                                         lv_obj_t *unit_lbl,
                                         lv_obj_t *text_lbl,
                                         const ParsedCfg &p) {
  if (icon_lbl) lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
  if (sensor_container) {
    lv_obj_clear_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_move_foreground(sensor_container);
  }
  if (sensor_lbl) lv_label_set_text(sensor_lbl, "--");
  if (unit_lbl) lv_label_set_text(unit_lbl, "");
  if (text_lbl) {
    lv_label_set_text(text_lbl, p.label.empty() ? "Climate" : p.label.c_str());
    lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    configure_button_label_wrap(text_lbl);
    lv_obj_move_foreground(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline ClimateControlCtx *create_climate_control_context(
    lv_obj_t *btn, lv_obj_t *label_lbl, const ParsedCfg &p,
    uint32_t accent_color, uint32_t secondary_color, uint32_t tertiary_color,
    const lv_font_t *number_font, const lv_font_t *unit_font,
    const lv_font_t *label_font, const lv_font_t *icon_font,
    int width_compensation_percent,
    lv_obj_t *value_lbl, lv_obj_t *unit_lbl) {
  ClimateControlCtx *ctx = new ClimateControlCtx();
  ctx->entity_id = p.entity;
  ctx->configured_label = p.label;
  ctx->precision = parse_precision(p.precision);
  climate_apply_saved_range(ctx, p.precision);
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->btn = btn;
  ctx->label_lbl = label_lbl;
  ctx->value_lbl = value_lbl;
  ctx->unit_lbl = unit_lbl;
  ctx->number_font = number_font;
  ctx->unit_font = unit_font;
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = normalize_width_compensation_percent(width_compensation_percent);
  if (btn) lv_obj_set_user_data(btn, ctx);
  int &count = climate_control_ref_count();
  if (count < MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) climate_control_refs()[count++] = ctx;
  climate_update_card(ctx);
  return ctx;
}

inline void subscribe_climate_control_state(ClimateControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  auto refresh = [ctx]() {
    climate_update_card(ctx);
    climate_control_set_modal_value(ctx);
  };
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef state) {
        ctx->hvac_mode = climate_hvac_service_value(string_ref_limited(state, HA_SHORT_STATE_MAX_LEN));
        ctx->available = !climate_unavailable_value(ctx->hvac_mode);
        if (!ctx->available) ctx->hvac_mode = "off";
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("hvac_action"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        ctx->hvac_action = climate_lower(climate_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        refresh();
      })
  );
  auto subscribe_temp = [ctx, refresh](const char *attr, int ClimateControlCtx::*field, bool ClimateControlCtx::*has_field) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, field, has_field](esphome::StringRef value) {
          int tenths = 0;
          if (climate_parse_tenths(value, tenths)) {
            ctx->*field = climate_clamp_tenths(ctx, tenths);
            ctx->*has_field = true;
          } else {
            ctx->*has_field = false;
          }
          refresh();
        })
    );
  };
  subscribe_temp("current_temperature", &ClimateControlCtx::current_tenths, &ClimateControlCtx::has_current);
  subscribe_temp("temperature", &ClimateControlCtx::target_tenths, &ClimateControlCtx::has_target);
  subscribe_temp("target_temp_low", &ClimateControlCtx::low_tenths, &ClimateControlCtx::has_low);
  subscribe_temp("target_temp_high", &ClimateControlCtx::high_tenths, &ClimateControlCtx::has_high);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("min_temp"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        int tenths = 0;
        if (!ctx->custom_min && climate_parse_tenths(value, tenths)) ctx->min_tenths = tenths;
        if (ctx->max_tenths <= ctx->min_tenths) ctx->max_tenths = ctx->min_tenths + 10;
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("max_temp"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        int tenths = 0;
        if (!ctx->custom_max && climate_parse_tenths(value, tenths)) ctx->max_tenths = tenths;
        if (ctx->max_tenths <= ctx->min_tenths) ctx->max_tenths = ctx->min_tenths + 10;
        refresh();
      })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("target_temp_step"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh](esphome::StringRef value) {
        int tenths = 0;
        if (climate_parse_tenths(value, tenths) && tenths > 0 && tenths <= 100)
          ctx->step_tenths = tenths;
        else
          ctx->step_tenths = CLIMATE_DEFAULT_STEP_TENTHS;
        refresh();
      })
  );
  auto subscribe_text = [ctx, refresh](const char *attr, std::string ClimateControlCtx::*field) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, field](esphome::StringRef value) {
          ctx->*field = climate_lower(climate_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
          refresh();
        })
    );
  };
  subscribe_text("fan_mode", &ClimateControlCtx::fan_mode);
  subscribe_text("swing_mode", &ClimateControlCtx::swing_mode);
  subscribe_text("preset_mode", &ClimateControlCtx::preset_mode);
  auto subscribe_list = [ctx, refresh](const char *attr, std::vector<std::string> ClimateControlCtx::*field) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, field](esphome::StringRef value) {
          ctx->*field = climate_parse_options(value);
          refresh();
        })
    );
  };
  subscribe_list("hvac_modes", &ClimateControlCtx::hvac_modes);
  subscribe_list("fan_modes", &ClimateControlCtx::fan_modes);
  subscribe_list("swing_modes", &ClimateControlCtx::swing_modes);
  subscribe_list("preset_modes", &ClimateControlCtx::preset_modes);
}
