#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Subpage helpers ───────────────────────────────────────────────────

// Button definition parsed from a subpage config (pipe+colon delimited)
struct SubpageBtn {
  std::string entity;
  std::string label;
  std::string icon;
  std::string icon_on;
  std::string sensor;     // sensor entity, cover/internal mode, or action name
  std::string unit;
  std::string type;       // button type: "" (toggle), action, sensor, calendar, timezone, weather_forecast, slider, light_brightness, light_switch, fan_*, cover, garage, lock, alarm, media, push, internal, subpage
  std::string precision;  // decimal places for sensor display; "text" = text sensor mode
  std::string options;    // comma-delimited card options
};

inline std::vector<std::string> split_subpage_fields(const std::string &value, char delim) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= value.length()) {
    size_t end = value.find(delim, start);
    if (end == std::string::npos) end = value.length();
    out.push_back(value.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

inline std::string compact_subpage_type(const std::string &code) {
  if (code == "A") return "action";
  if (code == "D") return "calendar";
  if (code == "T") return "timezone";
  if (code == "S") return "sensor";
  if (code == "W") return "weather";
  if (code == "F") return "weather_forecast";
  if (code == "B") return "fan_switch";
  if (code == "J") return "fan_speed";
  if (code == "O") return "fan_oscillate";
  if (code == "E") return "fan_direction";
  if (code == "Z") return "fan_preset";
  if (code == "V") return "light_brightness";
  if (code == "Q") return "light_switch";
  if (code == "Y") return "alarm";
  if (code == "L") return "slider";
  if (code == "C") return "cover";
  if (code == "N") return "light_temperature";
  if (code == "R") return "garage";
  if (code == "K") return "lock";
  if (code == "M") return "media";
  if (code == "H") return "climate";
  if (code == "P") return "push";
  if (code == "I") return "internal";
  if (code == "G") return "subpage";
  return code;
}

inline std::string decode_compact_subpage_field(const std::string &value) {
  return decode_compact_field(value);
}

inline SubpageBtn normalize_subpage_btn(SubpageBtn b) {
  if (brightness_slider_type(b.type) && !b.sensor.empty()) b.sensor.clear();
  if (fan_card_type(b.type)) {
    b.sensor.clear();
    b.unit.clear();
    b.precision.clear();
    b.options.clear();
    if (b.icon.empty() || b.icon == "Auto") b.icon = fan_card_default_icon_name(b.type);
    if (b.type == "fan_switch") {
      if (b.icon_on.empty() || b.icon_on == "Auto") b.icon_on = "Fan";
    } else {
      b.icon_on.clear();
    }
  }
  if (b.type == "weather_forecast") {
    b.type = "weather";
    b.precision = "tomorrow";
    if (b.label == "Weather") b.label.clear();
  }
  if (b.type == "media") {
    if (b.sensor == "controls") {
      if (b.icon.empty() || b.icon == "Speaker") b.icon = "Auto";
      b.sensor = "play_pause";
    } else if (b.sensor.empty()) {
      b.sensor = "play_pause";
    } else if (b.sensor != "play_pause" && b.sensor != "previous" &&
               b.sensor != "next" && b.sensor != "volume" &&
               b.sensor != "position" && b.sensor != "now_playing") {
      b.sensor = "play_pause";
    }
    if (b.sensor == "previous" && b.label == "Skip Previous") b.label = "Previous";
    if (b.sensor == "next" && b.label == "Skip Next") b.label = "Next";
    if (b.sensor == "volume") {
      if (b.label.empty() || b.label == "Media") b.label = "Volume";
      b.icon = "Auto";
    }
    if (b.sensor == "position" && (b.label.empty() || b.label == "Track")) b.label = "Position";
    if (b.sensor == "now_playing") {
      b.precision = (b.precision == "progress" || b.precision == "play_pause") ? b.precision : "";
    } else if ((b.sensor == "play_pause" || b.sensor == "position") && b.precision == "state") {
      b.precision = "state";
    } else {
      b.precision.clear();
    }
  }
  if (b.type == "climate") {
    b.sensor.clear();
    b.unit.clear();
  }
  if (b.type == "garage") {
    if (b.sensor != "open" && b.sensor != "close") b.sensor.clear();
    b.unit.clear();
    b.precision.clear();
    if (!b.sensor.empty()) b.icon_on.clear();
  }
  if (b.type == "alarm") {
    b.sensor.clear();
    b.unit.clear();
    b.precision.clear();
    b.icon_on.clear();
    if (b.icon.empty() || b.icon == "Auto") b.icon = "Security";
  }
  if (b.type == "light_switch") {
    b.sensor.clear();
    b.unit.clear();
    b.precision.clear();
    b.options.clear();
  }
  ParsedCfg p;
  p.type = b.type;
  p.precision = b.precision;
  if (!b.type.empty() && b.type != "action" && b.type != "alarm" && !fan_card_type(b.type) && !card_large_numbers_supported(p)) {
    b.options.clear();
  }
  return b;
}

inline ParsedCfg parsed_cfg_from_subpage_btn(const SubpageBtn &b) {
  ParsedCfg p;
  p.entity = b.entity;
  p.label = b.label;
  p.icon = b.icon;
  p.icon_on = b.icon_on;
  p.sensor = b.sensor;
  p.unit = b.unit;
  p.type = b.type;
  p.precision = b.precision;
  p.options = b.options;
  return normalize_parsed_cfg(p);
}

inline lv_obj_t *create_grid_card_button(lv_obj_t *parent, lv_coord_t radius,
                                         lv_coord_t pad,
                                         const lv_font_t *label_font,
                                         lv_color_t text_color) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, pad, LV_PART_MAIN);
  if (label_font) lv_obj_set_style_text_font(btn, label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(btn, text_color, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  return btn;
}

inline lv_obj_t *create_card_sensor_container(lv_obj_t *parent,
                                              const lv_font_t *value_font,
                                              const lv_font_t *unit_font,
                                              lv_color_t text_color,
                                              lv_obj_t **value_lbl,
                                              lv_obj_t **unit_lbl) {
  lv_obj_t *container = lv_obj_create(parent);
  lv_obj_set_align(container, LV_ALIGN_TOP_LEFT);
  lv_obj_set_size(container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(container, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(container, 0, LV_PART_MAIN);
  lv_obj_set_layout(container, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(container, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(container, LV_FLEX_ALIGN_END, LV_PART_MAIN);

  lv_obj_t *value = lv_label_create(container);
  if (value_font) lv_obj_set_style_text_font(value, value_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(value, text_color, LV_PART_MAIN);
  lv_label_set_text(value, "--");

  lv_obj_t *unit = lv_label_create(container);
  if (unit_font) lv_obj_set_style_text_font(unit, unit_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(unit, text_color, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(unit, 6, LV_PART_MAIN);
  lv_label_set_text(unit, "");

  if (value_lbl) *value_lbl = value;
  if (unit_lbl) *unit_lbl = unit;
  return container;
}

inline BtnSlot create_dynamic_card_slot(lv_obj_t *btn,
                                        const lv_font_t *icon_font,
                                        const lv_font_t *value_font,
                                        const lv_font_t *label_font,
                                        lv_color_t text_color) {
  BtnSlot slot{};
  slot.config = nullptr;
  slot.btn = btn;
  slot.icon_lbl = lv_label_create(btn);
  if (icon_font) lv_obj_set_style_text_font(slot.icon_lbl, icon_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(slot.icon_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(slot.icon_lbl, "\U000F0493");
  lv_obj_align(slot.icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

  slot.sensor_container = create_card_sensor_container(
    btn, value_font, label_font, text_color, &slot.sensor_lbl, &slot.unit_lbl);

  slot.text_lbl = lv_label_create(btn);
  if (label_font) lv_obj_set_style_text_font(slot.text_lbl, label_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(slot.text_lbl, text_color, LV_PART_MAIN);
  lv_label_set_text(slot.text_lbl, "Configure");
  lv_obj_align(slot.text_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  configure_button_label_wrap(slot.text_lbl);
  return slot;
}

// Parse "order|entity:label:icon:...|entity:label:..." into a vector of SubpageBtns
inline std::vector<SubpageBtn> parse_subpage_config(const std::string &sp_cfg) {
  std::vector<SubpageBtn> btns;
  if (sp_cfg.empty()) return btns;

  bool compact = sp_cfg[0] == '~';
  std::vector<std::string> pipes = split_subpage_fields(compact ? sp_cfg.substr(1) : sp_cfg, '|');
  if (pipes.size() < 2) return btns;

  for (size_t pi = 1; pi < pipes.size(); pi++) {
    if (compact) {
      std::vector<std::string> flds = split_subpage_fields(pipes[pi], ',');
      std::string tp = flds.size() > 0 ? compact_subpage_type(flds[0]) : "";
      std::string e = flds.size() > 1 ? decode_compact_subpage_field(flds[1]) : "";
      std::string l = flds.size() > 2 ? decode_compact_subpage_field(flds[2]) : "";
      std::string ic = flds.size() > 3 ? decode_compact_subpage_field(flds[3]) : "Auto";
      if (ic.empty()) ic = "Auto";
      std::string io = flds.size() > 4 ? decode_compact_subpage_field(flds[4]) : "Auto";
      if (io.empty()) io = "Auto";
      std::string sn = flds.size() > 5 ? decode_compact_subpage_field(flds[5]) : "";
      std::string un = flds.size() > 6 ? decode_compact_subpage_field(flds[6]) : "";
      std::string pr = flds.size() > 7 ? decode_compact_subpage_field(flds[7]) : "";
      std::string op = flds.size() > 8 ? decode_compact_subpage_field(flds[8]) : "";
      btns.push_back(normalize_subpage_btn({e, l, ic, io, sn, un, tp, pr, op}));
      continue;
    }
    std::vector<std::string> flds = split_subpage_fields(pipes[pi], ':');
    std::string e = flds.size() > 0 ? flds[0] : "";
    std::string l = flds.size() > 1 ? flds[1] : "";
    std::string ic = flds.size() > 2 ? flds[2] : "Auto";
    if (ic.empty()) ic = "Auto";
    std::string io = flds.size() > 3 ? flds[3] : "Auto";
    if (io.empty()) io = "Auto";
    std::string sn = flds.size() > 4 ? flds[4] : "";
    std::string un = flds.size() > 5 ? flds[5] : "";
    std::string tp = flds.size() > 6 ? flds[6] : "";
    std::string pr = flds.size() > 7 ? flds[7] : "";
    std::string op = flds.size() > 8 ? flds[8] : "";
    btns.push_back(normalize_subpage_btn({e, l, ic, io, sn, un, tp, pr, op}));
  }
  return btns;
}

// Extract the order string (everything before the first pipe) from subpage config
inline std::string get_subpage_order(const std::string &sp_cfg) {
  if (sp_cfg.empty()) return "";
  size_t start = sp_cfg[0] == '~' ? 1 : 0;
  size_t pe = sp_cfg.find('|', start);
  if (pe == std::string::npos) return sp_cfg.substr(start);
  return sp_cfg.substr(start, pe - start);
}

inline std::string subpage_back_token_base(std::string token) {
  size_t eq = token.find('=');
  if (eq != std::string::npos) token = token.substr(0, eq);
  return token;
}

inline std::string subpage_back_label_from_order_token(const std::string &token) {
  size_t eq = token.find('=');
  if (eq == std::string::npos) return "Back";
  std::string label = decode_compact_subpage_field(token.substr(eq + 1));
  return label.empty() ? "Back" : label;
}

inline std::string get_subpage_back_label(const std::string &order_str) {
  if (order_str.empty()) return "Back";
  size_t st = 0;
  while (st <= order_str.length()) {
    size_t cm = order_str.find(',', st);
    if (cm == std::string::npos) cm = order_str.length();
    if (cm > st) {
      std::string tk = order_str.substr(st, cm - st);
      std::string base = subpage_back_token_base(tk);
      if (base == "B" || base == "Bd" || base == "Bw" || base == "Bb" ||
          base == "Bt" || base == "Bx") {
        return subpage_back_label_from_order_token(tk);
      }
    }
    st = cm + 1;
  }
  return "Back";
}

// Subpage grid layout with support for a back button token ("B")
struct SubpageOrder {
  int positions[MAX_GRID_SLOTS] = {};
  int row_span[MAX_GRID_SLOTS] = {};
  int col_span[MAX_GRID_SLOTS] = {};
  int back_pos = 0;
  int back_row_span = 1;
  int back_col_span = 1;
  bool has_back_token = false;
};

inline void subscribe_subpage_parent_indicator(
    const std::string &entity_id,
    lv_obj_t *parent_btn, lv_obj_t *parent_icon,
    int parent_idx, bool *child_was_on,
    bool has_alt_icon, const char *off_glyph, const char *on_glyph,
    int *sp_on_count) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [parent_btn, parent_icon, parent_idx, child_was_on,
       has_alt_icon, off_glyph, on_glyph, sp_on_count](esphome::StringRef state) {
        bool is_on = is_entity_on_ref(state);
        if (is_on && !*child_was_on) {
          sp_on_count[parent_idx]++;
          *child_was_on = true;
        } else if (!is_on && *child_was_on) {
          sp_on_count[parent_idx]--;
          *child_was_on = false;
        }
        if (sp_on_count[parent_idx] > 0) {
          lv_obj_add_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, on_glyph);
        } else {
          lv_obj_clear_state(parent_btn, LV_STATE_CHECKED);
          if (has_alt_icon) lv_label_set_text(parent_icon, off_glyph);
        }
      })
  );
}

// Parse subpage order CSV; "B"/"Bd"/"Bw"/"Bb"/"Bt"/"Bx" tokens mark the back button position
inline void parse_subpage_order(const std::string &order_str, int num_slots, int num_btns,
                                SubpageOrder &result) {
  int slot_limit = bounded_grid_slots(num_slots);
  int btn_limit = bounded_grid_slots(num_btns);
  for (int i = 0; i < MAX_GRID_SLOTS; i++) {
    result.row_span[i] = 1;
    result.col_span[i] = 1;
  }
  if (order_str.empty()) return;
  size_t gp2 = 0, st2 = 0;
  while (st2 <= order_str.length() && gp2 < (size_t)slot_limit) {
    size_t cm = order_str.find(',', st2);
    if (cm == std::string::npos) cm = order_str.length();
    if (cm > st2) {
      std::string tk = order_str.substr(st2, cm - st2);
      tk = subpage_back_token_base(tk);
      if (tk == "B" || tk == "Bd" || tk == "Bw" || tk == "Bb" || tk == "Bt" || tk == "Bx") {
        result.back_pos = gp2;
        grid_token_spans(tk.length() > 1 ? tk[1] : '\0', result.back_row_span, result.back_col_span);
        result.has_back_token = true;
      } else {
        int row_span = 1, col_span = 1;
        if (!tk.empty() && grid_token_has_span_suffix(tk.back())) {
          grid_token_spans(tk.back(), row_span, col_span);
          tk.pop_back();
        }
        int v = atoi(tk.c_str());
        if (v >= 1 && v <= btn_limit) {
          result.positions[gp2] = v;
          result.row_span[v - 1] = row_span;
          result.col_span[v - 1] = col_span;
        }
      }
    }
    gp2++;
    st2 = cm + 1;
  }
}
