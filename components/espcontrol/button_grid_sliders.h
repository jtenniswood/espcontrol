#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Slider widgets ───────────────────────────────────────────────────

// Context attached to each LVGL slider via user_data
struct SliderCtx {
  std::string entity_id;
  lv_obj_t *fill;
  bool horizontal;
  bool cover_tilt;
  bool inverted;
  lv_coord_t radius;
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
constexpr lv_coord_t MEDIA_VOLUME_REFERENCE_SIDE_PX = 480;
constexpr lv_coord_t MEDIA_VOLUME_ARC_STROKE_REF_PX = 17;
constexpr lv_coord_t MEDIA_VOLUME_BACK_BUTTON_REF_PX = 46;
constexpr lv_coord_t MEDIA_VOLUME_BUTTON_REF_PX = 80;
constexpr lv_coord_t MEDIA_VOLUME_INSET_REF_PX = 18;
constexpr lv_coord_t MEDIA_VOLUME_CONTROLS_GAP_REF_PX = 24;
constexpr lv_coord_t MEDIA_VOLUME_CONTROLS_DOWN_REF_PX = 22;
constexpr lv_coord_t MEDIA_VOLUME_TITLE_GAP_REF_PX = 10;
constexpr lv_coord_t MEDIA_VOLUME_UNIT_Y_REF_PX = -22;

struct MediaVolumeCtx {
  std::string entity_id;
  std::string label;
  int current_pct = 0;
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
  std::function<void()> pause_home_idle;
  std::function<void()> resume_home_idle;
  bool available = true;
};

struct MediaHomeGridMetrics {
  lv_obj_t *page = nullptr;
  lv_obj_t *first_card = nullptr;
  int cols = 3;
  int rows = 3;
};

inline MediaHomeGridMetrics &media_home_grid_metrics() {
  static MediaHomeGridMetrics metrics;
  return metrics;
}

inline void set_media_home_grid_metrics(lv_obj_t *page, int cols, int rows,
                                        lv_obj_t *first_card = nullptr) {
  MediaHomeGridMetrics &metrics = media_home_grid_metrics();
  metrics.page = page;
  metrics.first_card = first_card;
  metrics.cols = cols > 0 ? cols : 3;
  metrics.rows = rows > 0 ? rows : 3;
}

struct ControlModalLayout {
  lv_coord_t sw = 480;
  lv_coord_t sh = 480;
  lv_coord_t short_side = 480;
  lv_coord_t panel_x = 4;
  lv_coord_t panel_y = 0;
  lv_coord_t panel_w = 472;
  lv_coord_t panel_h = 480;
  lv_coord_t inset = MEDIA_VOLUME_INSET_REF_PX;
  lv_coord_t back_size = MEDIA_VOLUME_BACK_BUTTON_REF_PX;
  lv_coord_t btn_size = MEDIA_VOLUME_BUTTON_REF_PX;
  lv_coord_t arc_stroke = MEDIA_VOLUME_ARC_STROKE_REF_PX;
  lv_coord_t controls_gap = MEDIA_VOLUME_CONTROLS_GAP_REF_PX;
  lv_coord_t arc_size = 320;
  lv_coord_t arc_center_x = 0;
  lv_coord_t arc_center_y = 0;
  lv_coord_t value_center_y = 0;
  lv_coord_t title_gap = MEDIA_VOLUME_TITLE_GAP_REF_PX;
  lv_coord_t controls_center_y = 0;
};

inline lv_coord_t control_modal_scaled_px(lv_coord_t px, lv_coord_t short_side) {
  return px * short_side / MEDIA_VOLUME_REFERENCE_SIDE_PX;
}

inline lv_coord_t control_modal_card_radius(lv_obj_t *btn) {
  if (btn) return lv_obj_get_style_radius(btn, LV_PART_MAIN);
  MediaHomeGridMetrics &metrics = media_home_grid_metrics();
  return metrics.first_card ? lv_obj_get_style_radius(metrics.first_card, LV_PART_MAIN) : 18;
}

inline ControlModalLayout control_modal_calc_layout(int width_compensation_percent) {
  ControlModalLayout layout;
  lv_disp_t *disp = lv_disp_get_default();
  layout.sw = disp ? lv_disp_get_hor_res(disp) : 480;
  layout.sh = disp ? lv_disp_get_ver_res(disp) : 480;
  layout.short_side = layout.sw < layout.sh ? layout.sw : layout.sh;

  layout.panel_x = 4;
  layout.panel_y = 0;
  layout.panel_w = layout.sw - layout.panel_x - 4;
  layout.panel_h = layout.sh;
  MediaHomeGridMetrics &metrics = media_home_grid_metrics();
  if (metrics.page) {
    lv_obj_update_layout(metrics.page);
    layout.panel_x = lv_obj_get_style_pad_left(metrics.page, LV_PART_MAIN);
    layout.panel_y = lv_obj_get_style_pad_top(metrics.page, LV_PART_MAIN);
    layout.panel_w = layout.sw - layout.panel_x - lv_obj_get_style_pad_right(metrics.page, LV_PART_MAIN);
    layout.panel_h = layout.sh - layout.panel_y - lv_obj_get_style_pad_bottom(metrics.page, LV_PART_MAIN);
  }

  layout.back_size = control_modal_scaled_px(MEDIA_VOLUME_BACK_BUTTON_REF_PX, layout.short_side);
  layout.btn_size = control_modal_scaled_px(MEDIA_VOLUME_BUTTON_REF_PX, layout.short_side);
  layout.inset = control_modal_scaled_px(MEDIA_VOLUME_INSET_REF_PX, layout.short_side);
  if (layout.inset < 8) layout.inset = 8;
  layout.arc_stroke = control_modal_scaled_px(MEDIA_VOLUME_ARC_STROKE_REF_PX, layout.short_side);
  layout.controls_gap = control_modal_scaled_px(MEDIA_VOLUME_CONTROLS_GAP_REF_PX, layout.short_side);
  layout.title_gap = control_modal_scaled_px(MEDIA_VOLUME_TITLE_GAP_REF_PX, layout.short_side);

  layout.arc_size = layout.panel_w < layout.panel_h ? layout.panel_w : layout.panel_h;
  layout.arc_size -= layout.inset * 2;
  lv_coord_t reserved_bottom = layout.btn_size / 3 + layout.inset;
  lv_coord_t available_h = layout.panel_h - layout.inset * 2;
  if (available_h > reserved_bottom) {
    lv_coord_t fit_h = available_h - reserved_bottom + layout.arc_stroke;
    if (layout.arc_size > fit_h) layout.arc_size = fit_h;
  }
  if (layout.arc_size < 74) layout.arc_size = 74;

  int width_percent = normalize_width_compensation_percent(width_compensation_percent);
  lv_coord_t visible_arc_w = compensated_width(layout.arc_size, width_percent);
  if (visible_arc_w > layout.panel_w - layout.inset * 2) {
    layout.arc_size = (layout.panel_w - layout.inset * 2) * 100 / width_percent;
    visible_arc_w = compensated_width(layout.arc_size, width_percent);
  }

  layout.arc_center_x = (layout.arc_size - visible_arc_w) / 2;
  layout.arc_center_y = 0;
  layout.value_center_y = layout.arc_stroke / 2;
  layout.controls_center_y = layout.arc_size / 2 - layout.btn_size / 2 - layout.inset +
    control_modal_scaled_px(MEDIA_VOLUME_CONTROLS_DOWN_REF_PX, layout.short_side);
  return layout;
}

inline void control_modal_style_overlay(lv_obj_t *overlay) {
  if (!overlay) return;
  lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
}

inline void control_modal_style_panel(lv_obj_t *panel, uint32_t bg_color, lv_coord_t radius) {
  if (!panel) return;
  lv_obj_set_style_bg_color(panel, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(panel, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(panel, 0, LV_PART_MAIN);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
}

inline void control_modal_apply_panel_layout(lv_obj_t *overlay, lv_obj_t *panel,
                                             const ControlModalLayout &layout,
                                             lv_coord_t radius) {
  if (overlay) lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
  if (!panel) return;
  lv_obj_set_size(panel, layout.panel_w, layout.panel_h);
  lv_obj_set_pos(panel, layout.panel_x, layout.panel_y);
  lv_obj_set_style_radius(panel, radius, LV_PART_MAIN);
}

inline void control_modal_apply_back_button_layout(lv_obj_t *btn,
                                                   const ControlModalLayout &layout) {
  if (!btn) return;
  lv_obj_set_size(btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(btn, layout.back_size / 2, LV_PART_MAIN);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, layout.inset, layout.inset);
}

inline void control_modal_apply_arc_layout(lv_obj_t *arc,
                                           const ControlModalLayout &layout,
                                           int width_compensation_percent,
                                           bool with_knob = true) {
  if (!arc) return;
  lv_obj_set_size(arc, layout.arc_size, layout.arc_size);
  apply_width_compensation(arc, width_compensation_percent);
  lv_obj_align(arc, LV_ALIGN_CENTER, layout.arc_center_x, layout.arc_center_y);
  lv_obj_set_style_arc_width(arc, layout.arc_stroke, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, layout.arc_stroke, LV_PART_INDICATOR);
  if (with_knob) lv_obj_set_style_pad_all(arc, layout.short_side < 520 ? 4 : 6, LV_PART_KNOB);
}

inline void control_modal_apply_step_buttons_layout(lv_obj_t *minus_btn,
                                                    lv_obj_t *plus_btn,
                                                    const ControlModalLayout &layout) {
  if (minus_btn) {
    lv_obj_set_size(minus_btn, layout.btn_size, layout.btn_size);
    lv_obj_set_style_radius(minus_btn, layout.btn_size / 2, LV_PART_MAIN);
    lv_obj_align(minus_btn, LV_ALIGN_CENTER,
      -(layout.btn_size + layout.controls_gap) / 2, layout.controls_center_y);
  }
  if (plus_btn) {
    lv_obj_set_size(plus_btn, layout.btn_size, layout.btn_size);
    lv_obj_set_style_radius(plus_btn, layout.btn_size / 2, LV_PART_MAIN);
    lv_obj_align(plus_btn, LV_ALIGN_CENTER,
      (layout.btn_size + layout.controls_gap) / 2, layout.controls_center_y);
  }
}

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
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
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

inline void slider_bind_geometry_refresh(lv_obj_t *btn, lv_obj_t *slider) {
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    lv_obj_t *sl = (lv_obj_t *)lv_event_get_user_data(e);
    slider_refresh_geometry(sl);
  }, LV_EVENT_SIZE_CHANGED, slider);
  slider_refresh_geometry(slider);
}

// Create an invisible LVGL slider with a colored fill overlay inside a button
inline lv_obj_t *setup_slider_widget(lv_obj_t *btn, uint32_t on_color, bool horizontal) {
  lv_obj_set_style_pad_all(btn, 0,
    static_cast<lv_style_selector_t>(LV_PART_MAIN));
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *fill = lv_obj_create(btn);
  lv_obj_set_size(fill, 0, 0);
  lv_obj_set_style_bg_color(fill, lv_color_hex(on_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(fill, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(fill, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(fill, 0, LV_PART_MAIN);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *slider = lv_slider_create(btn);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  lv_obj_update_layout(btn);
  slider_fit_to_button(slider, btn, horizontal);

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

inline void setup_cover_toggle_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Cover" : p.label.c_str());
}

inline void setup_cover_command_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Cover" : p.label.c_str());
  apply_push_button_transition(s.btn);
}

// Full slider button setup: visual + event handlers + HA action on release
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);
  if (p.type == "cover")
    lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));

  bool horizontal = false;
  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, horizontal);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
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
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    int fill_val = c->inverted ? 100 - val : val;
    slider_update_ctx_fill(c, lv_obj_get_parent(sl), fill_val);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty()) {
      if (!c->available) return;
      int val = lv_slider_get_value(sl);
      send_slider_action(c->entity_id, val, c->cover_tilt);
    }
  }, LV_EVENT_RELEASED, nullptr);
}

// Subscribe to HA state for a slider entity (light brightness, fan percentage, or cover position/tilt)
inline void subscribe_slider_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                  lv_obj_t *slider,
                                  bool has_icon_on,
                                  const char *icon_off, const char *icon_on,
                                  const std::string &entity_id,
                                  bool cover_tilt = false) {
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *fill = sctx ? sctx->fill : nullptr;
  bool horiz = sctx ? sctx->horizontal : false;
  bool inv = sctx ? sctx->inverted : false;
  lv_coord_t rad = sctx ? sctx->radius : 0;
  bool is_cover = is_cover_entity(entity_id);
  bool is_fan = is_fan_entity(entity_id);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(esphome::StringRef)>(
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on, sctx](esphome::StringRef state) {
        bool unavailable = ha_state_unavailable_ref(state);
        if (sctx) sctx->available = !unavailable;
        apply_control_availability(btn_ptr, slider, !unavailable);
        bool on = is_entity_on_ref(state);
        if (!on) {
          lv_slider_set_value(slider, 0, LV_ANIM_OFF);
          if (fill) slider_update_fill(fill, btn_ptr, inv ? 100 : 0, horiz, inv, rad);
        }
        if (has_icon_on)
          lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
      })
  );
  if (is_cover) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string(cover_tilt ? "current_tilt_position" : "current_position"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](esphome::StringRef val) {
          float pos = 0.0f;
          if (parse_float_ref(val, pos)) {
            int pct = (int)(pos + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
            if (has_icon_on) {
              lv_label_set_text(icon_lbl, pct > 0 ? icon_on : icon_off);
            }
          }
        })
    );
  } else if (is_fan) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("percentage"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad](esphome::StringRef val) {
          float pct_f = 0.0f;
          if (parse_float_ref(val, pct_f)) {
            int pct = (int)(pct_f + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
          }
        })
    );
  } else {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("brightness"),
      std::function<void(esphome::StringRef)>(
        [slider, btn_ptr, fill, horiz, inv, rad](esphome::StringRef val) {
          float bri = 0.0f;
          if (parse_float_ref(val, bri)) {
            int pct = (int)((bri * 100.0f + 127.0f) / 255.0f);
            if (pct < 1) pct = 1;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
          }
        })
    );
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
  esphome::api::global_api_server->subscribe_home_assistant_state(
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
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
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
  esphome::api::global_api_server->subscribe_home_assistant_state(
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
  if (mode == "previous") return "Previous";
  if (mode == "next") return "Next";
  if (mode == "volume") return "Volume";
  if (mode == "position") return "Position";
  if (mode == "play_pause") return "Play/Pause";
  return "Media";
}

inline std::string media_label(const ParsedCfg &p) {
  return p.label.empty() ? std::string("Volume") : p.label;
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
  pct = media_clamp_percent(pct);
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
  if (ui.overlay) lv_obj_del(ui.overlay);
  ui.overlay = nullptr;
  ui.panel = nullptr;
  ui.back_btn = nullptr;
  ui.arc = nullptr;
  ui.title_lbl = nullptr;
  ui.pct_row = nullptr;
  ui.pct_lbl = nullptr;
  ui.pct_unit_lbl = nullptr;
  ui.minus_btn = nullptr;
  ui.plus_btn = nullptr;
  ui.active = nullptr;
  ui.updating_arc = false;
}

inline lv_obj_t *control_modal_create_round_button(lv_obj_t *parent, lv_coord_t size,
                                                  const char *text,
                                                  const lv_font_t *font,
                                                  uint32_t border_color,
                                                  uint32_t bg_color,
                                                  int width_compensation_percent = 100) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, size, size);
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(border_color), LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
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
  control_modal_apply_step_buttons_layout(ui.minus_btn, ui.plus_btn, layout);
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
    lv_arc_set_value(ui.arc, pct);
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
  media_volume_hide_modal();
  MediaVolumeModalUi &ui = media_volume_modal_ui();
  ui.active = ctx;

  lv_obj_t *parent = lv_layer_top();
  ui.overlay = lv_obj_create(parent);
  control_modal_style_overlay(ui.overlay);

  ui.panel = lv_obj_create(ui.overlay);
  control_modal_style_panel(ui.panel, ctx->tertiary_color, media_volume_card_radius(ctx));

  ui.back_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0141",
    ctx->icon_font, 0x454545, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_set_style_bg_opa(ui.back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.back_btn, 0, LV_PART_MAIN);
  lv_obj_t *back_label = lv_obj_get_child(ui.back_btn, 0);
  if (back_label) lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_add_event_cb(ui.back_btn, [](lv_event_t *) {
    media_volume_hide_modal();
  }, LV_EVENT_CLICKED, nullptr);

  ui.arc = lv_arc_create(ui.panel);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, 0, 100);
  lv_arc_set_value(ui.arc, ctx->current_pct);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(ctx->accent_color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(ui.arc, lv_color_hex(0xFFFFFF), LV_PART_KNOB);
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
  lv_label_set_text(ui.title_lbl, "Volume");
  lv_obj_set_style_text_color(ui.title_lbl, lv_color_hex(0xA0A0A0), LV_PART_MAIN);
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
  lv_obj_set_style_text_color(ui.pct_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pct_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->number_font) lv_obj_set_style_text_font(ui.pct_lbl, ctx->number_font, LV_PART_MAIN);
  apply_width_compensation(ui.pct_lbl, ctx->width_compensation_percent);

  ui.pct_unit_lbl = lv_label_create(ui.pct_row);
  lv_label_set_text(ui.pct_unit_lbl, "");
  lv_obj_set_style_text_color(ui.pct_unit_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.pct_unit_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->unit_font) lv_obj_set_style_text_font(ui.pct_unit_lbl, ctx->unit_font, LV_PART_MAIN);
  lv_obj_set_style_translate_y(ui.pct_unit_lbl, MEDIA_VOLUME_UNIT_Y_REF_PX, LV_PART_MAIN);
  apply_width_compensation(ui.pct_unit_lbl, ctx->width_compensation_percent);

  ui.minus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Minus"),
    ctx->icon_font, 0xBDBDBD, ctx->tertiary_color, ctx->width_compensation_percent);
  ui.plus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Plus"),
    ctx->icon_font, 0xBDBDBD, ctx->tertiary_color, ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) media_volume_apply_percent(ui.active, ui.active->current_pct - 1, true, true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *) {
    MediaVolumeModalUi &ui = media_volume_modal_ui();
    if (ui.active) media_volume_apply_percent(ui.active, ui.active->current_pct + 1, true, true);
  }, LV_EVENT_CLICKED, nullptr);

  media_volume_layout_modal(ctx);
  media_volume_set_modal_value(ctx, ctx->current_pct);
  lv_obj_move_foreground(ui.overlay);
}
