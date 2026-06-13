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
  lv_obj_t *media_track_bg = nullptr;
  lv_obj_t *media_value_lbl = nullptr;
  lv_obj_t *media_status_lbl = nullptr;
  lv_timer_t *media_timer = nullptr;
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
  lv_obj_t *btn = nullptr;
  bool play_pause_background = false;
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
constexpr lv_coord_t MEDIA_VOLUME_JC4880P443_BUTTON_REF_PX = 96;

struct MediaVolumeCtx {
  std::string entity_id;
  std::string label;
  int current_pct = 0;
  int max_pct = 100;
  int pending_pct = -1;
  uint32_t pending_until_ms = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = DEFAULT_OFF_COLOR;
  uint32_t tertiary_color = DEFAULT_TERTIARY_COLOR;
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
  std::function<void()> suspend_display_takeover;
  std::function<void()> resume_display_takeover;
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
  MediaVolumeCtx *active = nullptr;
  bool updating_arc = false;
};

inline MediaVolumeModalUi &media_volume_modal_ui() {
  static MediaVolumeModalUi ui;
  return ui;
}

inline lv_coord_t media_volume_card_radius(MediaVolumeCtx *ctx) {
  return control_modal_card_radius(ctx ? ctx->btn : nullptr);
}

inline ControlModalLayout media_volume_step_button_layout(const ControlModalLayout &layout) {
  ControlModalLayout controls_layout = layout;
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    controls_layout.btn_size = control_modal_scaled_px(
      MEDIA_VOLUME_JC4880P443_BUTTON_REF_PX, layout.short_side);
    controls_layout.controls_center_y = layout.arc_size / 2 -
      controls_layout.btn_size / 2 - layout.inset +
      control_modal_controls_down_px(layout);
  }
  return controls_layout;
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
  }
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);
}

// Resize the colored fill overlay to reflect the current slider percentage
inline void slider_update_fill(lv_obj_t *fill, lv_obj_t *btn, int pct, bool horizontal, bool inverted, lv_coord_t r) {
  if (!fill || !btn) return;
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (bw <= 0 || bh <= 0) return;
  lv_obj_set_style_radius(fill, r, LV_PART_MAIN);
  if (horizontal) {
    lv_coord_t w = (lv_coord_t)((int32_t)bw * pct / 100);
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
    slider_update_fill(c->fill, btn, pct, c->horizontal, c->inverted, c->radius);
  }
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
};

struct CoverControlCtx {
  std::string entity_id;
  std::string label;
  std::string friendly_name;
  int current_position = 0;
  int current_tilt = 0;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  const char *icon_closed_glyph = nullptr;
  const char *icon_open_glyph = nullptr;
  const lv_font_t *icon_font = nullptr;
  int width_compensation_percent = 100;
  bool available = true;
  bool dragging_position = false;
  bool dragging_tilt = false;
  bool updating_position = false;
  bool updating_tilt = false;
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
  lv_obj_t *controls_box = nullptr;
  lv_obj_t *up_btn = nullptr;
  lv_obj_t *stop_btn = nullptr;
  lv_obj_t *down_btn = nullptr;
  lv_obj_t *position_slider = nullptr;
  lv_obj_t *tilt_slider = nullptr;
  CoverControlCtx *active = nullptr;
  CoverControlTab tab = CoverControlTab::CONTROLS;
};

inline CoverControlModalUi &cover_control_modal_ui() {
  static CoverControlModalUi ui;
  return ui;
}

inline std::string cover_control_title(CoverControlCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Cover"));
  if (!ctx->label.empty()) return ctx->label;
  if (!ctx->friendly_name.empty()) return ctx->friendly_name;
  return espcontrol_i18n(std::string("Cover"));
}

inline void cover_control_apply_card_visual(CoverControlCtx *ctx,
                                            const std::string &state_text = "") {
  if (!ctx || !ctx->btn) return;
  apply_control_availability(ctx->btn, ctx->btn, ctx->available);
  bool active = !state_text.empty() ? cover_toggle_state_is_active(state_text) : ctx->current_position > 0;
  set_card_checked_state(ctx->btn, ctx->available && active);
  if (ctx->icon_lbl) {
    bool open_icon = !state_text.empty() ? garage_state_uses_open_icon(state_text) : ctx->current_position > 0;
    lv_label_set_text(ctx->icon_lbl, open_icon ? ctx->icon_open_glyph : ctx->icon_closed_glyph);
  }
  if (ctx->label_lbl) {
    std::string title = cover_control_title(ctx);
    lv_label_set_text(ctx->label_lbl, title.c_str());
  }
}

inline void cover_control_style_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
  if (!btn) return;
  lv_obj_set_style_bg_color(
    btn, lv_color_hex(active ? accent_color : DARK_BACKGROUND_TERTIARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, active ? LV_OPA_COVER : LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
}

inline void cover_control_apply_tab_visibility() {
  CoverControlModalUi &ui = cover_control_modal_ui();
  CoverControlCtx *ctx = ui.active;
  if (!ctx) return;
  if (!ctx->supports_tilt && ui.tab == CoverControlTab::TILT) {
    ui.tab = CoverControlTab::CONTROLS;
  }
  bool show_controls = ui.tab == CoverControlTab::CONTROLS;
  bool show_position = ui.tab == CoverControlTab::POSITION;
  bool show_tilt = ctx->supports_tilt && ui.tab == CoverControlTab::TILT;
  if (ui.tilt_tab) {
    if (ctx->supports_tilt) lv_obj_clear_flag(ui.tilt_tab, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tilt_tab, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.controls_box) {
    if (show_controls) lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.controls_box, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.position_slider) {
    if (show_position) lv_obj_clear_flag(ui.position_slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.position_slider, LV_OBJ_FLAG_HIDDEN);
  }
  if (ui.tilt_slider) {
    if (show_tilt) lv_obj_clear_flag(ui.tilt_slider, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(ui.tilt_slider, LV_OBJ_FLAG_HIDDEN);
  }
  cover_control_style_tab(ui.controls_tab, show_controls, ctx->accent_color);
  cover_control_style_tab(ui.position_tab, show_position, ctx->accent_color);
  cover_control_style_tab(ui.tilt_tab, show_tilt, ctx->accent_color);
}

inline void cover_control_layout_modal(CoverControlCtx *ctx);

inline lv_obj_t *cover_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                 const lv_font_t *font,
                                                 CoverControlTab tab,
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
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_transform_zoom(label, 210, LV_PART_MAIN);
    lv_obj_center(label);
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

inline void cover_control_layout_slider(lv_obj_t *slider, lv_coord_t width,
                                        lv_coord_t height, lv_coord_t center_y) {
  if (!slider) return;
  lv_obj_set_size(slider, width, height);
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, center_y);
  lv_coord_t slider_radius = width / 5;
  if (slider_radius < 18) slider_radius = 18;
  if (slider_radius > 34) slider_radius = 34;
  lv_obj_set_style_radius(slider, slider_radius, LV_PART_MAIN);
  lv_obj_set_style_radius(slider, 0, LV_PART_INDICATOR);
  lv_obj_set_style_width(slider, 0, LV_PART_KNOB);
  lv_obj_set_style_height(slider, 0, LV_PART_KNOB);
}

inline void cover_control_layout_modal(CoverControlCtx *ctx) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ctx || !ui.panel) return;
  if (!ctx->supports_tilt && ui.tab == CoverControlTab::TILT) {
    ui.tab = CoverControlTab::CONTROLS;
  }
  ControlModalLayout layout = control_modal_calc_layout(ctx->width_compensation_percent);

  lv_coord_t tab_size = layout.back_size * 7 / 10;
  if (tab_size < 48) tab_size = 48;
  if (tab_size > 68) tab_size = 68;
  lv_coord_t selected_tab_size = tab_size + tab_size / 8;
  lv_coord_t tab_frame_pad = tab_size / 5;
  lv_coord_t tab_frame_h = tab_size + tab_frame_pad * 2;
  lv_coord_t tab_gap = tab_size / 4;
  int visible_tab_count = ctx->supports_tilt ? 3 : 2;
  lv_coord_t tabs_total_w = tab_size * visible_tab_count + tab_gap * (visible_tab_count - 1);
  lv_coord_t tab_frame_w = tabs_total_w + tab_frame_pad * 2;
  lv_coord_t max_tab_frame_w = layout.panel_w - layout.inset * 3;
  if (tab_frame_w > max_tab_frame_w) tab_frame_w = max_tab_frame_w;
  if (ui.tab_row) {
    lv_obj_set_size(ui.tab_row, tab_frame_w, tab_frame_h);
    lv_obj_set_style_radius(ui.tab_row, tab_frame_h / 2, LV_PART_MAIN);
    lv_obj_align(ui.tab_row, LV_ALIGN_TOP_MID, 0, layout.inset + 2);
  }
  lv_obj_t *tabs[3] = {ui.controls_tab, ui.position_tab, ctx->supports_tilt ? ui.tilt_tab : nullptr};
  lv_coord_t first_tab_x = (tab_frame_w - tabs_total_w) / 2;
  int visible_index = 0;
  for (int i = 0; i < 3; i++) {
    if (!tabs[i]) continue;
    bool active = (i == static_cast<int>(ui.tab));
    lv_coord_t tab_btn_size = active ? selected_tab_size : tab_size;
    lv_obj_set_size(tabs[i], tab_btn_size, tab_btn_size);
    lv_obj_set_style_radius(tabs[i], tab_btn_size / 2, LV_PART_MAIN);
    lv_coord_t tab_x = first_tab_x + visible_index * (tab_size + tab_gap);
    lv_obj_align(tabs[i], LV_ALIGN_LEFT_MID, tab_x - (tab_btn_size - tab_size) / 2, 0);
    lv_obj_t *label = lv_obj_get_child(tabs[i], 0);
    if (label) lv_obj_align(label, LV_ALIGN_CENTER, tab_btn_size / 16, tab_btn_size / 16);
    visible_index++;
  }

  lv_coord_t content_center_y = tab_frame_h / 2 + 12;
  lv_coord_t content_h = layout.panel_h - layout.inset * 3 - tab_frame_h - 16;
  if (content_h < 160) content_h = layout.panel_h / 2;
  lv_coord_t content_w = tab_frame_w;
  if (content_w >= content_h) content_w = content_h * 3 / 4;
  cover_control_layout_slider(ui.position_slider, content_w, content_h, content_center_y);
  cover_control_layout_slider(ui.tilt_slider, content_w, content_h, content_center_y);

  if (ui.controls_box) {
    lv_coord_t box_w = layout.panel_w - layout.inset * 3;
    lv_coord_t box_h = content_h;
    lv_obj_set_size(ui.controls_box, box_w, box_h);
    lv_obj_align(ui.controls_box, LV_ALIGN_CENTER, 0, content_center_y);
    lv_coord_t btn_size = layout.btn_size;
    if (btn_size < 72) btn_size = 72;
    if (btn_size > 118) btn_size = 118;
    lv_coord_t gap = btn_size / 4;
    lv_coord_t total_h = btn_size * 3 + gap * 2;
    lv_coord_t start_y = (box_h - total_h) / 2;
    if (start_y < 0) start_y = 0;
    lv_obj_t *buttons[3] = {ui.up_btn, ui.stop_btn, ui.down_btn};
    for (int i = 0; i < 3; i++) {
      if (!buttons[i]) continue;
      lv_obj_set_size(buttons[i], btn_size, btn_size);
      lv_obj_set_style_radius(buttons[i], btn_size / 2, LV_PART_MAIN);
      lv_obj_align(buttons[i], LV_ALIGN_TOP_MID, 0, start_y + i * (btn_size + gap));
    }
  }
}

inline void cover_control_hide_modal() {
  CoverControlModalUi &ui = cover_control_modal_ui();
  lv_obj_t *overlay = ui.overlay;
  ui = CoverControlModalUi();
  control_modal_delete_overlay(ControlModalKind::COVER_CONTROL, overlay);
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
  cover_control_set_slider_value(
    ui.position_slider, ctx->updating_position, ctx->dragging_position, pct);
}

inline void cover_control_set_tilt_value(CoverControlCtx *ctx, int pct) {
  CoverControlModalUi &ui = cover_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  cover_control_set_slider_value(ui.tilt_slider, ctx->updating_tilt, ctx->dragging_tilt, pct);
}

inline bool cover_control_parse_supported_features(esphome::StringRef val, int &features) {
  if (slider_attribute_missing_ref(val)) return false;
  std::string value = string_ref_limited(val, 16);
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return false;
  features = static_cast<int>(parsed);
  return true;
}

inline void cover_control_set_tilt_supported(CoverControlCtx *ctx, bool supported) {
  if (!ctx || ctx->supports_tilt == supported) return;
  ctx->supports_tilt = supported;
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
  lv_obj_set_style_bg_color(slider, lv_color_hex(DARK_BACKGROUND_SECONDARY), LV_PART_MAIN);
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

inline void cover_control_open_modal(CoverControlCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::COVER_CONTROL, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, "\U000F0141", false, cover_control_hide_modal);
  CoverControlModalUi &ui = cover_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = CoverControlTab::CONTROLS;
  if (!ui.panel) return;

  ui.tab_row = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.tab_row, lv_color_hex(DARK_BACKGROUND_SECONDARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.tab_row, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.tab_row, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.tab_row, LV_OBJ_FLAG_SCROLLABLE);
  ui.controls_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Gesture Tap"), ctx->icon_font,
    CoverControlTab::CONTROLS, ctx->width_compensation_percent);
  ui.position_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Blinds Horizontal"), ctx->icon_font,
    CoverControlTab::POSITION, ctx->width_compensation_percent);
  ui.tilt_tab = cover_control_create_tab_button(
    ui.tab_row, find_icon("Blinds Open"), ctx->icon_font,
    CoverControlTab::TILT, ctx->width_compensation_percent);

  ui.controls_box = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.controls_box, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.controls_box, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.controls_box, LV_OBJ_FLAG_SCROLLABLE);
  ui.up_btn = control_modal_create_round_button(
    ui.controls_box, 96, find_icon("Arrow Up"), ctx->icon_font,
    ctx->accent_color, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.stop_btn = control_modal_create_round_button(
    ui.controls_box, 96, find_icon("Stop"), ctx->icon_font,
    ctx->accent_color, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.down_btn = control_modal_create_round_button(
    ui.controls_box, 96, find_icon("Arrow Down"), ctx->icon_font,
    ctx->accent_color, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  if (ui.up_btn) {
    lv_obj_add_event_cb(ui.up_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available) send_cover_command_action(ui.active->entity_id, "open");
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (ui.stop_btn) {
    lv_obj_add_event_cb(ui.stop_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available) send_cover_command_action(ui.active->entity_id, "stop");
    }, LV_EVENT_CLICKED, nullptr);
  }
  if (ui.down_btn) {
    lv_obj_add_event_cb(ui.down_btn, [](lv_event_t *e) {
      (void) e;
      CoverControlModalUi &ui = cover_control_modal_ui();
      if (ui.active && ui.active->available) send_cover_command_action(ui.active->entity_id, "close");
    }, LV_EVENT_CLICKED, nullptr);
  }

  ui.position_slider = lv_slider_create(ui.panel);
  cover_control_style_slider(ui.position_slider, ctx->accent_color);
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
    ui.active->current_position = lv_slider_get_value(slider);
    cover_control_apply_card_visual(ui.active);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (!ui.active) return;
    ui.active->dragging_position = false;
    if (!ui.active->available) return;
    lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int pct = lv_slider_get_value(slider);
    ui.active->current_position = pct;
    send_slider_action(ui.active->entity_id, pct, false);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.position_slider, [](lv_event_t *e) {
    CoverControlModalUi &ui = cover_control_modal_ui();
    if (ui.active) ui.active->dragging_position = false;
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.tilt_slider = lv_slider_create(ui.panel);
  cover_control_style_slider(ui.tilt_slider, ctx->accent_color);
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
  cover_control_apply_tab_visibility();
  lv_obj_move_foreground(ui.overlay);
}

inline void setup_cover_modal_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? espcontrol_i18n("Cover") : p.label.c_str());
  apply_push_button_transition(s.btn);
}

inline CoverControlCtx *create_cover_control_context(
    const BtnSlot &s,
    const ParsedCfg &p,
    uint32_t accent_color,
    const lv_font_t *icon_font,
    int width_compensation_percent) {
  CoverControlCtx *ctx = new CoverControlCtx();
  ctx->entity_id = p.entity;
  ctx->label = p.label;
  ctx->accent_color = accent_color;
  ctx->btn = s.btn;
  ctx->icon_lbl = s.icon_lbl;
  ctx->label_lbl = s.text_lbl;
  ctx->icon_closed_glyph = slider_icon_off(p.type, p.entity, p.icon);
  ctx->icon_open_glyph = slider_icon_on(p.type, p.entity, p.icon, p.icon_on);
  ctx->icon_font = icon_font;
  ctx->width_compensation_percent = width_compensation_percent;
  lv_obj_set_user_data(s.btn, ctx);
  return ctx;
}

inline void subscribe_cover_control_state(CoverControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty()) return;
  register_ha_control_availability(ctx->btn, ctx->btn);
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef state) {
        std::string state_text = string_ref_limited(state, HA_SHORT_STATE_MAX_LEN);
        ctx->available = !ha_state_unavailable_ref(state);
        cover_control_apply_card_visual(ctx, state_text);
        CoverControlModalUi &ui = cover_control_modal_ui();
        if (ui.active == ctx) {
          apply_control_availability(ui.panel, ui.panel, ctx->available, false);
        }
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("current_position"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        int pct = 0;
        if (!slider_parse_pct(val, pct)) return;
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
        if (!cover_control_parse_supported_features(val, features)) {
          cover_control_set_tilt_supported(ctx, false);
          return;
        }
        constexpr int COVER_SUPPORT_SET_TILT_POSITION = 128;
        cover_control_set_tilt_supported(ctx, (features & COVER_SUPPORT_SET_TILT_POSITION) != 0);
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
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
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

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (!sl) return;
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    int fill_val = c->inverted ? 100 - val : val;
    slider_update_ctx_fill(c, lv_obj_get_parent(sl), fill_val);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

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
  register_ha_control_availability(btn_ptr, slider);
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *fill = sctx ? sctx->fill : nullptr;
  bool horiz = sctx ? sctx->horizontal : false;
  bool inv = sctx ? sctx->inverted : false;
  lv_coord_t rad = sctx ? sctx->radius : 0;
  bool is_cover = is_cover_entity(entity_id);
  bool is_fan = is_fan_entity(entity_id);
  bool is_light = slider_entity_is_light(entity_id);
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
        apply_control_availability(btn_ptr, slider, !unavailable);
        bool on = is_entity_on_ref(state);
        if (!on) {
          slider_set_value_safe(slider, 0);
          slider_update_fill(fill, btn_ptr, inv ? 100 : 0, horiz, inv, rad);
        }
        if (has_icon_on)
          slider_set_icon_safe(icon_lbl, on ? icon_on : icon_off);
      })
  );
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
  register_ha_control_availability(btn_ptr, slider);
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  // Track on/off so kelvin updates can be ignored once the light is known off
  // while still handling the initial case where HA sends color_temp before state.
  ha_subscribe_state(
    entity_id,
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, kelvin_color, sctx](esphome::StringRef state) {
        bool unavailable = ha_state_unavailable_ref(state);
        if (sctx) sctx->available = !unavailable;
        apply_control_availability(btn_ptr, slider, !unavailable);
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
  if (mode == "previous") return find_icon("Skip Previous");
  if (mode == "next") return find_icon("Skip Next");
  if (mode == "play_pause") return find_icon("Play Pause");
  if (mode == "volume") return find_icon("Volume High");
  if (mode == "position") return find_icon("Progress Clock");
  if (mode == "now_playing") return find_icon("Music");
  return find_icon("Play Pause");
}

inline std::string media_default_label(const std::string &mode) {
  if (mode == "previous") return espcontrol_i18n(std::string("Previous"));
  if (mode == "next") return espcontrol_i18n(std::string("Next"));
  if (mode == "volume") return espcontrol_i18n(std::string("Volume"));
  if (mode == "position") return espcontrol_i18n(std::string("Position"));
  if (mode == "play_pause") return espcontrol_i18n(std::string("Play/Pause"));
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
  if (send_action) send_media_volume_action(ctx->entity_id, pct);
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
  lv_obj_set_style_translate_y(ui.pct_unit_lbl,
    control_modal_scaled_px(MEDIA_VOLUME_UNIT_Y_REF_PX, layout.short_side), LV_PART_MAIN);
  lv_obj_align(ui.title_lbl, LV_ALIGN_CENTER, 0, title_center_y);
  lv_obj_align(ui.pct_row, LV_ALIGN_CENTER, 0, layout.value_center_y);
  lv_obj_move_foreground(ui.back_btn);
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
    ctx->icon_font, "\U000F0141", false, media_volume_hide_modal);
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
    ctx->icon_font, DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
  ui.plus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Plus"),
    ctx->icon_font, DARK_CONTROL_NEUTRAL, DARK_BACKGROUND_TERTIARY, ctx->width_compensation_percent);
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

  media_volume_layout_modal(ctx);
  media_volume_set_modal_value(ctx, ctx->current_pct);
  lv_obj_move_foreground(ui.overlay);
}
