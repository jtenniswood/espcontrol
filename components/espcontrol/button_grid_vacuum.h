#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

struct VacuumCardCtx {
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *text_lbl = nullptr;
  std::string entity_id;
  std::string mode;
  std::string state;
  std::string label;
  std::string friendly_name;
  std::string options;
  std::string area_id;
  std::string fan_speed;
  std::vector<std::string> fan_speeds;
  bool status_card = false;
  bool available = true;
  bool custom_label = false;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
};

struct VacuumRoomEntry {
  std::string label;
  std::string area_id;
  bool selected = false;
};

enum class VacuumControlTab : uint8_t {
  CONTROLS = 0,
  ROOMS = 1,
  FAN = 2,
};

struct VacuumControlVisibleTabs {
  VacuumControlTab tabs[3] = {
    VacuumControlTab::CONTROLS,
    VacuumControlTab::ROOMS,
    VacuumControlTab::FAN,
  };
  uint8_t count = 0;

  bool contains(VacuumControlTab tab) const {
    for (uint8_t i = 0; i < count; i++) {
      if (tabs[i] == tab) return true;
    }
    return false;
  }

  void add(VacuumControlTab tab) {
    if (count >= 3 || contains(tab)) return;
    tabs[count++] = tab;
  }
};

struct VacuumControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *controls_tab = nullptr;
  lv_obj_t *rooms_tab = nullptr;
  lv_obj_t *fan_tab = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *status_lbl = nullptr;
  lv_obj_t *controls_box = nullptr;
  lv_obj_t *rooms_box = nullptr;
  lv_obj_t *fan_box = nullptr;
  lv_obj_t *room_summary_lbl = nullptr;
  lv_obj_t *room_clean_btn = nullptr;
  lv_obj_t *fan_list = nullptr;
  lv_obj_t *room_overlay = nullptr;
  lv_obj_t *room_list = nullptr;
  lv_obj_t *room_clean_nested_btn = nullptr;
  VacuumCardCtx *active = nullptr;
  VacuumControlTab tab = VacuumControlTab::CONTROLS;
  std::vector<VacuumRoomEntry> rooms;
};

inline VacuumControlModalUi &vacuum_control_modal_ui() {
  static VacuumControlModalUi ui;
  return ui;
}

struct VacuumCardTextRef {
  lv_obj_t *text_lbl = nullptr;
  VacuumCardCtx *ctx = nullptr;
  ParsedCfg cfg;
};

inline std::vector<VacuumCardTextRef> &subpage_vacuum_card_text_refs() {
  static std::vector<VacuumCardTextRef> refs;
  return refs;
}

inline void clear_subpage_vacuum_card_text_refs() {
  subpage_vacuum_card_text_refs().clear();
}

inline std::string vacuum_card_mode(const std::string &mode) {
  return card_runtime_vacuum_mode(mode);
}

inline bool vacuum_card_mode_needs_state(const std::string &mode) {
  return card_runtime_vacuum_state_mode(mode);
}

inline bool vacuum_card_modal_mode(const std::string &mode) {
  return vacuum_card_mode(mode) == "modal";
}

inline bool vacuum_card_read_only(const ParsedCfg &p) {
  return p.type == "vacuum" && vacuum_card_mode(p.sensor) == "status";
}

inline const char *vacuum_card_default_icon_name(const std::string &mode) {
  return card_runtime_vacuum_default_icon_name(mode);
}

inline const char *vacuum_card_mode_label(const std::string &mode) {
  std::string normalized = vacuum_card_mode(mode);
  if (normalized == "status") return "Vacuum";
  if (normalized == "dock") return "Dock";
  if (normalized == "pause_resume") return "Pause";
  if (normalized == "clean_spot") return "Spot Clean";
  if (normalized == "locate") return "Locate";
  if (normalized == "clean_area") return "Clean Area";
  if (normalized == "modal") return "Vacuum";
  return "Start";
}

inline const char *vacuum_state_icon_name(const std::string &state) {
  if (state == "error") return "Robot Vacuum Alert";
  if (state == "unavailable" || state == "unknown") return "Robot Vacuum Off";
  if (state == "docked" || state == "returning") return "Robot Vacuum Variant";
  return "Robot Vacuum";
}

inline std::string vacuum_state_label(const std::string &state,
                                      const std::string &fallback) {
  if (state == "cleaning") return espcontrol_i18n(std::string("Cleaning"));
  if (state == "docked") return espcontrol_i18n(std::string("Docked"));
  if (state == "error") return espcontrol_i18n(std::string("Error"));
  if (state == "idle") return espcontrol_i18n(std::string("Idle"));
  if (state == "paused") return espcontrol_i18n(std::string("Paused"));
  if (state == "returning") return espcontrol_i18n(std::string("Returning"));
  if (state == "unavailable") return espcontrol_i18n(std::string("Unavailable"));
  if (state == "unknown") return espcontrol_i18n(std::string("Unknown"));
  return fallback;
}

inline const char *vacuum_card_icon(const ParsedCfg &p) {
  return (!p.icon.empty() && p.icon != "Auto")
    ? find_icon(p.icon.c_str())
    : find_icon(vacuum_card_default_icon_name(p.sensor));
}

inline void setup_vacuum_card(BtnSlot &s, const ParsedCfg &p) {
  std::string label = p.label.empty()
    ? espcontrol_i18n(std::string(vacuum_card_mode_label(p.sensor)))
    : p.label;
  lv_label_set_text(s.text_lbl, label.c_str());
  lv_label_set_text(s.icon_lbl, vacuum_card_icon(p));
  if (vacuum_card_read_only(p)) {
    lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    return;
  }
  apply_push_button_transition(s.btn);
}

inline VacuumCardCtx *create_vacuum_card_context(const BtnSlot &s,
                                                 const ParsedCfg &p,
                                                 const lv_font_t *label_font = nullptr,
                                                 const lv_font_t *icon_font = nullptr,
                                                 int width_compensation_percent = 100) {
  VacuumCardCtx *ctx = new VacuumCardCtx();
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->text_lbl = s.text_lbl;
  ctx->entity_id = p.entity;
  ctx->mode = vacuum_card_mode(p.sensor);
  ctx->custom_label = !p.label.empty();
  ctx->label = ctx->custom_label
    ? p.label
    : espcontrol_i18n(std::string(vacuum_card_mode_label(ctx->mode)));
  ctx->options = p.options;
  ctx->area_id = p.unit;
  ctx->status_card = ctx->mode == "status";
  ctx->label_font = label_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  return ctx;
}

inline std::string vacuum_control_title(VacuumCardCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Vacuum"));
  if (ctx->custom_label && !ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  if (!ctx->label.empty()) return ctx->label;
  return espcontrol_i18n(std::string("Vacuum"));
}

inline bool vacuum_control_tab_from_token(const std::string &value,
                                          VacuumControlTab &tab) {
  if (value == "controls") {
    tab = VacuumControlTab::CONTROLS;
    return true;
  }
  if (value == "rooms") {
    tab = VacuumControlTab::ROOMS;
    return true;
  }
  if (value == "fan") {
    tab = VacuumControlTab::FAN;
    return true;
  }
  return false;
}

inline VacuumControlVisibleTabs vacuum_control_visible_tabs(VacuumCardCtx *ctx) {
  VacuumControlVisibleTabs visible;
  std::string value = cfg_option_value(ctx ? ctx->options : "", VACUUM_CONTROL_TABS_OPTION);
  if (value.empty()) value = VACUUM_CONTROL_DEFAULT_TABS_VALUE;

  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find('|', start);
    std::string token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
    VacuumControlTab tab = VacuumControlTab::CONTROLS;
    if (vacuum_control_tab_from_token(token, tab)) visible.add(tab);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (visible.count == 0) visible.add(VacuumControlTab::CONTROLS);
  return visible;
}

inline bool vacuum_control_tab_visible(VacuumCardCtx *ctx, VacuumControlTab tab) {
  VacuumControlVisibleTabs tabs = vacuum_control_visible_tabs(ctx);
  return tabs.contains(tab);
}

inline VacuumControlTab vacuum_control_first_visible_tab(VacuumCardCtx *ctx) {
  VacuumControlVisibleTabs tabs = vacuum_control_visible_tabs(ctx);
  return tabs.count == 0 ? VacuumControlTab::CONTROLS : tabs.tabs[0];
}

inline VacuumControlTab vacuum_control_first_available_tab(VacuumCardCtx *ctx) {
  VacuumControlVisibleTabs tabs = vacuum_control_visible_tabs(ctx);
  bool fan_available = ctx && !ctx->fan_speeds.empty();
  for (uint8_t i = 0; i < tabs.count; i++) {
    if (tabs.tabs[i] == VacuumControlTab::FAN && !fan_available) continue;
    return tabs.tabs[i];
  }
  return VacuumControlTab::CONTROLS;
}

inline lv_obj_t *vacuum_control_tab_button(VacuumControlModalUi &ui,
                                           VacuumControlTab tab) {
  switch (tab) {
    case VacuumControlTab::CONTROLS: return ui.controls_tab;
    case VacuumControlTab::ROOMS: return ui.rooms_tab;
    case VacuumControlTab::FAN: return ui.fan_tab;
  }
  return nullptr;
}

inline std::vector<std::string> vacuum_parse_list_attribute(const std::string &raw) {
  std::vector<std::string> out;
  std::string token;
  bool in_quote = false;
  char quote = '\0';
  for (char ch : raw) {
    if ((ch == '\'' || ch == '"') && (!in_quote || quote == ch)) {
      in_quote = !in_quote;
      quote = in_quote ? ch : '\0';
      continue;
    }
    if (!in_quote && (ch == '[' || ch == ']' || ch == ',' || ch == '|')) {
      std::string value = vacuum_trim_copy(token);
      if (!value.empty() && std::find(out.begin(), out.end(), value) == out.end()) {
        out.push_back(value);
      }
      token.clear();
      continue;
    }
    token.push_back(ch);
  }
  std::string value = vacuum_trim_copy(token);
  if (!value.empty() && std::find(out.begin(), out.end(), value) == out.end()) {
    out.push_back(value);
  }
  return out;
}

inline std::vector<VacuumRoomEntry> vacuum_parse_rooms(const std::string &options,
                                                       const std::string &fallback_area) {
  std::vector<VacuumRoomEntry> rooms;
  std::string raw = cfg_option_value(options, VACUUM_ROOMS_OPTION);
  size_t start = 0;
  while (start <= raw.size()) {
    size_t end = raw.find('\n', start);
    std::string row = vacuum_trim_copy(
      raw.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (!row.empty()) {
      size_t sep = row.find('=');
      if (sep != std::string::npos && sep > 0 && sep < row.size() - 1) {
        std::string label = vacuum_trim_copy(row.substr(0, sep));
        std::string area = vacuum_trim_copy(row.substr(sep + 1));
        if (!label.empty() && !area.empty()) rooms.push_back({label, area, false});
      }
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (rooms.empty() && !fallback_area.empty()) {
    rooms.push_back({espcontrol_i18n(std::string("Clean Area")), fallback_area, false});
  }
  return rooms;
}

inline int vacuum_selected_room_count() {
  int count = 0;
  for (const auto &room : vacuum_control_modal_ui().rooms) {
    if (room.selected) count++;
  }
  return count;
}

inline std::string vacuum_selected_room_summary() {
  int count = vacuum_selected_room_count();
  if (count <= 0) return espcontrol_i18n(std::string("No rooms selected"));
  if (count == 1) {
    for (const auto &room : vacuum_control_modal_ui().rooms) {
      if (room.selected) return room.label;
    }
  }
  return std::to_string(count) + " " + espcontrol_i18n(std::string("Rooms"));
}

inline void vacuum_control_update_room_controls() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  int count = vacuum_selected_room_count();
  if (ui.room_summary_lbl) {
    std::string summary = vacuum_selected_room_summary();
    lv_label_set_text(ui.room_summary_lbl, summary.c_str());
  }
  if (ui.room_clean_btn) {
    if (count > 0 && ui.active && ui.active->available) lv_obj_clear_state(ui.room_clean_btn, LV_STATE_DISABLED);
    else lv_obj_add_state(ui.room_clean_btn, LV_STATE_DISABLED);
  }
  if (ui.room_clean_nested_btn) {
    if (count > 0 && ui.active && ui.active->available) lv_obj_clear_state(ui.room_clean_nested_btn, LV_STATE_DISABLED);
    else lv_obj_add_state(ui.room_clean_nested_btn, LV_STATE_DISABLED);
  }
}

inline void vacuum_send_simple_action(VacuumCardCtx *ctx, const char *service,
                                      const char *data_key = nullptr,
                                      const char *data_value = nullptr) {
  if (!ctx || ctx->entity_id.empty() || !service || !ctx->available) return;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, data_key && data_value ? 2 : 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  if (data_key && data_value) ha_action_add_data(req, data_key, data_value);
  ha_action_send(req);
}

inline void vacuum_send_selected_rooms(VacuumCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !ctx->available) return;
  std::vector<std::string> selected;
  for (const auto &room : vacuum_control_modal_ui().rooms) {
    if (room.selected && !room.area_id.empty()) selected.push_back(room.area_id);
  }
  if (selected.empty()) return;

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, "vacuum.clean_area", false, selected.size() == 1 ? 2 : 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  if (selected.size() == 1) {
    ha_action_add_data(req, "cleaning_area_id", selected[0].c_str());
  } else {
    std::string rooms;
    for (const auto &area : selected) {
      if (!rooms.empty()) rooms += "|";
      rooms += area;
    }
    req.variables.init(1);
    req.data_template.init(1);
    ha_action_add_variable(req, "rooms", rooms.c_str());
    ha_action_add_data_template(req, "cleaning_area_id", "{{ rooms.split('|') }}");
  }
  ha_action_send(req);
}

inline void vacuum_control_style_tab(lv_obj_t *btn, bool active) {
  if (!btn) return;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? DEFAULT_TERTIARY_COLOR : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline void vacuum_control_apply_tab_visibility();
inline void vacuum_control_layout_modal(VacuumCardCtx *ctx);

inline lv_obj_t *vacuum_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                  const lv_font_t *font,
                                                  VacuumControlTab tab,
                                                  int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_bg_color(btn, lv_color_hex(DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
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
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_center(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    VacuumControlTab tab = static_cast<VacuumControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    VacuumControlModalUi &ui = vacuum_control_modal_ui();
    ui.tab = tab;
    vacuum_control_apply_tab_visibility();
    vacuum_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *vacuum_control_create_action_row(lv_obj_t *parent,
                                                  const std::string &label,
                                                  const char *icon,
                                                  const lv_font_t *font,
                                                  const char *service) {
  lv_obj_t *btn = control_modal_create_list_row(
    parent, label, false, 56, 16, DARK_CONTROL_NEUTRAL,
    DARK_BACKGROUND_SECONDARY, font, 100);
  if (!btn) return nullptr;
  lv_obj_t *value = lv_obj_get_child(btn, 0);
  if (value && icon) {
    std::string text = std::string(icon) + "  " + label;
    lv_label_set_text(value, text.c_str());
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    VacuumCardCtx *ctx = vacuum_control_modal_ui().active;
    const char *service = static_cast<const char *>(lv_event_get_user_data(e));
    vacuum_send_simple_action(ctx, service);
  }, LV_EVENT_CLICKED, const_cast<char *>(service));
  return btn;
}

inline void vacuum_control_hide_room_picker() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  lv_obj_t *overlay = ui.room_overlay;
  ui.room_overlay = nullptr;
  ui.room_list = nullptr;
  ui.room_clean_nested_btn = nullptr;
  control_modal_delete_nested_overlay(overlay);
}

inline void vacuum_control_render_room_list();

inline void vacuum_control_open_room_picker() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  if (!ui.active) return;
  ControlModalLayout layout = control_modal_calc_layout(ui.active->width_compensation_percent);
  lv_coord_t width = layout.panel_w - layout.inset * 3;
  if (width < 220) width = layout.panel_w - layout.inset;
  ControlModalNestedShell shell = control_modal_open_nested_menu(
    width, control_modal_card_radius(ui.active->btn), vacuum_control_hide_room_picker);
  ui.room_overlay = shell.overlay;
  if (!shell.panel) return;

  lv_obj_t *title = control_modal_create_title(
    shell.panel, espcontrol_i18n(std::string("Rooms")), width - 24,
    ui.active->label_font, ui.active->width_compensation_percent);
  if (title) lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  ui.room_list = control_modal_create_scroll_list(
    shell.panel, width - 24, layout.short_side / 2, 8);
  ui.room_clean_nested_btn = control_modal_create_text_button(
    shell.panel, espcontrol_i18n(std::string("Clean")),
    width - 24, width - 24, 52, 16, DEFAULT_SLIDER_COLOR, ui.active->label_font);
  if (ui.room_clean_nested_btn) {
    lv_obj_add_event_cb(ui.room_clean_nested_btn, [](lv_event_t *) {
      vacuum_send_selected_rooms(vacuum_control_modal_ui().active);
      control_modal_close_nested_menu();
    }, LV_EVENT_CLICKED, nullptr);
  }
  vacuum_control_render_room_list();
  vacuum_control_update_room_controls();
}

inline void vacuum_control_render_room_list() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  if (!ui.room_list || !ui.active) return;
  lv_obj_clean(ui.room_list);
  if (ui.rooms.empty()) {
    lv_obj_t *empty = lv_label_create(ui.room_list);
    if (empty) {
      lv_label_set_text(empty, espcontrol_i18n(std::string("No rooms configured")).c_str());
      lv_obj_set_style_text_color(empty, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
      if (ui.active->label_font) lv_obj_set_style_text_font(empty, ui.active->label_font, LV_PART_MAIN);
    }
    return;
  }
  for (size_t i = 0; i < ui.rooms.size(); i++) {
    lv_obj_t *row = control_modal_create_list_row(
      ui.room_list, ui.rooms[i].label, ui.rooms[i].selected, 52, 14,
      DEFAULT_SLIDER_COLOR, DARK_BACKGROUND_TERTIARY, ui.active->label_font,
      ui.active->width_compensation_percent);
    if (!row) continue;
    lv_obj_add_event_cb(row, [](lv_event_t *e) {
      size_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
      VacuumControlModalUi &ui = vacuum_control_modal_ui();
      if (index >= ui.rooms.size()) return;
      ui.rooms[index].selected = !ui.rooms[index].selected;
      vacuum_control_render_room_list();
      vacuum_control_update_room_controls();
    }, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));
  }
}

inline void vacuum_control_render_fan_list() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  if (!ui.fan_list || !ui.active) return;
  lv_obj_clean(ui.fan_list);
  if (ui.active->fan_speeds.empty()) {
    lv_obj_t *empty = lv_label_create(ui.fan_list);
    if (empty) {
      lv_label_set_text(empty, espcontrol_i18n(std::string("No options")).c_str());
      lv_obj_set_style_text_color(empty, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
      if (ui.active->label_font) lv_obj_set_style_text_font(empty, ui.active->label_font, LV_PART_MAIN);
    }
    return;
  }
  for (size_t i = 0; i < ui.active->fan_speeds.size(); i++) {
    const std::string &speed = ui.active->fan_speeds[i];
    lv_obj_t *row = control_modal_create_list_row(
      ui.fan_list, speed, speed == ui.active->fan_speed, 48, 14,
      DEFAULT_SLIDER_COLOR, DARK_BACKGROUND_TERTIARY, ui.active->label_font,
      ui.active->width_compensation_percent);
    if (!row) continue;
    lv_obj_add_event_cb(row, [](lv_event_t *e) {
      VacuumControlModalUi &ui = vacuum_control_modal_ui();
      size_t index = reinterpret_cast<uintptr_t>(lv_event_get_user_data(e));
      if (!ui.active || index >= ui.active->fan_speeds.size()) return;
      ui.active->fan_speed = ui.active->fan_speeds[index];
      vacuum_send_simple_action(
        ui.active, "vacuum.set_fan_speed", "fan_speed", ui.active->fan_speed.c_str());
      vacuum_control_render_fan_list();
    }, LV_EVENT_CLICKED, reinterpret_cast<void *>(i));
  }
}

inline void apply_vacuum_card_state(VacuumCardCtx *ctx,
                                    esphome::StringRef state,
                                    bool unavailable) {
  if (!ctx) return;
  ctx->state = unavailable ? "unavailable" : std::string(state.c_str(), state.size());
  ctx->available = !unavailable;
  if (ctx->icon_lbl) {
    lv_label_set_text(ctx->icon_lbl, find_icon(vacuum_state_icon_name(ctx->state)));
  }
  if (ctx->text_lbl) {
    std::string label = ctx->status_card
      ? vacuum_state_label(ctx->state, ctx->label)
      : (!ctx->custom_label && !ctx->friendly_name.empty() ? ctx->friendly_name : ctx->label);
    set_wrapped_button_label_text(ctx->text_lbl, label);
  }
  if (ctx->btn && (ctx->mode == "start_stop" || ctx->mode == "modal")) {
    set_card_checked_state(ctx->btn, ctx->state == "cleaning");
  }
  if (ctx->btn && !ctx->status_card) {
    apply_control_availability(ctx->btn, ctx->btn, !unavailable);
  }
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  if (ui.active == ctx) {
    if (ui.title_lbl) {
      std::string title = vacuum_control_title(ctx);
      lv_label_set_text(ui.title_lbl, title.c_str());
    }
    if (ui.status_lbl) {
      std::string label = vacuum_state_label(ctx->state, ctx->state);
      lv_label_set_text(ui.status_lbl, label.c_str());
    }
    vacuum_control_update_room_controls();
  }
}

inline void refresh_vacuum_card_translated_text(lv_obj_t *text_lbl,
                                                VacuumCardCtx *ctx,
                                                const ParsedCfg &p) {
  if (!text_lbl) return;
  std::string label = p.label.empty()
    ? espcontrol_i18n(std::string(vacuum_card_mode_label(p.sensor)))
    : p.label;
  if (ctx && p.label.empty()) {
    ctx->label = espcontrol_i18n(std::string(vacuum_card_mode_label(ctx->mode)));
  }
  if (ctx && ctx->status_card && !ctx->state.empty()) {
    label = vacuum_state_label(ctx->state, ctx->label);
  } else if (ctx) {
    label = ctx->label;
  }
  set_wrapped_button_label_text(text_lbl, label);
}

inline void register_subpage_vacuum_card_text(lv_obj_t *text_lbl,
                                              VacuumCardCtx *ctx,
                                              const ParsedCfg &p) {
  if (!text_lbl) return;
  subpage_vacuum_card_text_refs().push_back({text_lbl, ctx, p});
}

inline void refresh_subpage_vacuum_card_translated_text() {
  for (auto &ref : subpage_vacuum_card_text_refs()) {
    refresh_vacuum_card_translated_text(ref.text_lbl, ref.ctx, ref.cfg);
  }
}

inline void subscribe_vacuum_card_state(VacuumCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef state) {
      bool unavailable = ha_entity_state_unavailable_ref(ctx->entity_id, state);
      apply_vacuum_card_state(ctx, state, unavailable);
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
      if (!ctx->custom_label && ctx->text_lbl && !ctx->status_card) {
        set_wrapped_button_label_text(ctx->text_lbl, ctx->friendly_name);
      }
      VacuumControlModalUi &ui = vacuum_control_modal_ui();
      if (ui.active == ctx && ui.title_lbl) {
        std::string title = vacuum_control_title(ctx);
        lv_label_set_text(ui.title_lbl, title.c_str());
      }
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("fan_speed"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->fan_speed = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
      if (vacuum_control_modal_ui().active == ctx) vacuum_control_render_fan_list();
    })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("fan_speed_list"),
    std::function<void(esphome::StringRef)>([ctx](esphome::StringRef value) {
      ctx->fan_speeds = vacuum_parse_list_attribute(
        string_ref_limited(value, HA_STATE_TEXT_MAX_LEN * 4));
      VacuumControlModalUi &ui = vacuum_control_modal_ui();
      if (ui.active == ctx) {
        vacuum_control_render_fan_list();
        vacuum_control_apply_tab_visibility();
        vacuum_control_layout_modal(ctx);
      }
    })
  );
}

inline void vacuum_control_apply_tab_visibility() {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  VacuumCardCtx *ctx = ui.active;
  if (!ctx) return;
  VacuumControlVisibleTabs visible_tabs = vacuum_control_visible_tabs(ctx);
  bool fan_available = !ctx->fan_speeds.empty();
  if (ui.tab == VacuumControlTab::FAN && !fan_available) {
    ui.tab = vacuum_control_first_available_tab(ctx);
  }
  if (!visible_tabs.contains(ui.tab)) ui.tab = vacuum_control_first_available_tab(ctx);
  int tab_count = 0;
  for (uint8_t i = 0; i < visible_tabs.count; i++) {
    if (visible_tabs.tabs[i] == VacuumControlTab::FAN && !fan_available) continue;
    tab_count++;
  }
  bool show_tab_bar = tab_count > 1;
  if (ui.tab_row) {
    if (show_tab_bar) lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
  }
  bool show_controls = ui.tab == VacuumControlTab::CONTROLS;
  bool show_rooms = ui.tab == VacuumControlTab::ROOMS;
  bool show_fan = ui.tab == VacuumControlTab::FAN && fan_available;
  if (ui.controls_box) {
    if (show_controls) lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.rooms_box) {
    if (show_rooms) lv_obj_clear_flag(ui.rooms_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.rooms_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.fan_box) {
    if (show_fan) lv_obj_clear_flag(ui.fan_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.fan_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.controls_tab) {
    if (show_tab_bar && visible_tabs.contains(VacuumControlTab::CONTROLS)) lv_obj_clear_flag(ui.controls_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.rooms_tab) {
    if (show_tab_bar && visible_tabs.contains(VacuumControlTab::ROOMS)) lv_obj_clear_flag(ui.rooms_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.rooms_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.fan_tab) {
    if (show_tab_bar && visible_tabs.contains(VacuumControlTab::FAN) && fan_available) lv_obj_clear_flag(ui.fan_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.fan_tab, LV_OBJ_FLAG_HIDDEN);
  }
  vacuum_control_style_tab(ui.controls_tab, show_controls);
  vacuum_control_style_tab(ui.rooms_tab, show_rooms);
  vacuum_control_style_tab(ui.fan_tab, show_fan);
}

inline void vacuum_control_layout_modal(VacuumCardCtx *ctx) {
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  if (!ctx || !ui.panel) return;
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  VacuumControlVisibleTabs visible_tabs = vacuum_control_visible_tabs(ctx);
  int tab_count = 0;
  for (uint8_t i = 0; i < visible_tabs.count; i++) {
    if (visible_tabs.tabs[i] == VacuumControlTab::FAN && ctx->fan_speeds.empty()) continue;
    tab_count++;
  }
  if (tab_count < 1) tab_count = 1;

  lv_coord_t content_w = layout.panel_w - layout.inset * 2;
  if (content_w < 180) content_w = layout.panel_w - layout.inset;
  lv_obj_set_style_pad_all(ui.panel, layout.inset, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.panel, 10, LV_PART_MAIN);
  lv_obj_set_layout(ui.panel, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.panel, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.panel, LV_FLEX_ALIGN_START, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.panel, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

  if (ui.title_lbl) lv_obj_set_width(ui.title_lbl, content_w);
  if (ui.status_lbl) lv_obj_set_width(ui.status_lbl, content_w);

  bool show_tab_bar = tab_count > 1;
  if (ui.tab_row && show_tab_bar) {
    lv_coord_t tab_size = layout.back_size * 7 / 10;
    if (tab_size < 44) tab_size = 44;
    if (tab_size > 64) tab_size = 64;
    lv_coord_t gap = tab_size / 5;
    lv_coord_t row_w = tab_size * tab_count + gap * (tab_count - 1) + gap * 2;
    if (row_w > content_w) row_w = content_w;
    lv_obj_set_size(ui.tab_row, row_w, tab_size + gap);
    lv_obj_set_style_radius(ui.tab_row, (tab_size + gap) / 2, LV_PART_MAIN);
    lv_coord_t x = gap;
    for (uint8_t i = 0; i < visible_tabs.count; i++) {
      if (visible_tabs.tabs[i] == VacuumControlTab::FAN && ctx->fan_speeds.empty()) continue;
      lv_obj_t *btn = vacuum_control_tab_button(ui, visible_tabs.tabs[i]);
      if (!btn) continue;
      lv_obj_set_size(btn, tab_size, tab_size);
      lv_obj_set_style_radius(btn, tab_size / 2, LV_PART_MAIN);
      lv_obj_align(btn, LV_ALIGN_LEFT_MID, x, 0);
      x += tab_size + gap;
    }
  }

  lv_coord_t available_h = layout.panel_h - layout.inset * 2 - 132;
  if (show_tab_bar) available_h -= 68;
  if (available_h < 140) available_h = layout.panel_h / 2;
  lv_obj_t *boxes[3] = {ui.controls_box, ui.rooms_box, ui.fan_box};
  for (lv_obj_t *box : boxes) {
    if (!box) continue;
    lv_obj_set_size(box, content_w, available_h);
  }
  if (ui.fan_list) lv_obj_set_size(ui.fan_list, content_w, available_h);
  lv_obj_move_foreground(ui.back_btn);
}

inline void vacuum_control_hide_modal() {
  vacuum_control_hide_room_picker();
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = VacuumControlModalUi();
  control_modal_delete_overlay(ControlModalKind::VACUUM_CONTROL, overlay);
}

inline void vacuum_control_open_modal(VacuumCardCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::VACUUM_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, vacuum_control_hide_modal);
  VacuumControlModalUi &ui = vacuum_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = vacuum_control_first_available_tab(ctx);
  ui.rooms = vacuum_parse_rooms(ctx->options, ctx->area_id);
  if (!ui.panel) return;

  ui.title_lbl = control_modal_create_title(
    ui.panel, vacuum_control_title(ctx), shell.content_w, ctx->label_font,
    ctx->width_compensation_percent);
  ui.status_lbl = control_modal_create_title(
    ui.panel, vacuum_state_label(ctx->state, ctx->state.empty() ? espcontrol_i18n(std::string("Unknown")) : ctx->state),
    shell.content_w, ctx->label_font, ctx->width_compensation_percent);
  if (ui.status_lbl) lv_obj_set_style_text_color(ui.status_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);

  ui.tab_row = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.tab_row, lv_color_hex(DARK_BACKGROUND_SECONDARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.tab_row, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_SCROLLABLE);
  ui.controls_tab = vacuum_control_create_tab_button(
    ui.tab_row, find_icon("Play"), ctx->icon_font, VacuumControlTab::CONTROLS,
    ctx->width_compensation_percent);
  ui.rooms_tab = vacuum_control_create_tab_button(
    ui.tab_row, find_icon("Broom"), ctx->icon_font, VacuumControlTab::ROOMS,
    ctx->width_compensation_percent);
  ui.fan_tab = vacuum_control_create_tab_button(
    ui.tab_row, find_icon("Fan Speed 2"), ctx->icon_font, VacuumControlTab::FAN,
    ctx->width_compensation_percent);

  ui.controls_box = control_modal_create_scroll_list(ui.panel, shell.content_w, 220, 8);
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Start")), find_icon("Play"),
    ctx->label_font, "vacuum.start");
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Pause")), find_icon("Pause"),
    ctx->label_font, "vacuum.pause");
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Stop")), find_icon("Stop"),
    ctx->label_font, "vacuum.stop");
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Dock")), find_icon("Home"),
    ctx->label_font, "vacuum.return_to_base");
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Spot Clean")), find_icon("Vacuum"),
    ctx->label_font, "vacuum.clean_spot");
  vacuum_control_create_action_row(
    ui.controls_box, espcontrol_i18n(std::string("Locate")), find_icon("Robot Vacuum Alert"),
    ctx->label_font, "vacuum.locate");

  ui.rooms_box = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.rooms_box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.rooms_box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.rooms_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.rooms_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(ui.rooms_box, 10, LV_PART_MAIN);
  lv_obj_set_layout(ui.rooms_box, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.rooms_box, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  ui.room_summary_lbl = lv_label_create(ui.rooms_box);
  if (ui.room_summary_lbl) {
    lv_label_set_long_mode(ui.room_summary_lbl, LV_LABEL_LONG_DOT);
    lv_obj_set_width(ui.room_summary_lbl, shell.content_w);
    lv_obj_set_style_text_color(ui.room_summary_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
    lv_obj_set_style_text_align(ui.room_summary_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (ctx->label_font) lv_obj_set_style_text_font(ui.room_summary_lbl, ctx->label_font, LV_PART_MAIN);
  }
  lv_obj_t *pick_btn = control_modal_create_text_button(
    ui.rooms_box, espcontrol_i18n(std::string("Choose Rooms")),
    shell.content_w, shell.content_w, 56, 16, DARK_BACKGROUND_SECONDARY, ctx->label_font);
  if (pick_btn) {
    lv_obj_add_event_cb(pick_btn, [](lv_event_t *) {
      vacuum_control_open_room_picker();
    }, LV_EVENT_CLICKED, nullptr);
  }
  ui.room_clean_btn = control_modal_create_text_button(
    ui.rooms_box, espcontrol_i18n(std::string("Clean")),
    shell.content_w, shell.content_w, 56, 16, DEFAULT_SLIDER_COLOR, ctx->label_font);
  if (ui.room_clean_btn) {
    lv_obj_add_event_cb(ui.room_clean_btn, [](lv_event_t *) {
      vacuum_send_selected_rooms(vacuum_control_modal_ui().active);
    }, LV_EVENT_CLICKED, nullptr);
  }

  ui.fan_box = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.fan_box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.fan_box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.fan_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.fan_box, 0, LV_PART_MAIN);
  ui.fan_list = control_modal_create_scroll_list(ui.fan_box, shell.content_w, 220, 8);
  vacuum_control_render_fan_list();

  vacuum_control_apply_tab_visibility();
  vacuum_control_update_room_controls();
  vacuum_control_layout_modal(ctx);
  vacuum_control_apply_tab_visibility();
}

inline const char *vacuum_service_for_card(const VacuumCardCtx *ctx) {
  if (!ctx) return nullptr;
  if (ctx->mode == "start_stop") {
    return ctx->state == "cleaning" ? "vacuum.stop" : "vacuum.start";
  }
  if (ctx->mode == "pause_resume") {
    if (ctx->state == "cleaning") return "vacuum.pause";
    if (ctx->state == "paused") return "vacuum.start";
    return "vacuum.start_pause";
  }
  if (ctx->mode == "dock") return "vacuum.return_to_base";
  if (ctx->mode == "clean_spot") return "vacuum.clean_spot";
  if (ctx->mode == "locate") return "vacuum.locate";
  if (ctx->mode == "clean_area") return "vacuum.clean_area";
  return nullptr;
}

inline void send_vacuum_card_action(VacuumCardCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || ctx->status_card) return;
  const char *service = vacuum_service_for_card(ctx);
  if (!service) return;
  bool has_area = ctx->mode == "clean_area";
  if (has_area && ctx->area_id.empty()) return;

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, has_area ? 2 : 1)) return;
  ha_action_add_entity(req, ctx->entity_id);
  if (has_area) ha_action_add_data(req, "cleaning_area_id", ctx->area_id.c_str());
  ha_action_send(req);
}
