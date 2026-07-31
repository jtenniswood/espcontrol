#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Option select card helpers ───────────────────────────────────────

constexpr int OPTION_SELECT_MAX_OPTIONS = 64;
constexpr size_t OPTION_SELECT_OPTION_MAX_LEN = 96;
constexpr size_t OPTION_SELECT_OPTIONS_TEXT_MAX_LEN =
  OPTION_SELECT_MAX_OPTIONS * (OPTION_SELECT_OPTION_MAX_LEN + 4);

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
  uint32_t secondary_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
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
  if (!ctx) return espcontrol_i18n(std::string("Option"));
  if (!ctx->configured_label.empty()) return ctx->configured_label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  if (!ctx->entity_id.empty()) return ctx->entity_id;
  return espcontrol_i18n(std::string("Option"));
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
                                     bool has_sensor_color, uint32_t sensor_val,
                                     const lv_font_t *value_font = nullptr) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_width(s.sensor_container, lv_pct(100));
  lv_label_set_long_mode(s.sensor_lbl, LV_LABEL_LONG_DOT);
  lv_obj_set_width(s.sensor_lbl, lv_pct(100));
  const lv_font_t *text_value_font = value_font
    ? value_font
    : s.text_lbl ? lv_obj_get_style_text_font(s.text_lbl, LV_PART_MAIN) : nullptr;
  if (text_value_font) lv_obj_set_style_text_font(s.sensor_lbl, text_value_font, LV_PART_MAIN);
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  std::string label = p.label.empty()
    ? (p.entity.empty() ? espcontrol_i18n(std::string("Option")) : p.entity)
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
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
  if (service == nullptr) return;
  ha_send_entity_action(ctx->entity_id, service, "option", option.c_str());

  ctx->current_option = option;
  option_select_apply_card_text(ctx);
}

inline void option_select_hide_modal() {
  OptionSelectModalUi &ui = option_select_modal_ui();
  control_modal_delete_overlay(ControlModalKind::OPTION_SELECT, ui.overlay);
  ui = OptionSelectModalUi();
}

inline void option_select_open_modal(OptionSelectCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ctx->available ||
      !option_select_entity_supported(ctx->entity_id)) {
    return;
  }
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::OPTION_SELECT, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, option_select_hide_modal);
  OptionSelectModalUi &ui = option_select_modal_ui();
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

  std::string title = option_select_label(ctx);
  ui.title_lbl = control_modal_create_title(
    ui.panel, title, content_w - layout.back_size - gap,
    ctx->label_font, ctx->width_compensation_percent);
  lv_obj_align(ui.title_lbl, LV_ALIGN_TOP_MID, 0, title_y - layout.back_size / 2);

  ui.list = control_modal_create_scroll_list(ui.panel, content_w, list_h, gap);
  lv_obj_align(ui.list, LV_ALIGN_TOP_LEFT, layout.inset, list_y);

  int count = ctx->options.size() > OPTION_SELECT_MAX_OPTIONS
    ? OPTION_SELECT_MAX_OPTIONS
    : static_cast<int>(ctx->options.size());
  for (int i = 0; i < count; i++) {
    bool active = ctx->options[i] == ctx->current_option;
    lv_obj_t *btn = control_modal_create_list_row(
      ui.list, ctx->options[i], active, row_h, row_radius,
      ctx->accent_color, SECONDARY_GREY,
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
    lv_label_set_text(ui.empty_lbl, espcontrol_i18n("No options"));
    lv_label_set_long_mode(ui.empty_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(ui.empty_lbl, lv_pct(100));
    lv_obj_set_style_text_color(ui.empty_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.empty_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.empty_lbl, ctx->label_font, LV_PART_MAIN);
  }

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
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      std::string state_text = string_ref_limited(state, HA_STATE_TEXT_MAX_LEN);
      bool unavailable = ha_state_unavailable_ref(state);
      ctx->available = !unavailable;
      ctx->current_option = unavailable ? "" : state_text;
      option_select_apply_card_text(ctx);
      OptionSelectModalUi &ui = option_select_modal_ui();
      if (ui.active == ctx && !ctx->available) option_select_hide_modal();
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("options"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef options) {
      ctx->options = option_select_parse_options(
        string_ref_limited(options, OPTION_SELECT_OPTIONS_TEXT_MAX_LEN));
    })
  );
}

inline void subscribe_option_select_friendly_name(OptionSelectCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ctx->configured_label.empty()) return;
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef name) {
      ctx->friendly_name = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
      option_select_apply_card_text(ctx);
    })
  );
}
