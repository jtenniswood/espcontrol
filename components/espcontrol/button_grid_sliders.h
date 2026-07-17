#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Slider widgets ───────────────────────────────────────────────────

// Context attached to each LVGL slider via user_data
struct SliderCtx {
  std::string entity_id;
  lv_obj_t *fill = nullptr;
  bool horizontal = false;
  bool cover_tilt = false;
  bool inverted = false;
  lv_coord_t radius = 0;
  bool logged_state = false;
  bool logged_level = false;
  bool media_position = false;
  float media_duration = 0.0f;
  float media_position_seconds = 0.0f;
  uint32_t media_position_updated_ms = 0;
  bool media_position_updated_at_known = false;
  uint32_t media_position_updated_at_ms = 0;
  bool media_seek_pending = false;
  float media_seek_target_seconds = 0.0f;
  uint32_t media_seek_pending_ms = 0;
  bool media_playing = false;
  lv_obj_t *media_slider = nullptr;
  lv_timer_t *media_timer = nullptr;
  uint8_t media_position_refresh_remaining = 0;
  lv_obj_t *media_track_bg = nullptr;
  lv_obj_t *media_value_lbl = nullptr;
  lv_obj_t *media_status_lbl = nullptr;
  lv_coord_t content_pad = 0;
  bool available = true;
  bool interactive = true;
  // light_temperature fields
  bool light_temp = false;
  int kelvin_min = 2000;
  int kelvin_max = 6500;
  bool kelvin_color = false;
  bool light_on = false;
  bool light_state_known = false;
  bool light_temp_has_kelvin = false;
  int light_temp_last_kelvin = 2000;
  bool light_temp_dragging = false;
  lv_obj_t *text_lbl = nullptr;
  std::string cached_label;
};

struct MediaNowPlayingCtx {
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *artist_lbl = nullptr;
  lv_obj_t *progress_slider = nullptr;
  ImageCardCtx *cover_art = nullptr;
  lv_obj_t *cover_overlay = nullptr;
  lv_obj_t *btn = nullptr;
  char artist[HA_STATE_TEXT_MAX_LEN + 1] = {};
  bool external_source = false;
  bool play_pause_background = false;
  bool artist_below_title = false;
  lv_coord_t artist_gap = 0;
};

struct MediaPlaylistCtx {
  lv_obj_t *btn = nullptr;
  std::string entity_id;
  std::string content_id;
  std::string content_type;
  std::string current_content_id;
  std::string current_content_type;
  bool available = true;
  bool playing = false;
  bool has_current_content_id = false;
  bool has_current_content_type = false;
};

constexpr uint32_t MEDIA_SEEK_PENDING_TIMEOUT_MS = 3000;
constexpr float MEDIA_SEEK_MATCH_TOLERANCE_SECONDS = 2.0f;
constexpr lv_coord_t MEDIA_VOLUME_REFERENCE_SIDE_PX = DISPLAY_MODAL_REFERENCE_SIDE_PX;
constexpr lv_coord_t MEDIA_VOLUME_ARC_STROKE_REF_PX = DISPLAY_MODAL_ARC_STROKE_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_BACK_BUTTON_REF_PX = DISPLAY_MODAL_BACK_BUTTON_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_BUTTON_REF_PX = DISPLAY_MODAL_BUTTON_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_INSET_REF_PX = DISPLAY_MODAL_INSET_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_CONTROLS_GAP_REF_PX = DISPLAY_MODAL_CONTROLS_GAP_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_CONTROLS_DOWN_REF_PX = DISPLAY_MODAL_CONTROLS_DOWN_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_TITLE_GAP_REF_PX = DISPLAY_MODAL_TITLE_GAP_REF_PX;
constexpr lv_coord_t MEDIA_VOLUME_UNIT_Y_REF_PX = -22;
constexpr lv_coord_t MEDIA_VOLUME_COMPACT_PORTRAIT_BUTTON_REF_PX = 96;
constexpr lv_coord_t MEDIA_VOLUME_MIC_BUTTON_OFFSET_REF_PX = 8;
constexpr int MEDIA_VOLUME_MIC_ICON_ZOOM = 210;

struct MediaVolumeCtx {
  std::string entity_id;
  std::string label;
  int current_pct = 0;
  int max_pct = 100;
  int pending_pct = -1;
  uint32_t pending_until_ms = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  lv_obj_t *btn = nullptr;
  lv_obj_t *label_lbl = nullptr;
  lv_obj_t *pct_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  int width_compensation_percent = 100;
  const lv_font_t *value_font = nullptr;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *unit_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  std::function<void(int)> apply_percent;
  std::function<bool()> mic_muted;
  std::function<void(bool)> set_mic_muted;
  bool available = true;
};

struct MediaVolumeModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *arc = nullptr;
  lv_obj_t *title_lbl = nullptr;
  lv_obj_t *pct_row = nullptr;
  lv_obj_t *pct_lbl = nullptr;
  lv_obj_t *pct_unit_lbl = nullptr;
  lv_obj_t *minus_btn = nullptr;
  lv_obj_t *plus_btn = nullptr;
  lv_obj_t *mic_btn = nullptr;
  lv_obj_t *mic_lbl = nullptr;
  MediaVolumeCtx *active = nullptr;
  bool updating_arc = false;
};

inline MediaVolumeModalUi &media_volume_modal_ui() {
  static MediaVolumeModalUi ui;
  return ui;
}

inline int slider_clamp_pct(int pct);
inline bool slider_parse_light_brightness_pct(esphome::StringRef val, int &pct);
inline void slider_set_value_safe(lv_obj_t *slider, int pct);
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color,
                                bool interactive = true);

struct LightControlCtx {
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string options;
  int current_pct = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const char *icon_off_glyph = nullptr;
  const char *icon_on_glyph = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  int current_kelvin = 3500;
  int kelvin_min = 2000;
  int kelvin_max = 6500;
  bool available = true;
  bool on = false;
  bool color_modes_known = false;
  bool rgb_color_supported = true;
  bool updating_slider = false;
  bool updating_temp_slider = false;
  bool dragging_slider = false;
  bool dragging_temp_slider = false;
};

enum class LightControlTab : uint8_t {
  POWER = 0,
  BRIGHTNESS = 1,
  TEMPERATURE = 2,
  COLOR = 3,
};

struct LightControlVisibleTabs {
  LightControlTab tabs[4] = {
    LightControlTab::POWER,
    LightControlTab::BRIGHTNESS,
    LightControlTab::TEMPERATURE,
    LightControlTab::COLOR,
  };
  uint8_t count = 0;

  bool contains(LightControlTab tab) const {
    for (uint8_t i = 0; i < count; i++) {
      if (tabs[i] == tab) return true;
    }
    return false;
  }

  void add(LightControlTab tab) {
    if (count >= 4 || contains(tab)) return;
    tabs[count++] = tab;
  }
};

struct LightColorPresetClick {
  uint32_t color = 0;
  int kelvin_pct = -1;
};

struct LightColorPreset {
  uint32_t color;
  int kelvin_pct = -1;
};

struct LightControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *power_tab = nullptr;
  lv_obj_t *brightness_tab = nullptr;
  lv_obj_t *temperature_tab = nullptr;
  lv_obj_t *color_tab = nullptr;
  lv_obj_t *power_group = nullptr;
  lv_obj_t *power_on_btn = nullptr;
  lv_obj_t *power_off_btn = nullptr;
  lv_obj_t *slider = nullptr;
  lv_obj_t *slider_fill = nullptr;
  lv_obj_t *slider_handle = nullptr;
  lv_obj_t *temp_slider = nullptr;
  lv_obj_t *temp_slider_fill = nullptr;
  lv_obj_t *temp_slider_handle = nullptr;
  lv_obj_t *color_grid = nullptr;
  LightControlCtx *active = nullptr;
  LightControlTab tab = LightControlTab::POWER;
};

inline LightControlModalUi &light_control_modal_ui() {
  static LightControlModalUi ui;
  return ui;
}

inline bool light_control_tab_from_token(const std::string &value, LightControlTab &tab) {
  if (value == "power") {
    tab = LightControlTab::POWER;
    return true;
  }
  if (value == "brightness") {
    tab = LightControlTab::BRIGHTNESS;
    return true;
  }
  if (value == "temperature") {
    tab = LightControlTab::TEMPERATURE;
    return true;
  }
  if (value == "color") {
    tab = LightControlTab::COLOR;
    return true;
  }
  return false;
}

inline LightControlVisibleTabs light_control_visible_tabs(LightControlCtx *ctx) {
  LightControlVisibleTabs visible;
  std::string value = cfg_option_value(ctx ? ctx->options : "", LIGHT_CONTROL_TABS_OPTION);
  if (value.empty()) value = LIGHT_CONTROL_DEFAULT_TABS_VALUE;

  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find('|', start);
    std::string token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
    LightControlTab tab = LightControlTab::POWER;
    if (light_control_tab_from_token(token, tab)) visible.add(tab);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (visible.count == 0) visible.add(LightControlTab::POWER);
  return visible;
}

inline bool light_control_tab_visible(LightControlCtx *ctx, LightControlTab tab) {
  LightControlVisibleTabs tabs = light_control_visible_tabs(ctx);
  return tabs.contains(tab);
}

inline bool light_control_use_temperature_swatches(LightControlCtx *ctx) {
  return ctx && ctx->color_modes_known && !ctx->rgb_color_supported;
}

inline uint32_t light_control_swatch_count(LightControlCtx *ctx) {
  return light_control_use_temperature_swatches(ctx) ? 4 : 16;
}

inline LightControlTab light_control_first_visible_tab(LightControlCtx *ctx) {
  LightControlVisibleTabs tabs = light_control_visible_tabs(ctx);
  return tabs.count == 0 ? LightControlTab::POWER : tabs.tabs[0];
}

inline void light_control_ensure_visible_tab(LightControlCtx *ctx) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (light_control_tab_visible(ctx, ui.tab)) return;
  ui.tab = light_control_first_visible_tab(ctx);
}

inline lv_obj_t *light_control_tab_button(LightControlModalUi &ui, LightControlTab tab) {
  switch (tab) {
    case LightControlTab::POWER: return ui.power_tab;
    case LightControlTab::BRIGHTNESS: return ui.brightness_tab;
    case LightControlTab::TEMPERATURE: return ui.temperature_tab;
    case LightControlTab::COLOR: return ui.color_tab;
  }
  return nullptr;
}

inline void light_control_update_slider_handle(lv_obj_t *slider, lv_obj_t *handle, int pct);
inline void light_control_update_slider_fill(lv_obj_t *slider, lv_obj_t *fill,
                                             lv_obj_t *handle, int pct,
                                             lv_color_t fill_color);
inline void light_control_rebuild_color_grid(LightControlCtx *ctx);

inline std::string light_control_title(LightControlCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Light"));
  if (!ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Light"));
}

inline const char *light_control_icon_off(const ParsedCfg &p) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon("Lightbulb Outline");
}

inline const char *light_control_icon_on(const ParsedCfg &p) {
  if (!p.icon_on.empty() && p.icon_on != "Auto") return find_icon(p.icon_on.c_str());
  return find_icon("Lightbulb");
}

inline void light_control_apply_card_visual(LightControlCtx *ctx) {
  if (!ctx || !ctx->btn) return;
  set_card_checked_state(ctx->btn, ctx->on);
  if (ctx->icon_lbl) {
    const char *glyph = ctx->on && ctx->icon_on_glyph ? ctx->icon_on_glyph : ctx->icon_off_glyph;
    if (glyph) lv_label_set_text(ctx->icon_lbl, glyph);
  }
  if (ctx->label_lbl) {
    std::string title = light_control_title(ctx);
    lv_label_set_text(ctx->label_lbl, title.c_str());
  }
}

inline int light_control_display_pct(LightControlCtx *ctx) {
  return ctx && ctx->on ? ctx->current_pct : 0;
}

inline void light_control_set_modal_value(LightControlCtx *ctx, int pct) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  if (ctx->dragging_slider) return;
  pct = slider_clamp_pct(pct);
  if (ui.slider) {
    ctx->updating_slider = true;
    lv_slider_set_value(ui.slider, pct, LV_ANIM_OFF);
    ctx->updating_slider = false;
  }
  light_control_update_slider_fill(
    ui.slider, ui.slider_fill, ui.slider_handle, pct, lv_color_hex(ctx->accent_color));
  light_control_update_slider_handle(ui.slider, ui.slider_handle, pct);
}

inline void light_control_apply_modal_power(LightControlCtx *ctx) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  lv_obj_t *on_label = ui.power_on_btn ? lv_obj_get_child(ui.power_on_btn, 0) : nullptr;
  lv_obj_t *off_label = ui.power_off_btn ? lv_obj_get_child(ui.power_off_btn, 0) : nullptr;
  if (ui.power_on_btn) {
    lv_obj_set_style_bg_color(
      ui.power_on_btn,
      lv_color_hex(ctx->on ? ctx->accent_color : SECONDARY_GREY),
      LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui.power_on_btn, ctx->on ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.power_on_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ui.power_on_btn, 0, LV_PART_MAIN);
  }
  if (ui.power_off_btn) {
    lv_obj_set_style_bg_color(
      ui.power_off_btn,
      lv_color_hex(ctx->on ? SECONDARY_GREY : DARK_TEXT_PRIMARY),
      LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ui.power_off_btn, ctx->on ? LV_OPA_TRANSP : LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(ui.power_off_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ui.power_off_btn, 0, LV_PART_MAIN);
  }
  if (on_label) {
    lv_obj_set_style_text_color(
      on_label,
      lv_color_hex(ctx->on ? DARK_TEXT_PRIMARY : DARK_TEXT_MUTED),
      LV_PART_MAIN);
  }
  if (off_label) {
    lv_obj_set_style_text_color(
      off_label,
      lv_color_hex(ctx->on ? DARK_TEXT_MUTED : DARK_TEXT_INVERTED),
      LV_PART_MAIN);
  }
}

inline int light_control_kelvin_to_pct(LightControlCtx *ctx, int kelvin) {
  if (!ctx || ctx->kelvin_max <= ctx->kelvin_min) return 50;
  if (kelvin < ctx->kelvin_min) kelvin = ctx->kelvin_min;
  if (kelvin > ctx->kelvin_max) kelvin = ctx->kelvin_max;
  return 100 - ((kelvin - ctx->kelvin_min) * 100 / (ctx->kelvin_max - ctx->kelvin_min));
}

inline int light_control_kelvin_to_command_pct(LightControlCtx *ctx, int kelvin) {
  if (!ctx || ctx->kelvin_max <= ctx->kelvin_min) return 50;
  if (kelvin < ctx->kelvin_min) kelvin = ctx->kelvin_min;
  if (kelvin > ctx->kelvin_max) kelvin = ctx->kelvin_max;
  return (kelvin - ctx->kelvin_min) * 100 / (ctx->kelvin_max - ctx->kelvin_min);
}

inline int light_control_pct_to_kelvin(LightControlCtx *ctx, int pct) {
  if (!ctx || ctx->kelvin_max <= ctx->kelvin_min) return 3500;
  pct = slider_clamp_pct(pct);
  return ctx->kelvin_min + (100 - pct) * (ctx->kelvin_max - ctx->kelvin_min) / 100;
}

inline void light_control_set_temp_modal_value(LightControlCtx *ctx, int kelvin) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  if (ctx->dragging_temp_slider) return;
  if (kelvin < ctx->kelvin_min) kelvin = ctx->kelvin_min;
  if (kelvin > ctx->kelvin_max) kelvin = ctx->kelvin_max;
  if (ui.temp_slider) {
    ctx->updating_temp_slider = true;
    lv_slider_set_value(ui.temp_slider, light_control_kelvin_to_pct(ctx, kelvin), LV_ANIM_OFF);
    ctx->updating_temp_slider = false;
  }
  lv_color_t fill_color = kelvin_to_fill_color(kelvin, ctx->kelvin_min, ctx->kelvin_max);
  light_control_update_slider_fill(
    ui.temp_slider, ui.temp_slider_fill, ui.temp_slider_handle,
    light_control_kelvin_to_pct(ctx, kelvin), fill_color);
  light_control_update_slider_handle(
    ui.temp_slider, ui.temp_slider_handle, light_control_kelvin_to_pct(ctx, kelvin));
}

inline void light_control_style_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
  if (!btn) return;
  (void) accent_color;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? TERTIARY_GREY : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline void light_control_apply_tab_visibility() {
  LightControlModalUi &ui = light_control_modal_ui();
  LightControlCtx *ctx = ui.active;
  if (!ctx) return;
  LightControlVisibleTabs visible_tabs = light_control_visible_tabs(ctx);
  if (!visible_tabs.contains(ui.tab)) ui.tab = visible_tabs.tabs[0];
  bool show_tab_bar = visible_tabs.count > 1;
  bool show_power = ui.tab == LightControlTab::POWER;
  bool show_brightness = ui.tab == LightControlTab::BRIGHTNESS;
  bool show_temperature = ui.tab == LightControlTab::TEMPERATURE;
  bool show_color = ui.tab == LightControlTab::COLOR;
  if (ui.tab_row) {
    if (show_tab_bar) lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.power_tab) {
    if (show_tab_bar && visible_tabs.contains(LightControlTab::POWER)) lv_obj_clear_flag(ui.power_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.power_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.brightness_tab) {
    if (show_tab_bar && visible_tabs.contains(LightControlTab::BRIGHTNESS)) lv_obj_clear_flag(ui.brightness_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.brightness_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.temperature_tab) {
    if (show_tab_bar && visible_tabs.contains(LightControlTab::TEMPERATURE)) lv_obj_clear_flag(ui.temperature_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.temperature_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.color_tab) {
    if (show_tab_bar && visible_tabs.contains(LightControlTab::COLOR)) lv_obj_clear_flag(ui.color_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.color_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.power_group) {
    if (show_power) lv_obj_clear_flag(ui.power_group, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.power_group, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.slider) {
    if (show_brightness) lv_obj_clear_flag(ui.slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.slider, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.temp_slider) {
    if (show_temperature) lv_obj_clear_flag(ui.temp_slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.temp_slider, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.color_grid) {
    if (show_color) lv_obj_clear_flag(ui.color_grid, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.color_grid, LV_OBJ_FLAG_HIDDEN);
  }
  light_control_style_tab(ui.power_tab, show_power, ctx->accent_color);
  light_control_style_tab(ui.brightness_tab, show_brightness, ctx->accent_color);
  light_control_style_tab(ui.temperature_tab, show_temperature, ctx->accent_color);
  light_control_style_tab(ui.color_tab, show_color, ctx->accent_color);
  light_control_apply_modal_power(ctx);
}

inline void light_control_layout_modal(LightControlCtx *ctx);

inline void light_control_center_icon_label(lv_obj_t *label) {
  control_modal_center_tab_icon(label);
}

inline lv_obj_t *light_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                 const lv_font_t *font,
                                                 LightControlTab tab,
                                                 int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  apply_width_compensation(btn, width_compensation_percent);
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
    LightControlTab tab = static_cast<LightControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    LightControlModalUi &ui = light_control_modal_ui();
    ui.tab = tab;
    light_control_apply_tab_visibility();
    light_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *light_control_create_slider_handle(lv_obj_t *slider) {
  if (!slider) return nullptr;
  lv_obj_t *handle = lv_obj_create(slider);
  if (!handle) return nullptr;
  lv_obj_set_size(handle, 0, 0);
  lv_obj_set_style_bg_color(handle, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(handle, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(handle, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(handle, 0, LV_PART_MAIN);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_SCROLLABLE);
  return handle;
}

inline lv_coord_t light_control_slider_handle_inset(lv_obj_t *slider) {
  if (!slider) return 18;
  lv_coord_t inset = lv_obj_get_style_radius(slider, LV_PART_MAIN) * 3 / 4;
  if (inset < 16) inset = 16;
  if (inset > 28) inset = 28;
  return inset;
}

inline lv_coord_t light_control_slider_handle_height(lv_obj_t *slider) {
  if (!slider) return 6;
  lv_coord_t height = lv_obj_get_height(slider);
  lv_coord_t handle_h = height / 70;
  if (handle_h < 5) handle_h = 5;
  if (handle_h > 8) handle_h = 8;
  return handle_h;
}

inline lv_coord_t light_control_slider_fill_height(lv_obj_t *slider, int pct) {
  if (!slider) return 0;
  lv_coord_t height = lv_obj_get_height(slider);
  if (height <= 0) return 0;
  lv_coord_t fill_h = (lv_coord_t)((int32_t) height * slider_clamp_pct(pct) / 100);
  lv_coord_t min_handle_cap = light_control_slider_handle_inset(slider) * 2 +
    light_control_slider_handle_height(slider);
  if (fill_h < min_handle_cap) fill_h = min_handle_cap;
  if (fill_h > height) fill_h = height;
  return fill_h;
}

inline lv_obj_t *light_control_create_slider_fill(lv_obj_t *slider, lv_color_t fill_color) {
  if (!slider) return nullptr;
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_t *fill = lv_obj_create(slider);
  if (!fill) return nullptr;
  lv_obj_set_size(fill, 0, 0);
  lv_obj_set_style_bg_color(fill, fill_color, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(fill, 0, LV_PART_MAIN);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
  return fill;
}

inline void light_control_update_slider_fill(lv_obj_t *slider, lv_obj_t *fill,
                                             lv_obj_t *handle, int pct,
                                             lv_color_t fill_color) {
  if (!slider || !fill) return;
  lv_coord_t height = lv_obj_get_height(slider);
  lv_coord_t width = lv_obj_get_width(slider);
  if (height <= 0 || width <= 0) return;
  pct = slider_clamp_pct(pct);
  lv_obj_set_style_bg_color(fill, fill_color, LV_PART_MAIN);
  lv_coord_t fill_h = light_control_slider_fill_height(slider, pct);
  lv_obj_set_size(fill, width, fill_h);
  lv_obj_set_style_radius(fill, 0, LV_PART_MAIN);
  lv_obj_align(fill, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_move_foreground(fill);
  if (handle) lv_obj_move_foreground(handle);
}

inline void light_control_update_slider_handle(lv_obj_t *slider, lv_obj_t *handle, int pct) {
  if (!slider || !handle) return;
  lv_coord_t width = lv_obj_get_width(slider);
  lv_coord_t height = lv_obj_get_height(slider);
  if (width <= 0 || height <= 0) return;
  lv_coord_t handle_w = width * 3 / 5;
  if (handle_w < 20) handle_w = 20;
  if (handle_w > width - 12) handle_w = width - 12;
  if (handle_w < 8) handle_w = 8;
  lv_coord_t handle_h = light_control_slider_handle_height(slider);
  lv_coord_t inset = light_control_slider_handle_inset(slider);
  lv_coord_t fill_h = light_control_slider_fill_height(slider, pct);
  lv_coord_t y = height - fill_h + inset;
  if (y < inset) y = inset;
  if (y > height - inset - handle_h) y = height - inset - handle_h;
  if (y > height - handle_h) y = height - handle_h;
  lv_obj_set_size(handle, handle_w, handle_h);
  lv_obj_set_style_radius(handle, handle_h / 2, LV_PART_MAIN);
  lv_obj_align(handle, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(handle);
}

inline void light_control_layout_slider(lv_obj_t *slider, lv_coord_t width,
                                        lv_coord_t height, lv_coord_t center_y,
                                        int width_compensation_percent) {
  if (!slider) return;
  lv_obj_set_size(slider, width, height);
  apply_width_compensation(slider, width_compensation_percent);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, center_y);
  lv_coord_t slider_radius = width / 5;
  if (slider_radius < 18) slider_radius = 18;
  if (slider_radius > 34) slider_radius = 34;
  lv_obj_set_style_radius(slider, slider_radius, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(slider, true, LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
  lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
}

inline void light_control_style_slider(lv_obj_t *slider, uint32_t accent_color) {
  if (!slider) return;
  lv_slider_set_range(slider, 0, 100);
  lv_obj_set_style_bg_color(slider, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(accent_color), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
}

inline void light_control_toggle_modal_power() {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ui.active || !ui.active->available) return;
  bool turn_on = !ui.active->on;
  ui.active->on = turn_on;
  light_control_apply_card_visual(ui.active);
  light_control_set_modal_value(ui.active, light_control_display_pct(ui.active));
  light_control_apply_modal_power(ui.active);
  if (turn_on) send_turn_on_action(ui.active->entity_id);
  else send_turn_off_action(ui.active->entity_id);
}

inline lv_obj_t *light_control_create_power_button(lv_obj_t *parent, const lv_font_t *font,
                                                   int width_compensation_percent,
                                                   bool turn_on) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  (void) width_compensation_percent;
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    lv_label_set_text(label, find_icon(turn_on ? "Power" : "Circle Outline"));
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(label, turn_on ? 230 : 180, LV_PART_MAIN);
    light_control_center_icon_label(label);
  }
  return btn;
}

inline void light_control_layout_power(lv_obj_t *group, lv_obj_t *on_btn,
                                       lv_obj_t *off_btn, lv_coord_t width,
                                       lv_coord_t height, lv_coord_t center_y,
                                       int width_compensation_percent) {
  if (!group) return;
  lv_coord_t group_w = compensated_width(width, width_compensation_percent);
  if (group_w < width / 2) group_w = width / 2;
  if (group_w > width) group_w = width;
  lv_obj_set_size(group, group_w, height);
  apply_width_compensation(
    group, width_compensation_vertical_axis() ? width_compensation_percent : 100);
  lv_obj_align(group, LV_ALIGN_CENTER, 0, center_y);
  lv_coord_t radius = group_w / 4;
  if (radius < 24) radius = 24;
  if (radius > 46) radius = 46;
  lv_obj_set_style_radius(group, radius, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(group, true, LV_PART_MAIN);
  lv_coord_t inset = group_w / 16;
  if (inset < 8) inset = 8;
  if (inset > 16) inset = 16;
  lv_coord_t gap = inset;
  lv_coord_t button_w = group_w - inset * 2;
  lv_coord_t button_h = (height - inset * 2 - gap) / 2;
  if (button_w < group_w / 2) button_w = group_w / 2;
  if (button_h < 48) button_h = 48;
  lv_coord_t button_radius = group_w > 0 ? radius * button_w / group_w : radius;
  if (button_radius < 16) button_radius = 16;
  if (button_radius > button_h / 2) button_radius = button_h / 2;
  if (on_btn) {
    lv_obj_set_size(on_btn, button_w, button_h);
    lv_obj_set_style_radius(on_btn, button_radius, LV_PART_MAIN);
    lv_obj_align(on_btn, LV_ALIGN_TOP_MID, 0, inset);
    lv_obj_t *label = lv_obj_get_child(on_btn, 0);
    light_control_center_icon_label(label);
  }
  if (off_btn) {
    lv_obj_set_size(off_btn, button_w, button_h);
    lv_obj_set_style_radius(off_btn, button_radius, LV_PART_MAIN);
    lv_obj_align(off_btn, LV_ALIGN_BOTTOM_MID, 0, -inset);
    lv_obj_t *label = lv_obj_get_child(off_btn, 0);
    light_control_center_icon_label(label);
  }
}

inline lv_coord_t light_control_modal_control_width(const ControlModalLayout &layout,
                                                    lv_coord_t card_width,
                                                    lv_coord_t content_height) {
  lv_coord_t width = card_width;
  if (control_modal_uses_wide_landscape_tuning(layout)) {
    lv_coord_t max_width = content_height * 9 / 16;
    if (max_width < control_modal_scaled_px(160, layout.short_side))
      max_width = control_modal_scaled_px(160, layout.short_side);
    if (width > max_width) width = max_width;
  }
  return width;
}

inline void light_control_layout_modal(LightControlCtx *ctx) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ctx || !ui.panel) return;
  light_control_ensure_visible_tab(ctx);
  LightControlVisibleTabs visible_tabs = light_control_visible_tabs(ctx);
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);

  int tab_count = static_cast<int>(visible_tabs.count);
  bool show_tab_bar = tab_count > 1;
  ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);
  control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);
  for (int i = 0; show_tab_bar && i < tab_count; i++) {
    lv_obj_t *tab_btn = light_control_tab_button(ui, visible_tabs.tabs[i]);
    if (!tab_btn) continue;
    bool active = (visible_tabs.tabs[i] == ui.tab);
    control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);
  }

  const espcontrol::modal::ContentLayout content = control_modal_calc_content_layout(
    layout, tabs_layout, show_tab_bar, 160);
  lv_coord_t slider_h = content.height;
  lv_coord_t slider_w = light_control_modal_control_width(
    layout, control_modal_home_card_width(ctx->btn, layout), slider_h);
  lv_coord_t content_center_y = content.center_y;
  light_control_layout_power(
    ui.power_group, ui.power_on_btn, ui.power_off_btn, slider_w, slider_h,
    content_center_y, ctx->width_compensation_percent);
  light_control_apply_modal_power(ctx);
  light_control_layout_slider(
    ui.slider, slider_w, slider_h, content_center_y, ctx->width_compensation_percent);
  lv_obj_update_layout(ui.panel);
  int display_pct = light_control_display_pct(ctx);
  light_control_update_slider_fill(
    ui.slider, ui.slider_fill, ui.slider_handle, display_pct,
    lv_color_hex(ctx->accent_color));
  light_control_update_slider_handle(ui.slider, ui.slider_handle, display_pct);
  light_control_layout_slider(
    ui.temp_slider, slider_w, slider_h, content_center_y, ctx->width_compensation_percent);
  lv_obj_update_layout(ui.panel);
  light_control_update_slider_fill(
    ui.temp_slider, ui.temp_slider_fill, ui.temp_slider_handle,
    light_control_kelvin_to_pct(ctx, ctx->current_kelvin),
    kelvin_to_fill_color(ctx->current_kelvin, ctx->kelvin_min, ctx->kelvin_max));
  light_control_update_slider_handle(
    ui.temp_slider, ui.temp_slider_handle, light_control_kelvin_to_pct(ctx, ctx->current_kelvin));
  if (ui.color_grid) {
    lv_coord_t grid_side = layout.panel_w - layout.inset * 3;
    lv_coord_t color_safe_top = content.top;
    if (!show_tab_bar) {
      lv_coord_t chrome_safe_top = layout.back_inset_y + layout.back_size + layout.inset / 2;
      if (color_safe_top < chrome_safe_top) color_safe_top = chrome_safe_top;
    }
    lv_coord_t max_grid_h = content.bottom - color_safe_top;
    if (grid_side > max_grid_h) grid_side = max_grid_h;
    if (grid_side < 180) grid_side = 180;
    lv_obj_set_size(ui.color_grid, grid_side, grid_side);
    lv_coord_t color_center_y = content_center_y;
    lv_coord_t color_top = layout.panel_h / 2 + color_center_y - grid_side / 2;
    if (!show_tab_bar && color_top < color_safe_top) color_center_y += color_safe_top - color_top;
    lv_obj_align(ui.color_grid, LV_ALIGN_CENTER, 0, color_center_y);
    uint32_t swatch_count = light_control_swatch_count(ctx);
    uint32_t columns = swatch_count <= 4 ? swatch_count : 4;
    uint32_t rows = (swatch_count + columns - 1) / columns;
    lv_coord_t gap = 14;
    lv_coord_t swatch = (grid_side - gap * static_cast<lv_coord_t>(columns - 1)) /
      static_cast<lv_coord_t>(columns);
    if (swatch_count <= 4) {
      lv_coord_t max_swatch = control_modal_scaled_px(80, layout.short_side);
      if (swatch > max_swatch) swatch = max_swatch;
    }
    lv_coord_t used_w = swatch * static_cast<lv_coord_t>(columns) +
      gap * static_cast<lv_coord_t>(columns - 1);
    lv_coord_t used_h = swatch * static_cast<lv_coord_t>(rows) +
      gap * static_cast<lv_coord_t>(rows - 1);
    lv_coord_t start_x = (grid_side - used_w) / 2;
    lv_coord_t start_y = (grid_side - used_h) / 2;
    for (uint32_t i = 0; i < swatch_count; i++) {
      lv_obj_t *btn = lv_obj_get_child(ui.color_grid, i);
      if (!btn) continue;
      lv_obj_set_size(btn, swatch, swatch);
      apply_width_compensation(btn, ctx->width_compensation_percent);
      lv_obj_align(btn, LV_ALIGN_TOP_LEFT,
        start_x + static_cast<lv_coord_t>((i % columns) * (swatch + gap)),
        start_y + static_cast<lv_coord_t>((i / columns) * (swatch + gap)));
      lv_obj_set_style_radius(btn, swatch / 2, LV_PART_MAIN);
    }
  }
}

inline void light_control_hide_modal() {
  LightControlModalUi &ui = light_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = LightControlModalUi();
  control_modal_delete_overlay(ControlModalKind::LIGHT_CONTROL, overlay);
}

inline void light_control_open_modal(LightControlCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::LIGHT_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, light_control_hide_modal);
  LightControlModalUi &ui = light_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = light_control_first_visible_tab(ctx);
  if (!ui.panel) return;

  ui.tab_row = control_modal_create_tab_row(ui.panel);
  ui.power_tab = light_control_create_tab_button(
    ui.tab_row, find_icon("Power"), ctx->icon_font,
    LightControlTab::POWER, ctx->width_compensation_percent);
  ui.brightness_tab = light_control_create_tab_button(
    ui.tab_row, find_icon("Lightbulb"), ctx->icon_font,
    LightControlTab::BRIGHTNESS, ctx->width_compensation_percent);
  ui.temperature_tab = light_control_create_tab_button(
    ui.tab_row, find_icon("Thermometer"), ctx->icon_font,
    LightControlTab::TEMPERATURE, ctx->width_compensation_percent);
  ui.color_tab = light_control_create_tab_button(
    ui.tab_row, find_icon("Palette"), ctx->icon_font,
    LightControlTab::COLOR, ctx->width_compensation_percent);

  ui.power_group = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.power_group, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.power_group, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.power_group, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.power_group, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.power_group, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(ui.power_group);
  lv_obj_add_flag(ui.power_group, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.power_group, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(ui.power_group, [](lv_event_t *) {
    light_control_toggle_modal_power();
  }, LV_EVENT_CLICKED, nullptr);
  ui.power_on_btn = light_control_create_power_button(
    ui.power_group, ctx->icon_font, ctx->width_compensation_percent, true);
  ui.power_off_btn = light_control_create_power_button(
    ui.power_group, ctx->icon_font, ctx->width_compensation_percent, false);

  ui.slider = lv_slider_create(ui.panel);
  light_control_style_slider(ui.slider, ctx->accent_color);
  lv_slider_set_value(ui.slider, slider_clamp_pct(ctx->current_pct), LV_ANIM_OFF);
  ui.slider_fill = light_control_create_slider_fill(ui.slider, lv_color_hex(ctx->accent_color));
  ui.slider_handle = light_control_create_slider_handle(ui.slider);
  lv_obj_add_event_cb(ui.slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (ui.active) ui.active->dragging_slider = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (!ui.active || ui.active->updating_slider) return;
    ui.active->dragging_slider = true;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    if (ui.active->current_pct == pct) return;
    ui.active->current_pct = pct;
    light_control_update_slider_fill(
      slider, ui.slider_fill, ui.slider_handle, pct, lv_color_hex(ui.active->accent_color));
    light_control_update_slider_handle(slider, ui.slider_handle, pct);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_slider = false;
    if (!ui.active->available) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->current_pct = pct;
    light_control_update_slider_fill(
      slider, ui.slider_fill, ui.slider_handle, pct, lv_color_hex(ui.active->accent_color));
    light_control_update_slider_handle(slider, ui.slider_handle, pct);
    send_slider_action(ui.active->entity_id, pct);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (ui.active) ui.active->dragging_slider = false;
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.temp_slider = lv_slider_create(ui.panel);
  light_control_style_slider(ui.temp_slider, ctx->accent_color);
  lv_slider_set_value(ui.temp_slider, light_control_kelvin_to_pct(ctx, ctx->current_kelvin), LV_ANIM_OFF);
  ui.temp_slider_fill = light_control_create_slider_fill(
    ui.temp_slider, kelvin_to_fill_color(ctx->current_kelvin, ctx->kelvin_min, ctx->kelvin_max));
  ui.temp_slider_handle = light_control_create_slider_handle(ui.temp_slider);
  lv_obj_add_event_cb(ui.temp_slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (ui.active) ui.active->dragging_temp_slider = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.temp_slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (!ui.active || ui.active->updating_temp_slider) return;
    ui.active->dragging_temp_slider = true;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int kelvin = light_control_pct_to_kelvin(ui.active, lv_slider_get_value(slider));
    if (ui.active->current_kelvin == kelvin) return;
    ui.active->current_kelvin = kelvin;
    lv_color_t fill_color = kelvin_to_fill_color(
      kelvin, ui.active->kelvin_min, ui.active->kelvin_max);
    light_control_update_slider_fill(
      slider, ui.temp_slider_fill, ui.temp_slider_handle,
      light_control_kelvin_to_pct(ui.active, kelvin), fill_color);
    light_control_update_slider_handle(
      slider, ui.temp_slider_handle, light_control_kelvin_to_pct(ui.active, kelvin));
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.temp_slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_temp_slider = false;
    if (!ui.active->available) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    int kelvin = light_control_pct_to_kelvin(ui.active, pct);
    light_control_update_slider_fill(
      slider, ui.temp_slider_fill, ui.temp_slider_handle, pct,
      kelvin_to_fill_color(kelvin, ui.active->kelvin_min, ui.active->kelvin_max));
    light_control_update_slider_handle(slider, ui.temp_slider_handle, lv_slider_get_value(slider));
    send_light_temp_action(
      ui.active->entity_id, light_control_kelvin_to_command_pct(ui.active, kelvin),
      ui.active->kelvin_min, ui.active->kelvin_max);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.temp_slider, [](lv_event_t *e) {
    LightControlModalUi &ui = light_control_modal_ui();
    if (ui.active) ui.active->dragging_temp_slider = false;
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.color_grid = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.color_grid, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.color_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.color_grid, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.color_grid, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.color_grid, LV_OBJ_FLAG_SCROLLABLE);
  light_control_rebuild_color_grid(ctx);

  light_control_layout_modal(ctx);
  light_control_set_modal_value(ctx, light_control_display_pct(ctx));
  light_control_set_temp_modal_value(ctx, ctx->current_kelvin);
  light_control_apply_modal_power(ctx);
  light_control_apply_tab_visibility();
  lv_obj_move_foreground(ui.overlay);
}

inline void light_control_rebuild_color_grid(LightControlCtx *ctx) {
  LightControlModalUi &ui = light_control_modal_ui();
  if (!ctx || ui.active != ctx || !ui.color_grid) return;
  lv_obj_clean(ui.color_grid);

  static constexpr LightColorPreset RGB_PRESETS[16] = {
    {0xFFE6B3}, {0xFFFFFF}, {0xDCEBFF}, {0xFFD400},
    {0xFF7A00}, {0xFF2600}, {0xFF1744}, {0xFF4081},
    {0xD500F9}, {0x7C4DFF}, {0x2979FF}, {0x00E5FF},
    {0x00B8D4}, {0x00C853}, {0x7ED321}, {0xAEEA00},
  };
  static constexpr LightColorPreset TEMPERATURE_PRESETS[4] = {
    {0xFF972C, 0},
    {0xF8D7BC, 30},
    {0xDDE6FF, 68},
    {0xBDD1F7, 100},
  };
  const LightColorPreset *presets = light_control_use_temperature_swatches(ctx)
    ? TEMPERATURE_PRESETS
    : RGB_PRESETS;
  uint32_t count = light_control_swatch_count(ctx);
  for (uint32_t i = 0; i < count; i++) {
    lv_obj_t *swatch = lv_btn_create(ui.color_grid);
    if (!swatch) continue;
    lv_obj_set_style_bg_color(swatch, lv_color_hex(presets[i].color), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(swatch, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(swatch, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(swatch, 0, LV_PART_MAIN);
    control_modal_apply_pressed_fill(swatch);
    lv_obj_clear_flag(swatch, LV_OBJ_FLAG_SCROLLABLE);
    LightColorPresetClick *click = new LightColorPresetClick();
    click->color = presets[i].color;
    click->kelvin_pct = presets[i].kelvin_pct;
    lv_obj_add_event_cb(swatch, [](lv_event_t *e) {
      LightColorPresetClick *click = static_cast<LightColorPresetClick *>(lv_event_get_user_data(e));
      LightControlModalUi &ui = light_control_modal_ui();
      if (!click || !ui.active || !ui.active->available) return;
      if (click->kelvin_pct >= 0) {
        int pct = slider_clamp_pct(click->kelvin_pct);
        ui.active->current_kelvin = light_control_pct_to_kelvin(ui.active, pct);
        light_control_set_temp_modal_value(ui.active, ui.active->current_kelvin);
        send_light_temp_action(ui.active->entity_id, pct, ui.active->kelvin_min, ui.active->kelvin_max);
      } else {
        send_light_rgb_action(ui.active->entity_id, click->color);
      }
    }, LV_EVENT_CLICKED, click);
  }
  light_control_layout_modal(ctx);
}

inline void setup_light_control_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, light_control_icon_off(p));
  lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Light") : p.label.c_str());
  apply_push_button_transition(s.btn);
}

inline LightControlCtx *create_light_control_context(
    const BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    const lv_font_t *number_font,
    const lv_font_t *label_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  LightControlCtx *ctx = new LightControlCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label;
  ctx->options = p.options;
  ctx->accent_color = accent_color;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->icon_off_glyph = light_control_icon_off(p);
  ctx->icon_on_glyph = light_control_icon_on(p);
  ctx->label_font = label_font;
  ctx->number_font = number_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}

inline void subscribe_light_control_state(LightControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        ctx->available = !ha_state_unavailable_ref(state);
        ctx->on = is_entity_on_ref(state);
        light_control_apply_card_visual(ctx);
        light_control_set_modal_value(ctx, light_control_display_pct(ctx));
        light_control_apply_modal_power(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("brightness"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        int pct = 0;
        if (!slider_parse_light_brightness_pct(value, pct)) return;
        ctx->current_pct = pct;
        light_control_set_modal_value(ctx, light_control_display_pct(ctx));
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("color_temp_kelvin"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        std::string s = string_ref_limited(value, 16);
        if (s.empty()) return;
        int kelvin = atoi(s.c_str());
        if (kelvin <= 0) return;
        ctx->current_kelvin = kelvin;
        light_control_set_temp_modal_value(ctx, kelvin);
        LightControlModalUi &ui = light_control_modal_ui();
        if (ui.active == ctx) {
          int pct = light_control_kelvin_to_pct(ctx, kelvin);
          light_control_update_slider_fill(
            ui.temp_slider, ui.temp_slider_fill, ui.temp_slider_handle, pct,
            kelvin_to_fill_color(kelvin, ctx->kelvin_min, ctx->kelvin_max));
          light_control_update_slider_handle(ui.temp_slider, ui.temp_slider_handle, pct);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("supported_color_modes"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        std::string modes = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
        for (char &c : modes) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        if (modes.empty() || modes == "unknown" || modes == "unavailable") return;
        bool rgb_supported =
          modes.find("hs") != std::string::npos ||
          modes.find("xy") != std::string::npos ||
          modes.find("rgb") != std::string::npos;
        bool temperature_supported = modes.find("color_temp") != std::string::npos;
        bool show_rgb_palette = rgb_supported || !temperature_supported;
        bool changed = !ctx->color_modes_known || ctx->rgb_color_supported != show_rgb_palette;
        ctx->color_modes_known = true;
        ctx->rgb_color_supported = show_rgb_palette;
        LightControlModalUi &ui = light_control_modal_ui();
        if (changed && ui.active == ctx && ui.color_grid) {
          light_control_rebuild_color_grid(ctx);
          light_control_apply_tab_visibility();
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef value) {
        ctx->friendly_name = string_ref_limited(value, HA_STATE_TEXT_MAX_LEN);
        light_control_apply_card_visual(ctx);
      })
  );
}

inline lv_coord_t media_volume_card_radius(MediaVolumeCtx *ctx) {
  return control_modal_card_radius(ctx ? ctx->btn : nullptr);
}

inline ControlModalLayout media_volume_step_button_layout(const ControlModalLayout &layout) {
  ControlModalLayout controls_layout = layout;
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    controls_layout.btn_size = control_modal_scaled_px(
      MEDIA_VOLUME_COMPACT_PORTRAIT_BUTTON_REF_PX, layout.short_side);
    controls_layout.controls_center_y = layout.arc_size / 2 -
      controls_layout.btn_size / 2 - layout.inset +
      control_modal_controls_down_px(layout);
  }
  return controls_layout;
}

// Move vertical card-slider endpoints away from the screen bezel without
// shrinking the slider's touch area or its separate full-card fill overlay.
inline lv_coord_t slider_vertical_edge_inset(lv_coord_t height) {
  if (height <= 0) return 0;

  lv_coord_t inset = (lv_coord_t)(((int32_t)height * 8 + 50) / 100);
  if (inset < 8) inset = 8;
  if (inset > 24) inset = 24;

  lv_coord_t small_slider_limit = height / 4;
  if (inset > small_slider_limit) inset = small_slider_limit;
  return inset;
}

inline void slider_fit_to_button(lv_obj_t *slider, lv_obj_t *btn, bool horizontal) {
  if (!slider || !btn) return;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;

  if (horizontal) {
    lv_coord_t h = bh >= bw ? bw - 1 : bh;
    if (h < 1) h = 1;
    lv_obj_set_size(slider, bw, h);
  } else {
    lv_coord_t w = bw >= bh ? bh - 1 : bw;
    if (w < 1) w = 1;
    lv_obj_set_size(slider, w, bh);
    lv_coord_t edge_inset = slider_vertical_edge_inset(bh);
    lv_obj_set_style_pad_top(slider, edge_inset, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(slider, edge_inset, LV_PART_MAIN);
  }
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
}

// Resize the colored fill overlay to reflect the current slider percentage
inline void slider_update_fill(lv_obj_t *fill, lv_obj_t *btn, int pct, bool horizontal,
                               bool inverted, lv_coord_t r,
                               bool keep_min_horizontal_handle = false) {
  if (!fill || !btn) return;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;
  lv_obj_set_style_radius(fill, r, LV_PART_MAIN);
  if (horizontal) {
    lv_coord_t w = (lv_coord_t)((int32_t)bw * pct / 100);
    if (keep_min_horizontal_handle && r > 0) {
      lv_coord_t min_w = r * 2;
      if (min_w > bw) min_w = bw;
      if (w < min_w) w = min_w;
    }
    lv_obj_set_size(fill, w, bh);
    lv_obj_align(fill, inverted ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
  } else {
    lv_coord_t h = (lv_coord_t)((int32_t)bh * pct / 100);
    lv_obj_set_size(fill, bw, h);
    lv_obj_align(fill, inverted ? LV_ALIGN_TOP_MID : LV_ALIGN_BOTTOM_MID, 0, 0);
  }
}

inline void slider_horizontal_track_geometry(lv_obj_t *btn, lv_coord_t &x,
                                             lv_coord_t &y, lv_coord_t &w,
                                             lv_coord_t &h) {
  x = y = 0;
  w = h = 0;
  if (!btn) return;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;

  w = (lv_coord_t)((int32_t)bw * 84 / 100);
  h = (lv_coord_t)(((int32_t)bh * 15 + 100) / 200);
  if (w < 1) w = 1;
  if (h < 4) h = 4;
  if (h > 12) h = 12;
  x = (bw - w) / 2;
  lv_coord_t bottom_pad = (lv_coord_t)((int32_t)bh * 10 / 100);
  if (bottom_pad < 8) bottom_pad = 8;
  y = -bottom_pad;
}

inline void slider_update_horizontal_track_bg(lv_obj_t *track, lv_obj_t *btn) {
  if (!track || !btn) return;
  lv_coord_t x, y, w, h;
  slider_horizontal_track_geometry(btn, x, y, w, h);
  if (w <= 0 || h <= 0) return;
  lv_obj_set_style_radius(track, h / 2, LV_PART_MAIN);
  lv_obj_set_size(track, w, h);
  lv_obj_align(track, LV_ALIGN_BOTTOM_LEFT, x, y);
}

inline void slider_update_horizontal_track_fill(lv_obj_t *fill, lv_obj_t *btn, int pct) {
  if (!fill || !btn) return;
  lv_coord_t x, y, track_w, track_h;
  slider_horizontal_track_geometry(btn, x, y, track_w, track_h);
  if (track_w <= 0 || track_h <= 0) return;
  lv_coord_t fill_w = (lv_coord_t)((int32_t)track_w * pct / 100);
  if (fill_w < 0) fill_w = 0;

  lv_obj_set_style_radius(fill, track_h / 2, LV_PART_MAIN);
  lv_obj_set_size(fill, fill_w, track_h);
  lv_obj_align(fill, LV_ALIGN_BOTTOM_LEFT, x, y);
}

inline void slider_update_ctx_fill(SliderCtx *c, lv_obj_t *btn, int pct) {
  if (!c || !c->fill || !btn) return;
  if (c->media_position && c->media_track_bg) {
    slider_update_horizontal_track_bg(c->media_track_bg, btn);
    slider_update_horizontal_track_fill(c->fill, btn, pct);
  } else {
    slider_update_fill(c->fill, btn, pct, c->horizontal, c->inverted, c->radius,
                       c->media_position);
  }
}

inline void slider_prime_media_position_fill(SliderCtx *c, lv_obj_t *btn) {
  if (!c || !c->media_position || !c->fill || !btn || c->media_track_bg) return;
  lv_coord_t min_w = c->radius > 0 ? c->radius * 2 : 1;
  lv_coord_t btn_w = lv_obj_get_width(btn);
  if (btn_w > 0 && min_w > btn_w) min_w = btn_w;
  lv_obj_set_style_radius(c->fill, c->radius, LV_PART_MAIN);
  lv_obj_set_width(c->fill, min_w);
  lv_obj_set_height(c->fill, lv_pct(100));
  lv_obj_align(c->fill, c->inverted ? LV_ALIGN_RIGHT_MID : LV_ALIGN_LEFT_MID, 0, 0);
}

inline void slider_refresh_geometry(lv_obj_t *slider) {
  if (!slider) return;
  SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *btn = lv_obj_get_parent(slider);
  if (!c || !btn) return;

  slider_fit_to_button(slider, btn, c->horizontal);
  int val = lv_slider_get_value(slider);
  int fill_val = c->inverted ? 100 - val : val;
  slider_update_ctx_fill(c, btn, fill_val);
}

inline void slider_deferred_geometry_refresh_cb(lv_timer_t *timer) {
  if (!timer) return;
  lv_obj_t *slider = (lv_obj_t *)lv_timer_get_user_data(timer);
  slider_refresh_geometry(slider);
  lv_timer_del(timer);
}

inline void slider_bind_geometry_refresh(lv_obj_t *btn, lv_obj_t *slider) {
  if (!btn || !slider) return;
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_user_data(e);
    slider_refresh_geometry(sl);
  }, LV_EVENT_SIZE_CHANGED, slider);
  lv_timer_create(slider_deferred_geometry_refresh_cb, 1, slider);
}

// Create an invisible LVGL slider with a colored fill overlay inside a button
inline lv_obj_t *setup_slider_widget(lv_obj_t *btn, uint32_t on_color, bool horizontal) {
  if (!btn) return nullptr;
  lv_obj_set_style_pad_all(btn, 0,
    static_cast<lv_style_selector_t>(LV_PART_MAIN));
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *fill = lv_obj_create(btn);
  if (!fill) return nullptr;
  lv_obj_set_size(fill, 0, 0);
  lv_obj_set_style_bg_color(fill, lv_color_hex(on_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *slider = lv_slider_create(btn);
  if (!slider) {
    lv_obj_del(fill);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    return nullptr;
  }
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);

  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(LV_PART_INDICATOR));
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_border_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_shadow_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_pad_all(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_width(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));
  lv_obj_set_style_height(slider, 0,
    static_cast<lv_style_selector_t>(LV_PART_KNOB));

  lv_obj_move_to_index(fill, 0);
  lv_obj_move_to_index(slider, 1);

  return slider;
}

inline bool slider_has_alt_icon(const std::string &type, const std::string &icon_on) {
  return brightness_slider_type(type) || type == "cover" || (!icon_on.empty() && icon_on != "Auto");
}

inline const char *slider_icon_off(const std::string &type, const std::string &entity_id,
                                   const std::string &icon) {
  if (type == "cover" && (icon.empty() || icon == "Auto"))
    return find_icon("Blinds");
  if (icon.empty() || icon == "Auto")
    return domain_default_icon(entity_id.substr(0, entity_id.find('.')));
  return find_icon(icon.c_str());
}

inline const char *slider_icon_on(const std::string &type, const std::string &entity_id,
                                  const std::string &icon, const std::string &icon_on) {
  if (type == "cover" && (icon_on.empty() || icon_on == "Auto"))
    return find_icon("Blinds Open");
  if (brightness_slider_type(type) && (icon_on.empty() || icon_on == "Auto"))
    return slider_icon_off(type, entity_id, icon);
  return find_icon(icon_on.c_str());
}

inline int slider_clamp_pct(int pct);
inline bool slider_parse_pct(esphome::StringRef val, int &pct);
inline bool slider_attribute_missing_ref(esphome::StringRef val);

enum class CoverControlTab : uint8_t {
  CONTROLS = 0,
  POSITION = 1,
  TILT = 2,
  PRESETS = 3,
};

struct CoverControlVisibleTabs {
  CoverControlTab tabs[4] = {
    CoverControlTab::POSITION,
    CoverControlTab::CONTROLS,
    CoverControlTab::TILT,
    CoverControlTab::PRESETS,
  };
  uint8_t count = 0;

  bool contains(CoverControlTab tab) const {
    for (uint8_t i = 0; i < count; i++) {
      if (tabs[i] == tab) return true;
    }
    return false;
  }

  void add(CoverControlTab tab) {
    if (count >= 4 || contains(tab)) return;
    tabs[count++] = tab;
  }
};

struct CoverControlCtx {
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  std::string options;
  int current_position = 0;
  int current_tilt = 0;
  bool current_position_known = false;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  lv_obj_t *btn = nullptr;
  lv_obj_t *card_slider = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const char *icon_closed_glyph = nullptr;
  const char *icon_open_glyph = nullptr;
  const lv_font_t *option_title_font = nullptr;
  const lv_font_t *option_value_font = nullptr;
  const lv_font_t *option_menu_font = nullptr;
  const lv_font_t *card_icon_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  bool available = true;
  bool dragging_position = false;
  bool dragging_tilt = false;
  bool updating_position = false;
  bool updating_tilt = false;
  bool supported_features_known = false;
  int supported_features = 0;
  bool supports_position = true;
  bool supports_open = true;
  bool supports_close = true;
  bool supports_stop = true;
  bool supports_open_tilt = false;
  bool supports_close_tilt = false;
  bool supports_stop_tilt = false;
  bool supports_tilt = false;
};

struct CoverControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *controls_tab = nullptr;
  lv_obj_t *position_tab = nullptr;
  lv_obj_t *tilt_tab = nullptr;
  lv_obj_t *presets_tab = nullptr;
  lv_obj_t *controls_box = nullptr;
  lv_obj_t *up_btn = nullptr;
  lv_obj_t *stop_btn = nullptr;
  lv_obj_t *down_btn = nullptr;
  lv_obj_t *presets_box = nullptr;
  lv_obj_t *preset_btns[5] = {nullptr, nullptr, nullptr, nullptr, nullptr};
  lv_obj_t *position_slider = nullptr;
  lv_obj_t *position_fill = nullptr;
  lv_obj_t *position_handle = nullptr;
  lv_obj_t *tilt_slider = nullptr;
  lv_obj_t *tilt_handle = nullptr;
  CoverControlCtx *active = nullptr;
  CoverControlTab tab = CoverControlTab::POSITION;
};

inline CoverControlModalUi &cover_control_modal_ui() {
  static CoverControlModalUi ui;
  return ui;
}

inline bool cover_control_tab_from_token(const std::string &value, CoverControlTab &tab) {
  if (value == "position") {
    tab = CoverControlTab::POSITION;
    return true;
  }
  if (value == "controls") {
    tab = CoverControlTab::CONTROLS;
    return true;
  }
  if (value == "tilt") {
    tab = CoverControlTab::TILT;
    return true;
  }
  if (value == "presets") {
    tab = CoverControlTab::PRESETS;
    return true;
  }
  return false;
}

inline bool cover_control_supports_position(CoverControlCtx *ctx) {
  return !ctx || !ctx->supported_features_known || ctx->supports_position;
}

inline bool cover_control_command_available(CoverControlCtx *ctx, const std::string &mode) {
  if (!ctx || !ctx->supported_features_known) return true;
  if (mode == "open") return ctx->supports_open || ctx->supports_open_tilt;
  if (mode == "close") return ctx->supports_close || ctx->supports_close_tilt;
  if (mode == "stop") return ctx->supports_stop || ctx->supports_stop_tilt;
  return false;
}

inline bool cover_control_supports_controls(CoverControlCtx *ctx) {
  return cover_control_command_available(ctx, "open") ||
         cover_control_command_available(ctx, "close") ||
         cover_control_command_available(ctx, "stop");
}

inline bool cover_control_command_uses_tilt(CoverControlCtx *ctx, const std::string &mode) {
  if (!ctx || !ctx->supported_features_known) return false;
  if (mode == "open") return !ctx->supports_open && ctx->supports_open_tilt;
  if (mode == "close") return !ctx->supports_close && ctx->supports_close_tilt;
  if (mode == "stop") return !ctx->supports_stop && ctx->supports_stop_tilt;
  return false;
}

inline CoverControlVisibleTabs cover_control_visible_tabs(CoverControlCtx *ctx) {
  CoverControlVisibleTabs visible;
  std::string value = cfg_option_value(ctx ? ctx->options : "", COVER_CONTROL_TABS_OPTION);
  if (value.empty()) value = card_runtime_cover_control_tabs_default();

  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find('|', start);
    std::string token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
    CoverControlTab tab = CoverControlTab::POSITION;
    if (cover_control_tab_from_token(token, tab) &&
        (tab != CoverControlTab::POSITION || cover_control_supports_position(ctx)) &&
        (tab != CoverControlTab::CONTROLS || cover_control_supports_controls(ctx)) &&
        (tab != CoverControlTab::TILT || !ctx || ctx->supports_tilt) &&
        (tab != CoverControlTab::PRESETS || cover_control_supports_position(ctx))) {
      visible.add(tab);
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (visible.count == 0) {
    if (ctx && ctx->supports_tilt) visible.add(CoverControlTab::TILT);
    else if (cover_control_supports_controls(ctx)) visible.add(CoverControlTab::CONTROLS);
    else visible.add(CoverControlTab::POSITION);
  }
  return visible;
}

inline bool cover_control_tab_visible(CoverControlCtx *ctx, CoverControlTab tab) {
  CoverControlVisibleTabs tabs = cover_control_visible_tabs(ctx);
  return tabs.contains(tab);
}

inline CoverControlTab cover_control_first_visible_tab(CoverControlCtx *ctx) {
  CoverControlVisibleTabs tabs = cover_control_visible_tabs(ctx);
  return tabs.count == 0 ? CoverControlTab::POSITION : tabs.tabs[0];
}

inline void cover_control_ensure_visible_tab(CoverControlCtx *ctx) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (cover_control_tab_visible(ctx, ui.tab)) return;
  ui.tab = cover_control_first_visible_tab(ctx);
}

inline lv_obj_t *cover_control_tab_button(CoverControlModalUi &ui, CoverControlTab tab) {
  switch (tab) {
    case CoverControlTab::CONTROLS: return ui.controls_tab;
    case CoverControlTab::POSITION: return ui.position_tab;
    case CoverControlTab::TILT: return ui.tilt_tab;
    case CoverControlTab::PRESETS: return ui.presets_tab;
  }
  return nullptr;
}

inline std::string cover_control_title(CoverControlCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Cover"));
  if (!ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Cover"));
}

inline void cover_control_update_card_slider(CoverControlCtx *ctx,
                                             const std::string &state_text = "") {
  if (!ctx || !ctx->card_slider) return;
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(ctx->card_slider);
  if (!sctx) return;
  int position = 100;
  if (ctx->current_position_known) {
    position = ctx->current_position;
  } else if (!state_text.empty() && cover_toggle_state_is_active(state_text)) {
    position = 0;
  }
  slider_set_value_safe(ctx->card_slider, position);
  slider_update_ctx_fill(sctx, ctx->btn, 100 - slider_clamp_pct(position));
}

inline void cover_control_apply_card_visual(CoverControlCtx *ctx,
                                            const std::string &state_text = "") {
  if (!ctx || !ctx->btn) return;
  cover_control_update_card_slider(ctx, state_text);
  bool active = ctx->current_position_known
    ? slider_clamp_pct(ctx->current_position) < 100
    : (!state_text.empty() ? cover_toggle_state_is_active(state_text) : ctx->current_position < 100);
  set_card_checked_state(ctx->btn, ctx->available && active);
  if (ctx->icon_lbl) {
    bool open_icon = ctx->current_position_known
      ? slider_clamp_pct(ctx->current_position) == 100
      : (!state_text.empty() ? garage_state_uses_open_icon(state_text) : ctx->current_position == 100);
    lv_label_set_text(ctx->icon_lbl, open_icon ? ctx->icon_open_glyph : ctx->icon_closed_glyph);
  }
  if (ctx->label_lbl) {
    std::string title = cover_control_title(ctx);
    lv_label_set_text(ctx->label_lbl, title.c_str());
  }
}

inline void cover_control_style_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
  if (!btn) return;
  (void) accent_color;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? DARK_TEXT_PRIMARY : SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(
      label, lv_color_hex(active ? TERTIARY_GREY : DARK_TEXT_PRIMARY), LV_PART_MAIN);
  }
}

inline void cover_control_apply_tab_visibility() {
  CoverControlModalUi &ui = cover_control_modal_ui();
  CoverControlCtx *ctx = ui.active;
  if (!ctx) return;
  cover_control_ensure_visible_tab(ctx);
  CoverControlVisibleTabs visible_tabs = cover_control_visible_tabs(ctx);
  bool show_tab_bar = visible_tabs.count > 1;
  bool show_controls = cover_control_supports_controls(ctx) && ui.tab == CoverControlTab::CONTROLS;
  bool show_position = cover_control_supports_position(ctx) && ui.tab == CoverControlTab::POSITION;
  bool show_tilt = ctx->supports_tilt && ui.tab == CoverControlTab::TILT;
  bool show_presets = cover_control_supports_position(ctx) && ui.tab == CoverControlTab::PRESETS;
  if (ui.tab_row) {
    if (show_tab_bar) lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tab_row, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.controls_tab) {
    if (show_tab_bar && visible_tabs.contains(CoverControlTab::CONTROLS)) lv_obj_clear_flag(ui.controls_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.position_tab) {
    if (show_tab_bar && visible_tabs.contains(CoverControlTab::POSITION)) lv_obj_clear_flag(ui.position_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.position_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.tilt_tab) {
    if (show_tab_bar && visible_tabs.contains(CoverControlTab::TILT)) lv_obj_clear_flag(ui.tilt_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tilt_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.presets_tab) {
    if (show_tab_bar && visible_tabs.contains(CoverControlTab::PRESETS)) lv_obj_clear_flag(ui.presets_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.presets_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.controls_box) {
    if (show_controls) lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.up_btn) {
    if (show_controls && cover_control_command_available(ctx, "open")) lv_obj_clear_flag(ui.up_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.up_btn, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.stop_btn) {
    if (show_controls && cover_control_command_available(ctx, "stop")) lv_obj_clear_flag(ui.stop_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.stop_btn, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.down_btn) {
    if (show_controls && cover_control_command_available(ctx, "close")) lv_obj_clear_flag(ui.down_btn, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.down_btn, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.position_slider) {
    if (show_position) lv_obj_clear_flag(ui.position_slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.position_slider, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.presets_box) {
    if (show_presets) lv_obj_clear_flag(ui.presets_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.presets_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.tilt_slider) {
    if (show_tilt) lv_obj_clear_flag(ui.tilt_slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tilt_slider, LV_OBJ_FLAG_HIDDEN);
  }
  cover_control_style_tab(ui.controls_tab, show_controls, ctx->accent_color);
  cover_control_style_tab(ui.position_tab, show_position, ctx->accent_color);
  cover_control_style_tab(ui.tilt_tab, show_tilt, ctx->accent_color);
  cover_control_style_tab(ui.presets_tab, show_presets, ctx->accent_color);
}

inline void cover_control_layout_modal(CoverControlCtx *ctx);
inline void cover_control_set_position_value(CoverControlCtx *ctx, int pct);

inline lv_obj_t *cover_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                 const lv_font_t *font,
                                                 CoverControlTab tab,
                                                 int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  apply_width_compensation(btn, width_compensation_percent);
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
    lv_obj_set_style_transform_zoom(label, 210, LV_PART_MAIN);
    light_control_center_icon_label(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    CoverControlTab tab = static_cast<CoverControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    CoverControlModalUi &ui = cover_control_modal_ui();
    ui.tab = tab;
    cover_control_apply_tab_visibility();
    cover_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *cover_control_create_wide_icon_button(lv_obj_t *parent, const char *icon,
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

inline lv_obj_t *cover_control_create_preset_button(lv_obj_t *parent, int pct,
                                                    const lv_font_t *icon_font,
                                                    const lv_font_t *label_font) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_size(btn, 118, 118);
  lv_obj_set_flex_grow(btn, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_top(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_column(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(btn, 8, LV_PART_MAIN);
  lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(btn, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_user_data(btn, reinterpret_cast<void *>(static_cast<uintptr_t>(pct)));

  lv_obj_t *icon = lv_label_create(btn);
  if (icon) {
    lv_label_set_text(icon, pct <= 50 ? find_icon("Blinds") : find_icon("Blinds Open"));
    lv_obj_set_style_text_color(icon, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (icon_font) lv_obj_set_style_text_font(icon, icon_font, LV_PART_MAIN);
    lv_obj_clear_flag(icon, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_obj_t *label = lv_label_create(btn);
  if (label) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(label, buf);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label, lv_pct(100));
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (label_font) lv_obj_set_style_text_font(label, label_font, LV_PART_MAIN);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
  }

  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    int pct = static_cast<int>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active || !ui.active->available || !cover_control_supports_position(ui.active)) return;
    ui.active->current_position_known = true;
    ui.active->current_position = slider_clamp_pct(pct);
    cover_control_set_position_value(ui.active, ui.active->current_position);
    cover_control_apply_card_visual(ui.active);
    send_slider_action(ui.active->entity_id, ui.active->current_position, false);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(pct)));

  return btn;
}

inline bool cover_control_uses_compact_portrait_option_grid(const ControlModalLayout &layout) {
  return control_modal_uses_compact_portrait_tuning(layout) &&
         !control_modal_uses_compact_control_tuning(layout) &&
         layout.sh > layout.sw;
}

inline bool cover_control_uses_wide_option_fit(const ControlModalLayout &layout) {
  return control_modal_uses_large_landscape_tuning(layout) ||
         control_modal_uses_wide_landscape_tuning(layout) ||
         cover_control_uses_compact_portrait_option_grid(layout);
}

inline lv_coord_t cover_control_option_tile_max(const ControlModalLayout &layout) {
  if (cover_control_uses_compact_portrait_option_grid(layout)) return 220;
  return control_modal_uses_wide_landscape_tuning(layout) ? 162 : 178;
}

inline lv_coord_t cover_control_option_tile_min(const ControlModalLayout &layout) {
  if (cover_control_uses_compact_portrait_option_grid(layout)) return 220;
  return control_modal_uses_wide_landscape_tuning(layout) ? 124 : 132;
}

inline lv_coord_t cover_control_option_tile_height(const ControlModalLayout &layout,
                                                   lv_coord_t tile_w) {
  return cover_control_uses_compact_portrait_option_grid(layout) ? 120 : tile_w;
}

inline const lv_font_t *cover_control_preset_icon_font(CoverControlCtx *ctx,
                                                       const ControlModalLayout &layout) {
  if (!ctx) return nullptr;
  if (cover_control_uses_compact_portrait_option_grid(layout) && ctx->card_icon_font)
    return ctx->card_icon_font;
  return ctx->icon_font;
}

inline lv_coord_t cover_control_home_grid_row_gap(const ControlModalLayout &layout) {
  ControlModalGridMetrics &metrics = control_modal_grid_metrics();
  if (metrics.page) {
    lv_obj_update_layout(metrics.page);
    lv_coord_t gap = lv_obj_get_style_pad_row(metrics.page, LV_PART_MAIN);
    if (gap > 0) return gap;
    gap = lv_obj_get_style_pad_column(metrics.page, LV_PART_MAIN);
    if (gap > 0) return gap;
  }
  lv_coord_t gap = control_modal_scaled_px(10, layout.short_side);
  return gap > 0 ? gap : 10;
}

inline lv_obj_t *cover_control_create_slider_handle(lv_obj_t *slider) {
  if (!slider) return nullptr;
  lv_obj_t *handle = lv_obj_create(slider);
  if (!handle) return nullptr;
  lv_obj_set_size(handle, 0, 0);
  lv_obj_set_style_bg_color(handle, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(handle, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(handle, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(handle, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(handle, 0, LV_PART_MAIN);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_SCROLLABLE);
  return handle;
}

inline lv_coord_t cover_control_slider_handle_inset(lv_obj_t *slider) {
  if (!slider) return 18;
  lv_coord_t inset = lv_obj_get_style_radius(slider, LV_PART_MAIN) * 3 / 4;
  if (inset < 16) inset = 16;
  if (inset > 28) inset = 28;
  return inset;
}

inline uint32_t cover_control_slider_fill_color(CoverControlCtx *ctx, int pct) {
  return slider_clamp_pct(pct) == 0
    ? (ctx ? ctx->secondary_color : SECONDARY_GREY)
    : (ctx ? ctx->accent_color : DEFAULT_SLIDER_COLOR);
}

inline void cover_control_update_slider_fill_color(lv_obj_t *slider,
                                                   CoverControlCtx *ctx,
                                                   int pct) {
  if (!slider) return;
  lv_obj_set_style_bg_color(
    slider, lv_color_hex(cover_control_slider_fill_color(ctx, pct)), LV_PART_INDICATOR);
}

inline void cover_control_update_slider_handle(lv_obj_t *slider, lv_obj_t *handle, int pct) {
  if (!slider || !handle) return;
  lv_coord_t width = lv_obj_get_width(slider);
  lv_coord_t height = lv_obj_get_height(slider);
  if (width <= 0 || height <= 0) return;
  lv_coord_t handle_w = width * 3 / 5;
  if (handle_w < 20) handle_w = 20;
  if (handle_w > width - 12) handle_w = width - 12;
  if (handle_w < 8) handle_w = 8;
  lv_coord_t handle_h = height / 70;
  if (handle_h < 5) handle_h = 5;
  if (handle_h > 8) handle_h = 8;
  lv_coord_t inset = cover_control_slider_handle_inset(slider);
  lv_coord_t travel = height - inset * 2 - handle_h;
  if (travel < 0) travel = 0;
  lv_coord_t y = inset + (lv_coord_t)((int32_t) travel * (100 - slider_clamp_pct(pct)) / 100);
  if (y > height - handle_h) y = height - handle_h;
  lv_obj_set_size(handle, handle_w, handle_h);
  lv_obj_set_style_radius(handle, handle_h / 2, LV_PART_MAIN);
  lv_obj_align(handle, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(handle);
}

inline void cover_control_update_position_handle(lv_obj_t *slider, lv_obj_t *handle,
                                                 lv_coord_t fill_h) {
  if (!slider || !handle) return;
  lv_coord_t width = lv_obj_get_width(slider);
  lv_coord_t height = lv_obj_get_height(slider);
  if (width <= 0 || height <= 0) return;
  lv_coord_t handle_w = width * 3 / 5;
  if (handle_w < 20) handle_w = 20;
  if (handle_w > width - 12) handle_w = width - 12;
  if (handle_w < 8) handle_w = 8;
  lv_coord_t handle_h = height / 70;
  if (handle_h < 5) handle_h = 5;
  if (handle_h > 8) handle_h = 8;
  lv_coord_t inset = cover_control_slider_handle_inset(slider);
  lv_coord_t y = fill_h - inset - handle_h;
  if (y < inset) y = inset;
  if (y > height - inset - handle_h) y = height - inset - handle_h;
  if (y > height - handle_h) y = height - handle_h;
  lv_obj_set_size(handle, handle_w, handle_h);
  lv_obj_set_style_radius(handle, handle_h / 2, LV_PART_MAIN);
  lv_obj_align(handle, LV_ALIGN_TOP_MID, 0, y);
  lv_obj_clear_flag(handle, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(handle);
}

inline void cover_control_layout_slider(lv_obj_t *slider, lv_coord_t width,
                                        lv_coord_t height, lv_coord_t center_y) {
  if (!slider) return;
  lv_obj_set_size(slider, width, height);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, center_y);
  lv_coord_t slider_radius = width / 5;
  if (slider_radius < 18) slider_radius = 18;
  if (slider_radius > 34) slider_radius = 34;
  lv_obj_set_style_radius(slider, slider_radius, LV_PART_MAIN);
  lv_obj_set_style_clip_corner(slider, true, LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
  lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
}

inline void cover_control_update_position_fill(int position_pct) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ui.position_slider || !ui.position_fill) return;
  int fill_pct = 100 - slider_clamp_pct(position_pct);
  lv_coord_t width = lv_obj_get_width(ui.position_slider);
  lv_coord_t height = lv_obj_get_height(ui.position_slider);
  if (width <= 0 || height <= 0) return;
  lv_obj_set_style_bg_color(
    ui.position_fill, lv_color_hex(cover_control_slider_fill_color(ui.active, fill_pct)),
    LV_PART_MAIN);
  lv_coord_t fill_h = (lv_coord_t)((int32_t) height * fill_pct / 100);
  lv_coord_t min_handle_cap = cover_control_slider_handle_inset(ui.position_slider) * 2 + 8;
  if (fill_h < min_handle_cap) fill_h = min_handle_cap;
  if (fill_h > height) fill_h = height;
  lv_obj_set_size(ui.position_fill, width, fill_h);
  lv_obj_set_style_radius(ui.position_fill, 0, LV_PART_MAIN);
  lv_obj_align(ui.position_fill, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_move_foreground(ui.position_fill);
  cover_control_update_position_handle(ui.position_slider, ui.position_handle, fill_h);
}

inline void cover_control_layout_modal(CoverControlCtx *ctx) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ctx || !ui.panel) return;
  cover_control_ensure_visible_tab(ctx);
  CoverControlVisibleTabs visible_tabs = cover_control_visible_tabs(ctx);
  cover_control_apply_tab_visibility();
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);

  int tab_count = static_cast<int>(visible_tabs.count);
  bool show_tab_bar = tab_count > 1;
  ControlModalTabLayout tabs_layout = control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);
  control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);
  for (int i = 0; show_tab_bar && i < tab_count; i++) {
    lv_obj_t *tab_btn = cover_control_tab_button(ui, visible_tabs.tabs[i]);
    if (!tab_btn) continue;
    bool active = (visible_tabs.tabs[i] == ui.tab);
    control_modal_layout_tab_button(
      tab_btn, layout, tabs_layout, i, active, ctx->width_compensation_percent);
  }

  const espcontrol::modal::ContentLayout content = control_modal_calc_content_layout(
    layout, tabs_layout, show_tab_bar, 160);
  lv_coord_t content_top = content.top;
  lv_coord_t content_h = content.height;
  lv_coord_t content_center_y = content.center_y;
  lv_coord_t content_w = control_modal_home_card_width(ctx->btn, layout);
  cover_control_layout_slider(ui.position_slider, content_w, content_h, content_center_y);
  lv_obj_update_layout(ui.panel);
  cover_control_update_position_fill(ctx->current_position);
  cover_control_layout_slider(ui.tilt_slider, content_w, content_h, content_center_y);
  lv_obj_update_layout(ui.panel);
  cover_control_update_slider_fill_color(ui.tilt_slider, ctx, ctx->current_tilt);
  cover_control_update_slider_handle(ui.tilt_slider, ui.tilt_handle, ctx->current_tilt);

  if (ui.controls_box) {
    lv_coord_t box_w = layout.panel_w - layout.inset * 3;
    lv_coord_t box_h = content_h;
    lv_obj_set_size(ui.controls_box, box_w, box_h);
    lv_obj_align(ui.controls_box, LV_ALIGN_CENTER, 0, content_center_y);
    lv_coord_t gap = cover_control_home_grid_row_gap(layout);
    lv_obj_t *buttons[3] = {nullptr, nullptr, nullptr};
    int button_count = 0;
    if (cover_control_command_available(ctx, "open")) buttons[button_count++] = ui.up_btn;
    if (cover_control_command_available(ctx, "stop")) buttons[button_count++] = ui.stop_btn;
    if (cover_control_command_available(ctx, "close")) buttons[button_count++] = ui.down_btn;
    if (button_count > 0) {
      lv_coord_t total_gap = gap * (button_count - 1);
      lv_coord_t btn_size = (box_w - total_gap) / button_count;
      if (btn_size > box_h) btn_size = box_h;
      if (btn_size < 56) btn_size = 56;
      lv_coord_t total_w = btn_size * button_count + total_gap;
      lv_coord_t start_x = (box_w - total_w) / 2;
      if (start_x < 0) start_x = 0;
      lv_coord_t button_radius = control_modal_card_radius(ctx->btn);
      for (int i = 0; i < button_count; i++) {
        if (!buttons[i]) continue;
        lv_obj_set_size(buttons[i], btn_size, btn_size);
        lv_obj_set_style_radius(buttons[i], button_radius, LV_PART_MAIN);
        lv_obj_align(buttons[i], LV_ALIGN_LEFT_MID, start_x + i * (btn_size + gap), 0);
        lv_obj_t *label = lv_obj_get_child(buttons[i], 0);
        if (label) lv_obj_center(label);
      }
    }
  }
  if (ui.presets_box) {
    lv_coord_t box_w = layout.panel_w;
    lv_coord_t box_h = layout.panel_h;
    lv_obj_set_size(ui.presets_box, box_w, box_h);
    lv_obj_set_pos(ui.presets_box, 0, 0);
    lv_coord_t tile_gap = control_modal_scaled_px(layout.short_side < 520 ? 10 : 12, layout.short_side);
    if (tile_gap < 8) tile_gap = 8;
    lv_coord_t content_w = layout.panel_w - layout.inset * 2;
    lv_coord_t tile_min_w = compensated_width(layout.short_side < 520 ? 138 : 168,
      ctx->width_compensation_percent);
    if (tile_min_w < 118) tile_min_w = 118;
    int column_count = content_w >= tile_min_w * 3 + tile_gap * 2 ? 3 : 2;
    if (content_w < tile_min_w * 2 + tile_gap) column_count = 1;
    lv_coord_t tile_w = (content_w - tile_gap * (column_count - 1)) / column_count;
    if (cover_control_uses_wide_option_fit(layout)) {
      lv_coord_t max_tile_w = cover_control_option_tile_max(layout);
      lv_coord_t min_tile_w = cover_control_option_tile_min(layout);
      int max_columns = control_modal_uses_wide_landscape_tuning(layout) ? 5 : 6;
      int fitted_columns = (content_w + tile_gap) / (max_tile_w + tile_gap);
      if (fitted_columns < 1) fitted_columns = 1;
      if (fitted_columns > max_columns) fitted_columns = max_columns;
      if (fitted_columns > 5) fitted_columns = 5;
      lv_coord_t fitted_tile_w = (content_w - tile_gap * (fitted_columns - 1)) / fitted_columns;
      tile_w = fitted_tile_w < max_tile_w ? fitted_tile_w : max_tile_w;
      if (tile_w < min_tile_w) tile_w = min_tile_w;
    }
    lv_coord_t tile_h = cover_control_option_tile_height(layout, tile_w);
    lv_obj_set_style_pad_top(ui.presets_box, content_top, LV_PART_MAIN);
    lv_obj_set_style_pad_left(ui.presets_box, layout.inset, LV_PART_MAIN);
    lv_obj_set_style_pad_right(ui.presets_box, layout.inset, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(ui.presets_box, layout.inset, LV_PART_MAIN);
    lv_obj_set_style_pad_row(ui.presets_box, tile_gap, LV_PART_MAIN);
    lv_obj_set_style_pad_column(ui.presets_box, tile_gap, LV_PART_MAIN);
    lv_obj_set_layout(ui.presets_box, LV_LAYOUT_FLEX);
    lv_obj_set_style_flex_flow(ui.presets_box, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN);
    lv_obj_set_style_flex_main_place(ui.presets_box, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_flex_cross_place(ui.presets_box, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_scroll_dir(ui.presets_box, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(ui.presets_box, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(ui.presets_box, LV_OBJ_FLAG_SCROLLABLE);
    for (int i = 0; i < 5; i++) {
      lv_obj_t *btn = ui.preset_btns[i];
      if (!btn) continue;
      int pct = static_cast<int>(reinterpret_cast<uintptr_t>(lv_obj_get_user_data(btn)));
      bool selected = ctx->current_position_known && slider_clamp_pct(ctx->current_position) == pct;
      uint32_t bg_color = selected ? ctx->accent_color : ctx->secondary_color;
      uint32_t text_color = selected ? DARK_TEXT_PRIMARY : readable_text_color_for_bg(bg_color);
      lv_obj_set_size(btn, tile_w, tile_h);
      lv_obj_set_style_radius(btn, control_modal_card_radius(ctx->btn), LV_PART_MAIN);
      lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
      lv_obj_t *icon = lv_obj_get_child(btn, 0);
      lv_obj_t *label = lv_obj_get_child(btn, 1);
      if (icon) {
        lv_obj_set_style_text_color(icon, lv_color_hex(text_color), LV_PART_MAIN);
        const lv_font_t *icon_font = cover_control_preset_icon_font(ctx, layout);
        if (icon_font) lv_obj_set_style_text_font(icon, icon_font, LV_PART_MAIN);
      }
      if (label) {
        lv_obj_set_style_text_color(label, lv_color_hex(text_color), LV_PART_MAIN);
        lv_obj_set_width(label, lv_pct(100));
      }
    }
    lv_obj_scroll_to_y(ui.presets_box, 0, LV_ANIM_OFF);
  }
  if (ui.tab_row) lv_obj_move_foreground(ui.tab_row);
  lv_obj_move_foreground(ui.back_btn);
}

inline void cover_control_hide_modal() {
  CoverControlModalUi &ui = cover_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = CoverControlModalUi();
  control_modal_delete_overlay(ControlModalKind::COVER_CONTROL, overlay);
}

inline void delete_cover_control_context(CoverControlCtx *ctx) {
  if (!ctx) return;
  if (cover_control_modal_ui().active == ctx) cover_control_hide_modal();
  if (ctx->btn && lv_obj_get_user_data(ctx->btn) == ctx) {
    lv_obj_set_user_data(ctx->btn, nullptr);
  }
  delete ctx;
}

inline void cover_control_set_slider_value(lv_obj_t *slider, bool &updating,
                                           bool dragging, int pct) {
  if (!slider || dragging) return;
  updating = true;
  lv_slider_set_value(slider, slider_clamp_pct(pct), LV_ANIM_OFF);
  updating = false;
}

inline void cover_control_set_position_value(CoverControlCtx *ctx, int pct) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  if (ctx->dragging_position) return;
  cover_control_set_slider_value(
    ui.position_slider, ctx->updating_position, ctx->dragging_position, pct);
  cover_control_update_position_fill(pct);
}

inline void cover_control_set_tilt_value(CoverControlCtx *ctx, int pct) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  cover_control_set_slider_value(ui.tilt_slider, ctx->updating_tilt, ctx->dragging_tilt, pct);
  cover_control_update_slider_fill_color(ui.tilt_slider, ctx, pct);
  if (!ctx->dragging_tilt) cover_control_update_slider_handle(ui.tilt_slider, ui.tilt_handle, pct);
}

inline void cover_control_apply_supported_features(CoverControlCtx *ctx,
                                                   bool known,
                                                   int features = 0) {
  if (!ctx) return;
  ctx->supported_features_known = known;
  ctx->supported_features = known ? features : 0;
  if (!known) {
    ctx->supports_position = true;
    ctx->supports_open = true;
    ctx->supports_close = true;
    ctx->supports_stop = true;
    ctx->supports_open_tilt = false;
    ctx->supports_close_tilt = false;
    ctx->supports_stop_tilt = false;
    ctx->supports_tilt = false;
  } else {
    ctx->supports_position = (features & COVER_SUPPORT_SET_POSITION) != 0;
    ctx->supports_open = (features & COVER_SUPPORT_OPEN) != 0;
    ctx->supports_close = (features & COVER_SUPPORT_CLOSE) != 0;
    ctx->supports_stop = (features & COVER_SUPPORT_STOP) != 0;
    ctx->supports_open_tilt = (features & COVER_SUPPORT_OPEN_TILT) != 0;
    ctx->supports_close_tilt = (features & COVER_SUPPORT_CLOSE_TILT) != 0;
    ctx->supports_stop_tilt = (features & COVER_SUPPORT_STOP_TILT) != 0;
    ctx->supports_tilt = (features & COVER_SUPPORT_SET_TILT_POSITION) != 0;
  }
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (ui.active == ctx) {
    cover_control_apply_tab_visibility();
    cover_control_layout_modal(ctx);
    cover_control_apply_tab_visibility();
  }
}

inline void cover_control_style_slider(lv_obj_t *slider, uint32_t accent_color) {
  if (!slider) return;
  lv_slider_set_range(slider, 0, 100);
  lv_obj_set_style_bg_color(slider, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(slider, lv_color_hex(accent_color), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(slider, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_outline_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
}

inline lv_obj_t *cover_control_create_position_fill(lv_obj_t *slider, uint32_t accent_color) {
  if (!slider) return nullptr;
  lv_obj_set_style_bg_opa(slider, LV_OPA_TRANSP, LV_PART_INDICATOR);
  lv_obj_t *fill = lv_obj_create(slider);
  if (!fill) return nullptr;
  lv_obj_set_size(fill, 0, 0);
  lv_obj_set_style_bg_color(fill, lv_color_hex(accent_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(fill, 0, LV_PART_MAIN);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);
  return fill;
}

inline void cover_control_open_modal(CoverControlCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::COVER_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, cover_control_hide_modal);
  CoverControlModalUi &ui = cover_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = cover_control_first_visible_tab(ctx);
  if (!ui.panel) return;

  ui.tab_row = control_modal_create_tab_row(ui.panel);
  ui.position_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("View Headline"), ctx->icon_font,
    CoverControlTab::POSITION, ctx->width_compensation_percent);
  ui.controls_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Swap Vertical"), ctx->icon_font,
    CoverControlTab::CONTROLS, ctx->width_compensation_percent);
  ui.tilt_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Swap Vertical"), ctx->icon_font,
    CoverControlTab::TILT, ctx->width_compensation_percent);
  ui.presets_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Roller Shade"), ctx->icon_font,
    CoverControlTab::PRESETS, ctx->width_compensation_percent);

  ui.controls_box = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.controls_box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_SCROLLABLE);
  ui.up_btn = cover_control_create_wide_icon_button(
    ui.controls_box, find_icon("Arrow Up"), ctx->icon_font);
  ui.stop_btn = cover_control_create_wide_icon_button(
    ui.controls_box, find_icon("Stop"), ctx->icon_font);
  ui.down_btn = cover_control_create_wide_icon_button(
    ui.controls_box, find_icon("Arrow Down"), ctx->icon_font);
  if (ui.up_btn) {
    lv_obj_add_event_cb(ui.up_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available &&
          cover_control_command_available(ui.active, "open")) {
        send_cover_command_action(
          ui.active->entity_id, "open",
          cover_control_command_uses_tilt(ui.active, "open"));
      }
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (ui.stop_btn) {
    lv_obj_add_event_cb(ui.stop_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available &&
          cover_control_command_available(ui.active, "stop")) {
        send_cover_command_action(
          ui.active->entity_id, "stop",
          cover_control_command_uses_tilt(ui.active, "stop"));
      }
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (ui.down_btn) {
    lv_obj_add_event_cb(ui.down_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available &&
          cover_control_command_available(ui.active, "close")) {
        send_cover_command_action(
          ui.active->entity_id, "close",
          cover_control_command_uses_tilt(ui.active, "close"));
      }
    }, LV_EVENT_CLICKED, nullptr);
  }

  ui.presets_box = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.presets_box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.presets_box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.presets_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.presets_box, 0, LV_PART_MAIN);
  const int preset_values[5] = {0, 25, 50, 75, 100};
  for (int i = 0; i < 5; i++) {
    ui.preset_btns[i] = cover_control_create_preset_button(
      ui.presets_box, preset_values[i],
      cover_control_preset_icon_font(ctx, control_modal_calc_layout(ctx->width_compensation_percent)),
      ctx->option_menu_font);
  }

  ui.position_slider = lv_slider_create(ui.panel);
  cover_control_style_slider(ui.position_slider, ctx->accent_color);
  ui.position_fill = cover_control_create_position_fill(ui.position_slider, ctx->accent_color);
  ui.position_handle = cover_control_create_slider_handle(ui.position_slider);
  lv_slider_set_value(ui.position_slider, slider_clamp_pct(ctx->current_position), LV_ANIM_OFF);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (ui.active) ui.active->dragging_position = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active || ui.active->updating_position) return;
    ui.active->dragging_position = true;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    ui.active->current_position_known = true;
    ui.active->current_position = lv_slider_get_value(slider);
    cover_control_update_position_fill(ui.active->current_position);
    cover_control_apply_card_visual(ui.active);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_position = false;
    if (!ui.active->available || !cover_control_supports_position(ui.active)) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->current_position_known = true;
    ui.active->current_position = pct;
    send_slider_action(ui.active->entity_id, pct, false);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (ui.active) ui.active->dragging_position = false;
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.tilt_slider = lv_slider_create(ui.panel);
  cover_control_style_slider(ui.tilt_slider, ctx->accent_color);
  ui.tilt_handle = cover_control_create_slider_handle(ui.tilt_slider);
  lv_slider_set_value(ui.tilt_slider, slider_clamp_pct(ctx->current_tilt), LV_ANIM_OFF);
  lv_obj_add_event_cb(ui.tilt_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (ui.active) ui.active->dragging_tilt = true;
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.tilt_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active || ui.active->updating_tilt) return;
    ui.active->dragging_tilt = true;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    ui.active->current_tilt = lv_slider_get_value(slider);
    cover_control_update_slider_fill_color(slider, ui.active, ui.active->current_tilt);
    cover_control_update_slider_handle(slider, ui.tilt_handle, ui.active->current_tilt);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.tilt_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_tilt = false;
    if (!ui.active->available) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->current_tilt = pct;
    send_slider_action(ui.active->entity_id, pct, true);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.tilt_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (ui.active) ui.active->dragging_tilt = false;
  }, LV_EVENT_PRESS_LOST, nullptr);

  cover_control_layout_modal(ctx);
  lv_obj_move_foreground(ui.overlay);
}

inline void setup_cover_modal_card(BtnSlot &s, const ParsedCfg &p) {
  setup_slider_visual(s, p, DEFAULT_SLIDER_COLOR, false);
  lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
  if (slider) lv_obj_clear_flag(slider, LV_OBJ_FLAG_CLICKABLE);
  if (s.btn) lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  apply_push_button_transition(s.btn);
}

inline CoverControlCtx *create_cover_control_context(
    const BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    uint32_t secondary_color,
    const lv_font_t *option_title_font,
    const lv_font_t *option_value_font,
    const lv_font_t *option_menu_font,
    const lv_font_t *card_icon_font,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  CoverControlCtx *ctx = new CoverControlCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label;
  ctx->options = p.options;
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->btn = s.btn;
  ctx->card_slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
  if (ctx->card_slider) {
    SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(ctx->card_slider);
    if (sctx) {
      sctx->available = ctx->available;
      if (sctx->fill) {
        lv_obj_set_style_bg_color(sctx->fill, lv_color_hex(accent_color), LV_PART_MAIN);
      }
    }
  }
  ctx->icon_lbl = s.icon_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->icon_closed_glyph = slider_icon_off(p.type, p.entity, p.icon);
  ctx->icon_open_glyph = slider_icon_on(p.type, p.entity, p.icon, p.icon_on);
  ctx->option_title_font = option_title_font;
  ctx->option_value_font = option_value_font;
  ctx->option_menu_font = option_menu_font ? option_menu_font : option_value_font;
  ctx->card_icon_font = card_icon_font;
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}

inline void subscribe_cover_control_state(CoverControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        cover_control_apply_card_visual(ctx, state_text);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("current_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        int pct = 0;
        if (!slider_parse_pct(val, pct)) return;
        ctx->current_position_known = true;
        ctx->current_position = pct;
        cover_control_set_position_value(ctx, pct);
        cover_control_apply_card_visual(ctx);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("current_tilt_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        int pct = 0;
        if (!slider_parse_pct(val, pct)) return;
        ctx->current_tilt = pct;
        cover_control_set_tilt_value(ctx, pct);
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("supported_features"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        int features = 0;
        if (!cover_parse_supported_features(val, features)) {
          cover_control_apply_supported_features(ctx, false);
          return;
        }
        cover_control_apply_supported_features(ctx, true, features);
      })
  );
  if (ctx->label.empty()) {
    ha_subscribe_attribute(
      ctx->entity_id, std::string("friendly_name"),
      std::function<void(esphome::StringRef)>(
        [ctx](esphome::StringRef value) {
          ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
          cover_control_apply_card_visual(ctx);
        })
    );
  }
}

inline void setup_cover_toggle_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Cover") : p.label.c_str());
}

inline void setup_cover_command_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Cover") : p.label.c_str());
  apply_push_button_transition(s.btn);
}

// Full slider button setup: visual + event handlers + HA action on release
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color,
                                bool interactive) {
  ESP_LOGI("slider", "Setup brightness slider for %s (%s)",
    p.entity.c_str(), p.type.c_str());
  setup_toggle_visual(s, p);
  if (p.type == "cover")
    lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));

  bool horizontal = false;
  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, horizontal);
  if (!slider) {
    ESP_LOGW("slider", "Slider setup failed for %s; falling back to toggle card",
      p.entity.c_str());
    setup_toggle_visual(s, p);
    if (s.btn) lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    if (s.icon_lbl) lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  ESP_LOGI("slider", "Slider object created for %s", p.entity.c_str());
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
  if (!fill) {
    ESP_LOGW("slider", "Slider fill missing for %s; falling back to toggle card",
      p.entity.c_str());
    setup_toggle_visual(s, p);
    if (s.btn) lv_obj_add_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
    if (s.icon_lbl) lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
    if (s.sensor_container) lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = horizontal;
  ctx->cover_tilt = p.type == "cover" && cover_tilt_mode(p.sensor);
  ctx->inverted = is_cover_entity(p.entity);
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(s.btn, slider);
  if (!interactive) lv_obj_clear_flag(slider, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (!sl) return;
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    int fill_val = c->inverted ? 100 - val : val;
    slider_update_ctx_fill(c, lv_obj_get_parent(sl), fill_val);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  if (interactive) {
    lv_obj_add_event_cb(slider, [](lv_event_t *e) {
      lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
      if (!sl) return;
      SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
      if (c && !c->entity_id.empty()) {
        if (!c->available) return;
        int val = lv_slider_get_value(sl);
        send_slider_action(c->entity_id, val, c->cover_tilt);
      }
    }, LV_EVENT_RELEASED, nullptr);
  }
}

inline int slider_clamp_pct(int pct) {
  return clamp_percent_value(pct);
}

inline bool slider_parse_pct(esphome::StringRef val, int &pct) {
  if (ha_state_unavailable_ref(val)) return false;
  float pct_f = 0.0f;
  if (!parse_float_ref(val, pct_f) || !std::isfinite(pct_f)) return false;
  pct = slider_clamp_pct((int)(pct_f + 0.5f));
  return true;
}

inline bool slider_parse_light_brightness_pct(esphome::StringRef val, int &pct) {
  if (ha_state_unavailable_ref(val)) return false;
  float bri = 0.0f;
  if (!parse_float_ref(val, bri) || !std::isfinite(bri)) return false;
  return light_brightness_to_percent(bri, pct);
}

inline bool slider_attribute_missing_ref(esphome::StringRef val) {
  std::string value = normalized_state_text(val);
  return value.empty() || value == "none" || value == "null" ||
         value == "unknown" || value == "unavailable";
}

inline bool slider_entity_is_light(const std::string &entity_id) {
  return entity_id.size() > 6 && entity_id.compare(0, 6, "light.") == 0;
}

inline void slider_set_value_safe(lv_obj_t *slider, int pct) {
  if (slider) lv_slider_set_value(slider, slider_clamp_pct(pct), LV_ANIM_OFF);
}

inline void slider_set_icon_safe(lv_obj_t *icon_lbl, const char *icon) {
  if (icon_lbl && icon) lv_label_set_text(icon_lbl, icon);
}

// Subscribe to HA state for a slider entity (light brightness, fan percentage, or cover position/tilt)
inline void subscribe_slider_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                  lv_obj_t *slider,
                                  bool has_icon_on,
                                  const char *icon_off, const char *icon_on,
                                  const std::string &entity_id,
                                  bool cover_tilt = false) {
  if (!slider) {
    ESP_LOGW("slider", "Skipping slider subscriptions for %s: slider missing",
      entity_id.c_str());
    return;
  }
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *fill = sctx ? sctx->fill : nullptr;
  bool horiz = sctx ? sctx->horizontal : false;
  bool inv = sctx ? sctx->inverted : false;
  lv_coord_t rad = sctx ? sctx->radius : 0;
  bool is_cover = is_cover_entity(entity_id);
  bool is_fan = is_fan_entity(entity_id);
  bool is_light = slider_entity_is_light(entity_id);
  if (is_cover) {
    ha_subscribe_attribute(
      entity_id, std::string(cover_tilt ? "current_tilt_position" : "current_position"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](esphome::StringRef val) {
          int pct = 0;
          if (slider_parse_pct(val, pct)) {
            slider_set_value_safe(slider, pct);
            slider_update_fill(fill, btn_ptr, inv ? 100 - pct : pct, horiz, inv, rad);
            if (has_icon_on) slider_set_icon_safe(icon_lbl, pct > 0 ? icon_on : icon_off);
          }
        })
    );
  } else if (is_fan) {
    ha_subscribe_attribute(
      entity_id, std::string("percentage"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad](esphome::StringRef val) {
          int pct = 0;
          if (slider_parse_pct(val, pct)) {
            slider_set_value_safe(slider, pct);
            slider_update_fill(fill, btn_ptr, inv ? 100 - pct : pct, horiz, inv, rad);
          }
        })
    );
  } else if (is_light) {
    ha_subscribe_attribute(
      entity_id, std::string("brightness"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad, sctx](esphome::StringRef val) {
          if (sctx && !sctx->logged_level) {
            sctx->logged_level = true;
            ESP_LOGI("slider", "First brightness for %s: %s",
              sctx->entity_id.c_str(), string_ref_limited(val, HA_SHORT_STATE_MAX_LEN).c_str());
          }
          int pct = 0;
          if (slider_parse_light_brightness_pct(val, pct)) {
            slider_set_value_safe(slider, pct);
            slider_update_fill(fill, btn_ptr, inv ? 100 - pct : pct, horiz, inv, rad);
          } else if (!slider_attribute_missing_ref(val)) {
            ESP_LOGW("slider", "Ignoring invalid brightness for %s: %s",
              sctx ? sctx->entity_id.c_str() : "",
              string_ref_limited(val, HA_SHORT_STATE_MAX_LEN).c_str());
          }
        })
    );
  } else {
    ESP_LOGW("slider", "No brightness attribute subscription for non-light slider entity %s",
      entity_id.c_str());
  }
  ESP_LOGI("slider", "Subscribing slider state for %s", entity_id.c_str());
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on, sctx](esphome::StringRef state) {
        if (sctx && !sctx->logged_state) {
          sctx->logged_state = true;
          ESP_LOGI("slider", "First slider state for %s: %s",
            sctx->entity_id.c_str(), string_ref_limited(state, HA_SHORT_STATE_MAX_LEN).c_str());
        }
        bool unavailable = ha_state_unavailable_ref(state);
        if (sctx) sctx->available = !unavailable;
        bool on = is_entity_on_ref(state);
        if (!on) {
          slider_set_value_safe(slider, 0);
          slider_update_fill(fill, btn_ptr, inv ? 100 : 0, horiz, inv, rad);
        }
        if (has_icon_on)
          slider_set_icon_safe(icon_lbl, on ? icon_on : icon_off);
      })
  );
}

// ── Light temperature card helpers ───────────────────────────────────

// Bulbs store color temperature as integer mireds, so a 5500K command echoes
// back from HA as ~5494K. Rounding the drag preview to 50K keeps the displayed
// value steady while the user fine-tunes the slider.
inline int light_temp_rounded_kelvin(SliderCtx *ctx, int kelvin) {
  if (!ctx) return kelvin;
  int rounded = ((kelvin + 25) / 50) * 50;
  if (rounded < ctx->kelvin_min) rounded = ctx->kelvin_min;
  if (rounded > ctx->kelvin_max) rounded = ctx->kelvin_max;
  return rounded;
}

inline void light_temp_show_drag_kelvin(SliderCtx *ctx, int kelvin) {
  if (!ctx || !ctx->text_lbl) return;
  char buf[16];
  snprintf(buf, sizeof(buf), "%dK", light_temp_rounded_kelvin(ctx, kelvin));
  lv_label_set_text(ctx->text_lbl, buf);
}

inline void light_temp_restore_label(SliderCtx *ctx) {
  if (!ctx || !ctx->text_lbl) return;
  lv_label_set_text(ctx->text_lbl, ctx->cached_label.c_str());
}

// Subscribe to friendly_name and keep the SliderCtx cached_label in sync;
// the bottom label always stays as a configured label or friendly name.
inline void subscribe_friendly_name_for_light_temp(lv_obj_t *text_lbl,
                                                    SliderCtx *ctx,
                                                    const std::string &entity_id) {
  if (entity_id.empty() || !text_lbl) return;
  ha_subscribe_attribute(
    entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [text_lbl, ctx](esphome::StringRef name) {
        if (ctx) ctx->cached_label = string_ref_limited(name, HA_FRIENDLY_NAME_MAX_LEN);
        if (ctx && ctx->light_temp_dragging) return;
        lv_label_set_text_limited(text_lbl, name, HA_FRIENDLY_NAME_MAX_LEN);
      })
  );
}

inline void light_temp_apply_kelvin_state(SliderCtx *ctx, lv_obj_t *btn_ptr,
                                          lv_obj_t *slider, int kelvin,
                                          bool kelvin_color) {
  if (!ctx || !slider || !btn_ptr) return;
  int k = kelvin;
  if (k < ctx->kelvin_min) k = ctx->kelvin_min;
  if (k > ctx->kelvin_max) k = ctx->kelvin_max;
  int range = ctx->kelvin_max - ctx->kelvin_min;
  int pct = range > 0 ? (k - ctx->kelvin_min) * 100 / range : 50;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_slider_set_value(slider, pct, LV_ANIM_OFF);
  if (ctx->fill)
    slider_update_fill(ctx->fill, btn_ptr, pct, false, false, ctx->radius);
  if (kelvin_color && ctx->fill)
    lv_obj_set_style_bg_color(ctx->fill,
      kelvin_to_fill_color(k, ctx->kelvin_min, ctx->kelvin_max), LV_PART_MAIN);
}

// Subscribe to on/off state plus color_temp_kelvin for a light temperature slider.
// When the light is off, the slider renders empty (value 0, no fill).
inline void subscribe_light_temp_state(lv_obj_t *btn_ptr, lv_obj_t *slider,
                                        const std::string &entity_id,
                                        int /*min_k*/, int /*max_k*/, bool kelvin_color) {
  if (!slider || entity_id.empty()) return;
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  // Track on/off so kelvin updates can be ignored once the light is known off
  // while still handling the initial case where HA sends color_temp before state.
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, kelvin_color, sctx](esphome::StringRef state) {
        bool unavailable = ha_state_unavailable_ref(state);
        if (sctx) sctx->available = !unavailable;
        bool on = is_entity_on_ref(state);
        if (sctx) {
          sctx->light_state_known = true;
          sctx->light_on = on;
        }
        if (!on) {
          lv_slider_set_value(slider, 0, LV_ANIM_OFF);
          if (sctx && sctx->fill)
            slider_update_fill(sctx->fill, btn_ptr, 0, false, false, sctx->radius);
        } else if (sctx && sctx->light_temp_has_kelvin) {
          light_temp_apply_kelvin_state(
            sctx, btn_ptr, slider, sctx->light_temp_last_kelvin, kelvin_color);
        }
      })
  );
  ha_subscribe_attribute(
    entity_id, std::string("color_temp_kelvin"),
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, kelvin_color, sctx](esphome::StringRef val) {
        float k_f = 0.0f;
        if (!parse_float_ref(val, k_f)) return;
        int k = (int)(k_f + 0.5f);
        if (!sctx) return;
        sctx->light_temp_last_kelvin = k;
        sctx->light_temp_has_kelvin = true;
        if (!sctx->light_state_known || !sctx->light_on) return;
        // HA can report values outside the configured display range. Clamp in
        // the renderer so slider and label agree.
        light_temp_apply_kelvin_state(sctx, btn_ptr, slider, k, kelvin_color);
      })
  );
}

// Build the visual for a light temperature slider card.
inline void setup_light_temp_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);
  lv_label_set_text(s.icon_lbl, light_temp_icon(p.icon));
  int min_k = 2000, max_k = 6500;
  parse_kelvin_range(p.unit, min_k, max_k);
  bool kcolor = (p.precision == "color");

  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, false);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_label_set_long_mode(s.icon_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_width(s.icon_lbl, lv_pct(100));
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = p.entity;
  ctx->fill = fill;
  ctx->horizontal = false;
  ctx->cover_tilt = false;
  ctx->inverted = false;
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  ctx->light_temp = true;
  ctx->kelvin_min = min_k;
  ctx->kelvin_max = max_k;
  ctx->kelvin_color = kcolor;
  ctx->light_on = false;
  ctx->text_lbl = s.text_lbl;
  ctx->cached_label = p.label;  // may be empty; friendly_name sub fills it later
  lv_obj_set_user_data(slider, (void *)ctx);
  slider_bind_geometry_refresh(s.btn, slider);

  if (kcolor && fill) {
    int mid_k = min_k + (max_k - min_k) / 2;
    lv_obj_set_style_bg_color(fill, kelvin_to_fill_color(mid_k, min_k, max_k), LV_PART_MAIN);
  }

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    slider_update_fill(c->fill, lv_obj_get_parent(sl), val, false, false, c->radius);
    int k = c->kelvin_min + val * (c->kelvin_max - c->kelvin_min) / 100;
    if (c->kelvin_color && c->fill) {
      lv_obj_set_style_bg_color(c->fill, kelvin_to_fill_color(k, c->kelvin_min, c->kelvin_max), LV_PART_MAIN);
    }
    // Treat dragging as the light coming on so following HA attribute updates
    // are not discarded before the on/off state echo arrives.
    c->light_on = true;
    c->light_state_known = true;
    c->light_temp_has_kelvin = true;
    c->light_temp_last_kelvin = k;
    c->light_temp_dragging = true;
    light_temp_show_drag_kelvin(c, k);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c) {
      c->light_temp_dragging = false;
      light_temp_restore_label(c);
    }
    if (c && !c->available) return;
    if (c && !c->entity_id.empty())
      send_light_temp_action(c->entity_id, lv_slider_get_value(sl), c->kelvin_min, c->kelvin_max);
  }, LV_EVENT_RELEASED, nullptr);
}

// ── Media player card helpers ─────────────────────────────────────────

inline const char *media_default_icon(const std::string &mode,
                                      const std::string &icon) {
  if (!icon.empty() && icon != "Auto") return find_icon(icon.c_str());
  if (mode == "control_modal") return find_icon("Play Pause");
  if (mode == "previous") return find_icon("Skip Previous");
  if (mode == "next") return find_icon("Skip Next");
  if (mode == "play_pause") return find_icon("Play Pause");
  if (mode == "volume") return find_icon("Volume High");
  if (mode == "position") return find_icon("Progress Clock");
  if (mode == "now_playing") return find_icon("Music");
  if (mode == "cover_art") return find_icon("Music");
  if (mode == "playlist") return find_icon("Music");
  return find_icon("Play Pause");
}

inline std::string media_default_label(const std::string &mode) {
  if (mode == "previous") return espcontrol_i18n(std::string("Previous"));
  if (mode == "next") return espcontrol_i18n(std::string("Next"));
  if (mode == "volume") return espcontrol_i18n(std::string("Volume"));
  if (mode == "position") return espcontrol_i18n(std::string("Position"));
  if (mode == "play_pause") return espcontrol_i18n(std::string("Play/Pause"));
  if (mode == "control_modal") return espcontrol_i18n(std::string("All Controls"));
  if (mode == "cover_art") return espcontrol_i18n(std::string("Cover Art"));
  if (mode == "playlist") return espcontrol_i18n(std::string("Playlist"));
  return espcontrol_i18n(std::string("Media"));
}

inline std::string media_label(const ParsedCfg &p) {
  return p.label.empty() ? espcontrol_i18n(std::string("Volume")) : p.label;
}

inline std::string media_action_label(const ParsedCfg &p, const std::string &mode) {
  return p.label.empty() ? media_default_label(mode) : p.label;
}

inline bool media_play_pause_show_state(const ParsedCfg &p) {
  return media_card_mode(p.sensor) == "play_pause" && p.precision == "state";
}

inline bool media_position_show_state(const ParsedCfg &p) {
  return media_card_mode(p.sensor) == "position" && p.precision == "state";
}

inline bool media_now_playing_progress_enabled(const ParsedCfg &p) {
  return media_card_mode(p.sensor) == "now_playing" && p.precision == "progress";
}

inline bool media_now_playing_play_pause_enabled(const ParsedCfg &p) {
  return media_card_mode(p.sensor) == "now_playing" && p.precision == "play_pause";
}

inline void media_format_time(float seconds, char *buf, size_t size) {
  if (!buf || size == 0) return;
  if (seconds < 0.0f || !std::isfinite(seconds)) seconds = 0.0f;
  int total = (int)(seconds + 0.5f);
  int h = total / 3600;
  int m = (total / 60) % 60;
  int s = total % 60;
  if (h > 0) snprintf(buf, size, "%d:%02d:%02d", h, m, s);
  else snprintf(buf, size, "%d:%02d", m, s);
}

inline void media_format_percent(int percent, char *buf, size_t size) {
  if (!buf || size == 0) return;
  if (percent < 0) percent = 0;
  if (percent > 100) percent = 100;
  snprintf(buf, size, "%d%%", percent);
}

inline int media_clamp_percent(int value) {
  if (value < 0) return 0;
  if (value > 100) return 100;
  return value;
}

inline int media_volume_max_pct(MediaVolumeCtx *ctx) {
  if (!ctx) return 100;
  if (ctx->max_pct < 1) return 1;
  if (ctx->max_pct > 100) return 100;
  return ctx->max_pct;
}

inline int media_volume_clamp_user_percent(MediaVolumeCtx *ctx, int value) {
  value = media_clamp_percent(value);
  int max_pct = media_volume_max_pct(ctx);
  return value > max_pct ? max_pct : value;
}

inline bool media_volume_pending_active(MediaVolumeCtx *ctx) {
  return ctx && ctx->pending_until_ms != 0 &&
         (int32_t)(ctx->pending_until_ms - esphome::millis()) > 0;
}

inline void media_volume_set_modal_value(MediaVolumeCtx *ctx, int pct);

inline bool media_volume_has_mic_control(MediaVolumeCtx *ctx) {
  return ctx && ctx->mic_muted && ctx->set_mic_muted;
}

inline void media_volume_apply_mic_button_state(MediaVolumeCtx *ctx) {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (!ctx || !ui.mic_btn || !ui.mic_lbl || !media_volume_has_mic_control(ctx)) return;
  bool muted = ctx->mic_muted();
  lv_label_set_text(ui.mic_lbl, muted ? "\U000F036D" : "\U000F036C");
  lv_obj_set_style_text_color(ui.mic_lbl,
    lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
}

inline void media_volume_refresh_active_mic_button() {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  media_volume_apply_mic_button_state(ui.active);
}

inline void media_volume_set_card_value(MediaVolumeCtx *ctx, int pct) {
  if (!ctx || !ctx->pct_lbl) return;
  pct = media_clamp_percent(pct);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", pct);
  lv_label_set_text(ctx->pct_lbl, buf);
  if (ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, "");
}

inline void media_volume_apply_percent(MediaVolumeCtx *ctx, int pct,
                                       bool from_user, bool send_action) {
  if (!ctx || !ctx->available) return;
  pct = media_volume_clamp_user_percent(ctx, pct);
  ctx->current_pct = pct;
  if (from_user) {
    ctx->pending_pct = pct;
    ctx->pending_until_ms = esphome::millis() + 1500;
  }
  media_volume_set_card_value(ctx, pct);
  media_volume_set_modal_value(ctx, pct);
  if (send_action) {
    if (ctx->apply_percent) {
      ctx->apply_percent(pct);
    } else {
      send_media_volume_action(ctx->entity_id, pct);
    }
  }
}

inline void media_volume_hide_modal() {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  control_modal_delete_overlay(ControlModalKind::MEDIA_VOLUME, ui.overlay);
  ui = MediaVolumeModalUi();
}

inline lv_coord_t media_volume_scaled_px(lv_coord_t px, lv_coord_t short_side) {
  return control_modal_scaled_px(px, short_side);
}

inline void media_volume_grid_card_rect(lv_coord_t sw, lv_coord_t sh,
                                        lv_coord_t &x, lv_coord_t &y,
                                        lv_coord_t &w, lv_coord_t &h) {
  MediaHomeGridMetrics &metrics = media_home_grid_metrics();
  lv_obj_t *home = metrics.page;
  int cols = metrics.cols > 0 ? metrics.cols : 3;
  int rows = metrics.rows > 0 ? metrics.rows : 3;
  x = 0;
  y = 0;
  w = sw / cols;
  h = sh / rows;
  if (!home) return;

  lv_obj_update_layout(home);
  lv_coord_t pad_left = lv_obj_get_style_pad_left(home, LV_PART_MAIN);
  lv_coord_t pad_right = lv_obj_get_style_pad_right(home, LV_PART_MAIN);
  lv_coord_t pad_top = lv_obj_get_style_pad_top(home, LV_PART_MAIN);
  lv_coord_t pad_bottom = lv_obj_get_style_pad_bottom(home, LV_PART_MAIN);
  lv_coord_t gap_col = lv_obj_get_style_pad_column(home, LV_PART_MAIN);
  lv_coord_t gap_row = lv_obj_get_style_pad_row(home, LV_PART_MAIN);
  int span_cols = cols < 3 ? cols : 3;
  int span_rows = rows < 3 ? rows : 3;
  if (metrics.first_card) {
    lv_area_t card_area;
    lv_obj_get_coords(metrics.first_card, &card_area);
    x = 5;
    y = card_area.y1;
    w = lv_obj_get_width(metrics.first_card) * span_cols + gap_col * (span_cols - 1);
    h = lv_obj_get_height(metrics.first_card) * span_rows + gap_row * (span_rows - 1);
    return;
  }

  lv_coord_t usable_w = sw - pad_left - pad_right - gap_col * (cols - 1);
  lv_coord_t usable_h = sh - pad_top - pad_bottom - gap_row * (rows - 1);
  lv_coord_t cell_w = usable_w > 0 ? usable_w / cols : w;
  lv_coord_t cell_h = usable_h > 0 ? usable_h / rows : h;
  w = cell_w * span_cols + gap_col * (span_cols - 1);
  h = cell_h * span_rows + gap_row * (span_rows - 1);
  x = 5;
  y = pad_top;
}

inline void media_volume_layout_modal(MediaVolumeCtx *ctx) {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);
  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout, media_volume_card_radius(ctx));
  if (ui.title_lbl) lv_obj_update_layout(ui.title_lbl);
  if (ui.pct_row) lv_obj_update_layout(ui.pct_row);
  lv_coord_t title_h = ui.title_lbl ? lv_obj_get_height(ui.title_lbl) : 0;
  lv_coord_t value_h = ui.pct_row ? lv_obj_get_height(ui.pct_row) : 0;
  lv_coord_t title_center_y = layout.value_center_y -
    (value_h / 2 + layout.title_gap + title_h / 2);

  control_modal_apply_back_button_layout(ui.back_btn, layout);
  control_modal_apply_arc_layout(ui.arc, layout, ctx->width_compensation_percent);
  control_modal_apply_step_buttons_layout(
    ui.minus_btn, ui.plus_btn, media_volume_step_button_layout(layout));
  if (ui.mic_btn) {
    lv_obj_set_size(ui.mic_btn, layout.back_size, layout.back_size);
    lv_obj_set_style_radius(ui.mic_btn, layout.back_size / 2, LV_PART_MAIN);
    lv_coord_t mic_offset =
      control_modal_scaled_px(MEDIA_VOLUME_MIC_BUTTON_OFFSET_REF_PX, layout.short_side);
    lv_obj_align(ui.mic_btn, LV_ALIGN_TOP_RIGHT,
      -layout.inset - mic_offset, layout.back_inset_y + mic_offset);
    if (ui.mic_lbl && MEDIA_VOLUME_MIC_ICON_ZOOM != 256) {
      lv_obj_update_layout(ui.mic_lbl);
      lv_coord_t offset_x = lv_obj_get_width(ui.mic_lbl) *
        (256 - MEDIA_VOLUME_MIC_ICON_ZOOM) / 512;
      lv_coord_t offset_y = lv_obj_get_height(ui.mic_lbl) *
        (256 - MEDIA_VOLUME_MIC_ICON_ZOOM) / 512;
      lv_obj_set_style_transform_zoom(ui.mic_lbl, MEDIA_VOLUME_MIC_ICON_ZOOM, LV_PART_MAIN);
      lv_obj_align(ui.mic_lbl, LV_ALIGN_CENTER, offset_x, offset_y);
    }
  }
  lv_obj_set_style_translate_y(ui.pct_unit_lbl,
    control_modal_scaled_px(MEDIA_VOLUME_UNIT_Y_REF_PX, layout.short_side), LV_PART_MAIN);
  lv_obj_align(ui.title_lbl, LV_ALIGN_CENTER, 0, title_center_y);
  lv_obj_align(ui.pct_row, LV_ALIGN_CENTER, 0, layout.value_center_y);
  lv_obj_move_foreground(ui.back_btn);
  if (ui.mic_btn) lv_obj_move_foreground(ui.mic_btn);
}

inline void media_volume_set_modal_value(MediaVolumeCtx *ctx, int pct) {
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  if (!ctx || ui.active != ctx) return;
  pct = media_clamp_percent(pct);
  if (ui.arc) {
    ui.updating_arc = true;
    lv_arc_set_value(ui.arc, pct > media_volume_max_pct(ctx) ? media_volume_max_pct(ctx) : pct);
    ui.updating_arc = false;
  }
  if (ui.pct_lbl) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", pct);
    lv_label_set_text(ui.pct_lbl, buf);
  }
  if (ui.pct_unit_lbl) lv_label_set_text(ui.pct_unit_lbl, "");
}

inline void media_volume_open_modal(MediaVolumeCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::MEDIA_VOLUME, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, media_volume_hide_modal);
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  lv_obj_t *back_label = lv_obj_get_child(ui.back_btn, 0);
  if (back_label) lv_obj_set_style_text_color(back_label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);

  ui.arc = lv_arc_create(ui.panel);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, 0, media_volume_max_pct(ctx));
  lv_arc_set_value(ui.arc, media_volume_clamp_user_percent(ctx, ctx->current_pct));
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(DARK_TRACK_BACKGROUND), LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(ctx->accent_color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.arc, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_KNOB);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_add_flag(ui.arc, LV_OBJ_FLAG_ADV_HITTEST);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    media_volume_apply_percent(ui.active, lv_arc_get_value(arc), true, true);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  ui.title_lbl = lv_label_create(ui.panel);
  lv_label_set_text(ui.title_lbl, espcontrol_i18n("Volume"));
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.title_lbl, ctx->label_font, LV_PART_MAIN);
  apply_width_compensation(ui.title_lbl, ctx->width_compensation_percent);

  ui.pct_row = lv_obj_create(ui.panel);
  lv_obj_set_size(ui.pct_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  lv_obj_clear_flag(ui.pct_row, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.pct_row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(ui.pct_row, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.pct_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.pct_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.pct_row, 4, LV_PART_MAIN);
  lv_obj_set_layout(ui.pct_row, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.pct_row, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.pct_row, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.pct_row, LV_FLEX_ALIGN_END, LV_PART_MAIN);

  ui.pct_lbl = lv_label_create(ui.pct_row);
  lv_obj_set_style_text_color(ui.pct_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pct_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->number_font) lv_obj_set_style_text_font(ui.pct_lbl, ctx->number_font, LV_PART_MAIN);
  apply_width_compensation(ui.pct_lbl, ctx->width_compensation_percent);

  ui.pct_unit_lbl = lv_label_create(ui.pct_row);
  lv_label_set_text(ui.pct_unit_lbl, "");
  lv_obj_set_style_text_color(ui.pct_unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pct_unit_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->unit_font) lv_obj_set_style_text_font(ui.pct_unit_lbl, ctx->unit_font, LV_PART_MAIN);
  lv_obj_set_style_translate_y(ui.pct_unit_lbl, MEDIA_VOLUME_UNIT_Y_REF_PX, LV_PART_MAIN);
  apply_width_compensation(ui.pct_unit_lbl, ctx->width_compensation_percent);

  ui.minus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Minus"),
    ctx->icon_font, DARK_CONTROL_NEUTRAL, SECONDARY_GREY, ctx->width_compensation_percent);
  ui.plus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Plus"),
    ctx->icon_font, DARK_CONTROL_NEUTRAL, SECONDARY_GREY, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) {
      int current = ui.active->current_pct > media_volume_max_pct(ui.active)
        ? media_volume_max_pct(ui.active)
        : ui.active->current_pct;
      media_volume_apply_percent(ui.active, current - 1, true, true);
    }
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) {
      int current = ui.active->current_pct > media_volume_max_pct(ui.active)
        ? media_volume_max_pct(ui.active)
        : ui.active->current_pct;
      media_volume_apply_percent(ui.active, current + 1, true, true);
    }
  }, LV_EVENT_CLICKED, nullptr);

  if (media_volume_has_mic_control(ctx)) {
    ui.mic_btn = control_modal_create_round_button(ui.panel, 32, "\U000F036C",
      ctx->icon_font, DARK_BORDER, SECONDARY_GREY, ctx->width_compensation_percent);
    if (ui.mic_btn) {
      control_modal_style_chrome_button(ui.mic_btn, shell.layout, true);
      ui.mic_lbl = lv_obj_get_child(ui.mic_btn, 0);
      lv_obj_add_event_cb(ui.mic_btn, [](lv_event_t *) {
        MediaVolumeModalUi &ui = media_volume_modal_ui();
        if (!media_volume_has_mic_control(ui.active)) return;
        bool muted = ui.active->mic_muted();
        ui.active->set_mic_muted(!muted);
        media_volume_apply_mic_button_state(ui.active);
      }, LV_EVENT_CLICKED, nullptr);
      media_volume_apply_mic_button_state(ctx);
    }
  }

  media_volume_layout_modal(ctx);
  media_volume_set_modal_value(ctx, ctx->current_pct);
  lv_obj_move_foreground(ui.overlay);
}
