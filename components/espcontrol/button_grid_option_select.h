#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Option select card helpers ───────────────────────────────────────

constexpr int OPTION_SELECT_MAX_OPTIONS = 32;
constexpr size_t OPTION_SELECT_OPTION_MAX_LEN = 96;
constexpr size_t OPTION_SELECT_OPTIONS_TEXT_MAX_LEN = 1024;

struct OptionSelectCtx {
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  std::string current_option;
  std::vector<std::string> options;
  bool available = true;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
  int width_compensation_percent = 100;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
};

struct OptionSelectOptionClick {
  OptionSelectCtx *ctx = nullptr;
  std::string value;
};

struct OptionSelectModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *list = nullptr;
  lv_obj_t *empty_lbl = nullptr;
  OptionSelectCtx *active = nullptr;
  OptionSelectOptionClick option_clicks[OPTION_SELECT_MAX_OPTIONS];
};

inline OptionSelectModalUi &option_select_modal_ui() {
  static OptionSelectModalUi ui;
  return ui;
}

inline bool option_select_entity_supported(const std::string &entity_id) {
  return entity_id.compare(0, 7, "select.") == 0 ||
         entity_id.compare(0, 13, "input_select.") == 0;
}

inline const char *option_select_service_for_entity(const std::string &entity_id) {
  if (entity_id.compare(0, 13, "input_select.") == 0) return "input_select.select_option";
  if (entity_id.compare(0, 7, "select.") == 0) return "select.select_option";
  return nullptr;
}

inline std::string option_select_label(OptionSelectCtx *ctx) {
  if (!ctx) return "Option";
  if (!ctx->configured_label.empty()) return ctx->configured_label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  if (!ctx->entity_id.empty()) return ctx->entity_id;
  return "Option";
}

inline std::string option_select_display_value(const std::string &value) {
  if (value.empty() || value == "unknown" || value == "unavailable") return "--";
  return value;
}

inline void option_select_apply_card_text(OptionSelectCtx *ctx) {
  if (!ctx) return;
  if (ctx->value_lbl) {
    std::string text = option_select_display_value(ctx->current_option);
    lv_label_set_text(ctx->value_lbl, text.c_str());
  }
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
  if (ctx->label_lbl) {
    std::string label = option_select_label(ctx);
    lv_label_set_text(ctx->label_lbl, label.c_str());
  }
}

inline void setup_option_select_card(BtnSlot &s, const ParsedCfg &p,
                                     bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_width(s.sensor_container, lv_pct(100));
  lv_label_set_long_mode(s.sensor_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(s.sensor_lbl, lv_pct(100));
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, p.label.empty()
    ? (p.entity.empty() ? "Option" : p.entity.c_str())
    : p.label.c_str());
  apply_push_button_transition(s.btn);
}

inline std::string option_select_trim(const std::string &value) {
  return trim_display_unit(value);
}

inline void option_select_add_option(std::vector<std::string> &out,
                                     const std::string &raw) {
  std::string value = option_select_trim(raw);
  while (!value.empty() && (value.front() == '\'' || value.front() == '"' ||
                            value.front() == '[' || value.front() == '(' ||
                            value.front() == '<')) {
    value.erase(value.begin());
    value = option_select_trim(value);
  }
  while (!value.empty() && (value.back() == '\'' || value.back() == '"' ||
                            value.back() == ']' || value.back() == ')' ||
                            value.back() == '>')) {
    value.pop_back();
    value = option_select_trim(value);
  }
  if (value.empty()) return;
  if (value.size() > OPTION_SELECT_OPTION_MAX_LEN) value.resize(OPTION_SELECT_OPTION_MAX_LEN);
  for (const auto &existing : out) {
    if (existing == value) return;
  }
  if (out.size() < OPTION_SELECT_MAX_OPTIONS) out.push_back(value);
}

inline std::vector<std::string> option_select_parse_delimited_options(
    const std::string &body, char delim) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= body.length()) {
    size_t end = body.find(delim, start);
    if (end == std::string::npos) end = body.length();
    option_select_add_option(out, body.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

inline std::vector<std::string> option_select_parse_list_options(const std::string &text) {
  std::vector<std::string> out;
  if (text.size() < 2) return out;
  std::string body = text.substr(1, text.size() - 2);
  std::string token;
  bool in_quote = false;
  char quote = '\0';
  bool escaping = false;

  for (char ch : body) {
    if (in_quote) {
      if (escaping) {
        token.push_back(ch);
        escaping = false;
      } else if (ch == '\\') {
        escaping = true;
      } else if (ch == quote) {
        in_quote = false;
      } else {
        token.push_back(ch);
      }
      continue;
    }
    if (ch == '\'' || ch == '"') {
      in_quote = true;
      quote = ch;
      continue;
    }
    if (ch == ',') {
      option_select_add_option(out, token);
      token.clear();
      continue;
    }
    token.push_back(ch);
  }
  option_select_add_option(out, token);
  return out;
}

inline std::vector<std::string> option_select_parse_options(const std::string &raw) {
  std::string text = option_select_trim(raw);
  if (text.empty() || text == "unknown" || text == "unavailable") return {};
  if ((text.front() == '[' && text.back() == ']') ||
      (text.front() == '(' && text.back() == ')')) {
    std::vector<std::string> parsed = option_select_parse_list_options(text);
    if (!parsed.empty()) return parsed;
  }
  if (text.find('\n') != std::string::npos) return option_select_parse_delimited_options(text, '\n');
  if (text.find('|') != std::string::npos) return option_select_parse_delimited_options(text, '|');
  if (text.find(',') != std::string::npos) return option_select_parse_delimited_options(text, ',');
  std::vector<std::string> out;
  option_select_add_option(out, text);
  return out;
}

inline void send_option_select_action(OptionSelectCtx *ctx, const std::string &option) {
  if (!ctx || ctx->entity_id.empty() || option.empty()) return;
  const char *service = option_select_service_for_entity(ctx->entity_id);
  if (service == nullptr || esphome::api::global_api_server == nullptr) return;

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(2);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(ctx->entity_id.c_str());
  auto &option_kv = req.data.emplace_back();
  option_kv.key = decltype(option_kv.key)("option");
  option_kv.value = decltype(option_kv.value)(option.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);

  ctx->current_option = option;
  option_select_apply_card_text(ctx);
}

inline void option_select_hide_modal() {
  OptionSelectModalUi &ui = option_select_modal_ui();
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui = OptionSelectModalUi();
}

inline lv_obj_t *option_select_create_option_button(lv_obj_t *parent,
                                                    const std::string &label,
                                                    bool active,
                                                    lv_coord_t height,
                                                    lv_coord_t radius,
                                                    uint32_t active_color,
                                                    uint32_t inactive_color,
                                                    const lv_font_t *font,
                                                    int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_width(btn, lv_pct(100));
  lv_obj_set_height(btn, height);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(active ? active_color : inactive_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *value = lv_label_create(btn);
  lv_label_set_text(value, label.c_str());
  lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
  lv_obj_set_width(value, lv_pct(100));
  lv_obj_set_style_text_color(value, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(value, font, LV_PART_MAIN);
  apply_width_compensation(value, width_compensation_percent);
  lv_obj_center(value);
  return btn;
}

inline void option_select_open_modal(OptionSelectCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ctx->available ||
      !option_select_entity_supported(ctx->entity_id)) {
    return;
  }
  media_volume_hide_modal();
  climate_control_hide_modal();
  switch_confirmation_hide_modal();
  option_select_hide_modal();

  OptionSelectModalUi &ui = option_select_modal_ui();
  ui.active = ctx;

  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  lv_coord_t radius = control_modal_card_radius(ctx->btn);
  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 120) content_w = layout.panel_w;
  lv_coord_t gap = control_modal_scaled_px(12, layout.short_side);
  if (gap < 8) gap = 8;
  lv_coord_t row_h = control_modal_scaled_px(48, layout.short_side);
  if (row_h < 34) row_h = 34;
  lv_coord_t row_radius = row_h / 3;
  lv_coord_t title_y = layout.inset + layout.back_size / 2;
  lv_coord_t list_y = layout.inset + layout.back_size + gap;
  lv_coord_t list_h = layout.panel_h - list_y - layout.inset;
  if (list_h < row_h) list_h = row_h;

  ui.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_overlay(ui.overlay);

  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, ctx->tertiary_color, radius);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, radius);

  ui.close_btn = control_modal_create_round_button(
    ui.panel, 32, "\U000F0156", ctx->icon_font,
    0x454545, ctx->tertiary_color);
  lv_obj_set_style_bg_opa(ui.close_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.close_btn, 0, LV_PART_MAIN);
  lv_obj_set_size(ui.close_btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(ui.close_btn, layout.back_size / 2, LV_PART_MAIN);
  lv_obj_align(ui.close_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);

  ui.title_lbl = lv_label_create(ui.panel);
  std::string title = option_select_label(ctx);
  lv_label_set_text(ui.title_lbl, title.c_str());
  lv_label_set_long_mode(ui.title_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(ui.title_lbl, content_w - layout.back_size - gap);
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.title_lbl, ctx->label_font, LV_PART_MAIN);
  apply_width_compensation(ui.title_lbl, ctx->width_compensation_percent);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, title_y - layout.back_size / 2);

  ui.list = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.list, content_w, list_h);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset, list_y);
  lv_obj_set_style_bg_opa(ui.list, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.list, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.list, gap, LV_PART_MAIN);
  lv_obj_set_layout(ui.list, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.list, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_scroll_dir(ui.list, LV_DIR_VER);

  int count = ctx->options.size() > OPTION_SELECT_MAX_OPTIONS
    ? OPTION_SELECT_MAX_OPTIONS
    : static_cast<int>(ctx->options.size());
  for (int i = 0; i < count; i++) {
    bool active = ctx->options[i] == ctx->current_option;
    lv_obj_t *btn = option_select_create_option_button(
      ui.list, ctx->options[i], active, row_h, row_radius,
      ctx->accent_color, ctx->secondary_color,
      ctx->label_font, ctx->width_compensation_percent);
    ui.option_clicks[i].ctx = ctx;
    ui.option_clicks[i].value = ctx->options[i];
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      OptionSelectOptionClick *click = (OptionSelectOptionClick *)lv_event_get_user_data(e);
      if (!click || !click->ctx) return;
      send_option_select_action(click->ctx, click->value);
      option_select_hide_modal();
    }, LV_EVENT_CLICKED, &ui.option_clicks[i]);
  }

  if (count == 0) {
    ui.empty_lbl = lv_label_create(ui.list);
    lv_label_set_text(ui.empty_lbl, "No options");
    lv_label_set_long_mode(ui.empty_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui.empty_lbl, lv_pct(100));
    lv_obj_set_style_text_color(ui.empty_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.empty_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.empty_lbl, ctx->label_font, LV_PART_MAIN);
  }

  lv_obj_add_event_cb(ui.close_btn, [](lv_event_t *) {
    option_select_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);

  lv_obj_move_foreground(ui.overlay);
}

inline OptionSelectCtx *create_option_select_context(
    BtnSlot &s, const ParsedCfg &p,
    uint32_t accent_color, uint32_t secondary_color, uint32_t tertiary_color,
    int width_compensation_percent) {
  OptionSelectCtx *ctx = new OptionSelectCtx();
  ctx->entity_id = p.entity;
  ctx->configured_label = p.label;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->value_lbl = s.sensor_lbl;
  ctx->unit_lbl = s.unit_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->width_compensation_percent = width_compensation_percent;
  ctx->value_font = s.sensor_lbl ? lv_obj_get_style_text_font(s.sensor_lbl, LV_PART_MAIN) : nullptr;
  ctx->label_font = s.text_lbl ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr;
  ctx->icon_font = s.icon_lbl ? lv_obj_get_style_text_font(s.icon_lbl, LV_PART_MAIN) : nullptr;
  lv_obj_set_user_data(s.btn, ctx);
  option_select_apply_card_text(ctx);
  return ctx;
}

inline void subscribe_option_select_state(OptionSelectCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || esphome::api::global_api_server == nullptr) return;
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, {},
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      std::string state_text = string_ref_limited(state, HA_STATE_TEXT_MAX_LEN);
      bool unavailable = ha_state_unavailable_ref(state);
      ctx->available = !unavailable;
      ctx->current_option = unavailable ? "" : state_text;
      apply_control_availability(ctx->btn, ctx->btn, ctx->available);
      option_select_apply_card_text(ctx);
      OptionSelectModalUi &ui = option_select_modal_ui();
      if (ui.active == ctx && !ctx->available) option_select_hide_modal();
    })
  );
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("options"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef options) {
      ctx->options = option_select_parse_options(
        string_ref_limited(options, OPTION_SELECT_OPTIONS_TEXT_MAX_LEN));
    })
  );
}

inline void subscribe_option_select_friendly_name(OptionSelectCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ctx->configured_label.empty() ||
      esphome::api::global_api_server == nullptr) {
    return;
  }
  esphome::api::global_api_server->subscribe_home_assistant_state(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      option_select_apply_card_text(ctx);
    })
  );
}
