#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// Shared control modal helpers.

constexpr lv_coord_t CONTROL_MODAL_REFERENCE_SIDE_PX = DISPLAY_MODAL_REFERENCE_SIDE_PX;
constexpr lv_coord_t CONTROL_MODAL_ARC_STROKE_REF_PX = DISPLAY_MODAL_ARC_STROKE_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_BACK_BUTTON_REF_PX = DISPLAY_MODAL_BACK_BUTTON_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_BUTTON_REF_PX = DISPLAY_MODAL_BUTTON_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_INSET_REF_PX = DISPLAY_MODAL_INSET_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_CONTROLS_GAP_REF_PX = DISPLAY_MODAL_CONTROLS_GAP_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_CONTROLS_DOWN_REF_PX = DISPLAY_MODAL_CONTROLS_DOWN_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_TITLE_GAP_REF_PX = DISPLAY_MODAL_TITLE_GAP_REF_PX;
constexpr lv_coord_t CONTROL_MODAL_P4_86_TAB_REF_PX = 50;
constexpr lv_coord_t CONTROL_MODAL_P4_86_TAB_CONTENT_GAP_REF_PX = 30;
constexpr lv_coord_t CONTROL_MODAL_JC4880P443_TAB_CONTENT_GAP_REF_PX = 12;

enum class ControlModalKind {
  NONE,
  MEDIA_VOLUME,
  CLIMATE,
  SWITCH_CONFIRMATION,
  OPTION_SELECT,
  FAN_PRESET,
  FAN_CONTROL,
  NETWORK_STATUS,
  ALARM_PIN,
  ALARM_CONTROL,
  IMAGE_CARD,
  TODO_LIST,
  COVER_CONTROL,
  LIGHT_CONTROL,
  MEDIA_CONTROL,
  LAWN_MOWER_CONTROL,
};

using ControlModalCloseCallback = void (*)();

struct ControlModalActive {
  ControlModalKind kind = ControlModalKind::NONE;
  lv_obj_t *overlay = nullptr;
  ControlModalCloseCallback close_callback = nullptr;
  uint32_t close_guard_until_ms = 0;
  bool closing = false;
};

inline ControlModalActive &control_modal_active() {
  static ControlModalActive active;
  return active;
}

inline void control_modal_reset_active() {
  control_modal_active() = ControlModalActive();
}

inline void control_modal_clear_active(ControlModalKind kind) {
  ControlModalActive &active = control_modal_active();
  if (active.kind == kind) control_modal_reset_active();
}

inline void control_modal_delete_overlay(ControlModalKind kind, lv_obj_t *&overlay) {
  lv_obj_t *deleted = overlay;
  overlay = nullptr;
  if (deleted) lv_obj_del(deleted);
  control_modal_clear_active(kind);
}

inline void control_modal_set_active(ControlModalKind kind, lv_obj_t *overlay,
                                     ControlModalCloseCallback close_callback) {
  ControlModalActive &active = control_modal_active();
  active.kind = kind;
  active.overlay = overlay;
  active.close_callback = close_callback;
  active.close_guard_until_ms = 0;
  active.closing = false;
}

inline bool control_modal_close_guard_active(const ControlModalActive &active) {
  return active.close_guard_until_ms != 0 &&
         (int32_t)(lv_tick_get() - active.close_guard_until_ms) < 0;
}

inline void control_modal_block_close_for(uint32_t delay_ms) {
  ControlModalActive &active = control_modal_active();
  if (active.kind == ControlModalKind::NONE || delay_ms == 0) return;
  active.close_guard_until_ms = lv_tick_get() + delay_ms;
}

inline void control_modal_close_active_internal(bool honor_close_guard) {
  ControlModalActive &active = control_modal_active();
  if (active.kind == ControlModalKind::NONE || active.closing) return;
  if (honor_close_guard && control_modal_close_guard_active(active)) return;

  ControlModalKind closing_kind = active.kind;
  void (*close_callback)() = active.close_callback;
  active.closing = true;
  if (close_callback) close_callback();
  if (control_modal_active().kind == closing_kind) control_modal_reset_active();
}

inline void control_modal_close_active() {
  control_modal_close_active_internal(true);
}

inline void control_modal_force_close_active() {
  control_modal_close_active_internal(false);
}

struct ControlModalGridMetrics {
  lv_obj_t *page = nullptr;
  lv_obj_t *first_card = nullptr;
  int cols = 3;
  int rows = 3;
};

inline ControlModalGridMetrics &control_modal_grid_metrics() {
  static ControlModalGridMetrics metrics;
  return metrics;
}

using MediaHomeGridMetrics = ControlModalGridMetrics;

inline MediaHomeGridMetrics &media_home_grid_metrics() {
  return control_modal_grid_metrics();
}

inline void set_control_modal_grid_metrics(lv_obj_t *page, int cols, int rows,
                                           lv_obj_t *first_card = nullptr) {
  ControlModalGridMetrics &metrics = control_modal_grid_metrics();
  metrics.page = page;
  metrics.first_card = first_card;
  metrics.cols = cols > 0 ? cols : 3;
  metrics.rows = rows > 0 ? rows : 3;
}

inline void set_media_home_grid_metrics(lv_obj_t *page, int cols, int rows,
                                        lv_obj_t *first_card = nullptr) {
  set_control_modal_grid_metrics(page, cols, rows, first_card);
}

struct ControlModalLayout {
  lv_coord_t sw = 480;
  lv_coord_t sh = 480;
  lv_coord_t short_side = 480;
  lv_coord_t panel_x = 4;
  lv_coord_t panel_y = 0;
  lv_coord_t panel_w = 472;
  lv_coord_t panel_h = 480;
  lv_coord_t inset = CONTROL_MODAL_INSET_REF_PX;
  lv_coord_t back_inset_x = CONTROL_MODAL_INSET_REF_PX;
  lv_coord_t back_inset_y = CONTROL_MODAL_INSET_REF_PX;
  lv_coord_t back_size = CONTROL_MODAL_BACK_BUTTON_REF_PX;
  lv_coord_t btn_size = CONTROL_MODAL_BUTTON_REF_PX;
  lv_coord_t arc_stroke = CONTROL_MODAL_ARC_STROKE_REF_PX;
  lv_coord_t controls_gap = CONTROL_MODAL_CONTROLS_GAP_REF_PX;
  lv_coord_t arc_size = 320;
  lv_coord_t arc_center_x = 0;
  lv_coord_t arc_center_y = 0;
  lv_coord_t value_center_y = 0;
  lv_coord_t title_gap = CONTROL_MODAL_TITLE_GAP_REF_PX;
  lv_coord_t controls_center_y = 0;
};

struct ControlModalTabLayout {
  int tab_count = 1;
  bool show_tab_bar = false;
  lv_coord_t tab_size = 48;
  lv_coord_t selected_tab_size = 54;
  lv_coord_t tab_frame_pad = 9;
  lv_coord_t tab_gap = 12;
  lv_coord_t tabs_total_w = 48;
  lv_coord_t tab_frame_w = 66;
  lv_coord_t tab_frame_h = 66;
  lv_coord_t tab_safe_left = 0;
  lv_coord_t centered_left = 0;
  lv_coord_t row_left = 0;
  lv_coord_t content_gap = 16;
};

struct ControlModalShell {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *close_btn = nullptr;
  ControlModalLayout layout;
  lv_coord_t content_w = 0;
};

struct ControlModalNestedShell {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
};

struct ControlModalNestedActive {
  lv_obj_t *overlay = nullptr;
  ControlModalCloseCallback close_callback = nullptr;
  bool closing = false;
};

struct ControlModalToastShell {
  lv_obj_t *box = nullptr;
};

inline ControlModalNestedActive &control_modal_nested_active() {
  static ControlModalNestedActive active;
  return active;
}

inline void control_modal_reset_nested_menu() {
  control_modal_nested_active() = ControlModalNestedActive();
}

inline void control_modal_clear_nested_menu(lv_obj_t *overlay) {
  ControlModalNestedActive &active = control_modal_nested_active();
  if (active.overlay == overlay) control_modal_reset_nested_menu();
}

inline void control_modal_delete_nested_overlay(lv_obj_t *&overlay) {
  lv_obj_t *deleted = overlay;
  overlay = nullptr;
  if (deleted) lv_obj_del(deleted);
  control_modal_clear_nested_menu(deleted);
}

inline void control_modal_close_nested_menu() {
  ControlModalNestedActive &active = control_modal_nested_active();
  if (!active.overlay || active.closing) return;

  lv_obj_t *closing_overlay = active.overlay;
  ControlModalCloseCallback close_callback = active.close_callback;
  active.closing = true;
  if (close_callback) close_callback();
  else if (closing_overlay) lv_obj_del(closing_overlay);
  if (control_modal_nested_active().overlay == closing_overlay) control_modal_reset_nested_menu();
}

inline lv_coord_t control_modal_scaled_px(lv_coord_t px, lv_coord_t short_side) {
  return display_modal_scaled_px(px, short_side);
}

inline bool control_modal_is_jc4880p443_size(const ControlModalLayout &layout) {
  return display_modal_is_jc4880p443_size(layout.sw, layout.sh);
}

inline bool control_modal_uses_compact_portrait_tuning(const ControlModalLayout &layout) {
  return control_modal_is_jc4880p443_size(layout);
}

inline bool control_modal_uses_square_tuning(const ControlModalLayout &layout) {
  return display_modal_is_square_size(layout.sw, layout.sh);
}

inline bool control_modal_uses_4848_tuning(const ControlModalLayout &layout) {
  return display_modal_is_4848_size(layout.sw, layout.sh);
}

inline bool control_modal_uses_4848_control_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_4848_tuning(layout) ||
         control_modal_is_jc4880p443_size(layout);
}

inline bool control_modal_uses_p4_86_tuning(const ControlModalLayout &layout) {
  return display_modal_is_p4_86_size(layout.sw, layout.sh);
}

inline bool control_modal_uses_large_landscape_tuning(const ControlModalLayout &layout) {
  return display_modal_is_large_landscape_size(layout.sw, layout.sh);
}

inline bool control_modal_uses_jc1060p470_tuning(const ControlModalLayout &layout) {
  return display_modal_is_jc1060p470_size(layout.sw, layout.sh);
}

inline lv_coord_t control_modal_screen_width(lv_coord_t fallback = 480) {
  lv_disp_t *disp = lv_disp_get_default();
  return disp ? lv_disp_get_hor_res(disp) : fallback;
}

inline bool control_modal_current_is_jc4880p443_size() {
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t width = disp ? lv_disp_get_hor_res(disp) : 0;
  lv_coord_t height = disp ? lv_disp_get_ver_res(disp) : 0;
  return display_modal_is_jc4880p443_size(width, height);
}

inline bool control_modal_current_is_4848_size() {
  lv_disp_t *disp = lv_disp_get_default();
  lv_coord_t width = disp ? lv_disp_get_hor_res(disp) : 0;
  lv_coord_t height = disp ? lv_disp_get_ver_res(disp) : 0;
  return display_modal_is_4848_size(width, height);
}

inline lv_coord_t control_modal_controls_down_px(const ControlModalLayout &layout) {
  return control_modal_scaled_px(CONTROL_MODAL_CONTROLS_DOWN_REF_PX, layout.short_side);
}

inline lv_coord_t control_modal_control_tab_min_size(const ControlModalLayout &layout) {
  if (control_modal_uses_large_landscape_tuning(layout)) return 64;
  if (control_modal_uses_compact_portrait_tuning(layout)) return 72;
  return 48;
}

inline lv_coord_t control_modal_control_tab_size(const ControlModalLayout &layout) {
  if (control_modal_uses_compact_portrait_tuning(layout)) {
    lv_coord_t size = control_modal_scaled_px(72, layout.short_side);
    lv_coord_t min_size = control_modal_control_tab_min_size(layout);
    lv_coord_t max_size = 76;
    if (size < min_size) size = min_size;
    if (size > max_size) size = max_size;
    return size;
  }
  lv_coord_t size = layout.back_size * (control_modal_uses_large_landscape_tuning(layout) ? 4 : 7) /
                    (control_modal_uses_large_landscape_tuning(layout) ? 5 : 10);
  lv_coord_t min_size = control_modal_control_tab_min_size(layout);
  lv_coord_t max_size = control_modal_uses_large_landscape_tuning(layout) ? 88 : 68;
  if (size < min_size) size = min_size;
  if (size > max_size) size = max_size;
  return size;
}

inline lv_coord_t control_modal_card_tab_size(const ControlModalLayout &layout) {
  if (control_modal_uses_jc1060p470_tuning(layout))
    return control_modal_scaled_px(54, layout.short_side);
  return control_modal_control_tab_size(layout);
}

inline lv_coord_t control_modal_prominent_card_tab_size(const ControlModalLayout &layout) {
  if (control_modal_uses_compact_portrait_tuning(layout))
    return control_modal_scaled_px(58, layout.short_side);
  return control_modal_card_tab_size(layout);
}

inline lv_coord_t control_modal_shared_tab_size(const ControlModalLayout &layout) {
  if (control_modal_uses_p4_86_tuning(layout))
    return control_modal_scaled_px(CONTROL_MODAL_P4_86_TAB_REF_PX, layout.short_side);
  return control_modal_prominent_card_tab_size(layout);
}

inline lv_coord_t control_modal_control_tab_gap(const ControlModalLayout &layout,
                                                lv_coord_t tab_size) {
  lv_coord_t gap = control_modal_uses_large_landscape_tuning(layout)
    ? tab_size * 2 / 5
    : tab_size / 4;
  lv_coord_t min_gap = control_modal_uses_large_landscape_tuning(layout) ? 24 : 12;
  if (gap < min_gap) gap = min_gap;
  return gap;
}

inline lv_coord_t control_modal_control_tab_content_gap(const ControlModalLayout &layout) {
  if (!control_modal_uses_large_landscape_tuning(layout)) return 16;
  lv_coord_t gap = control_modal_scaled_px(22, layout.short_side);
  if (gap < 28) gap = 28;
  return gap;
}

inline lv_coord_t control_modal_card_tab_content_gap(const ControlModalLayout &layout) {
  if (control_modal_uses_jc1060p470_tuning(layout))
    return control_modal_scaled_px(28, layout.short_side);
  return control_modal_control_tab_content_gap(layout);
}

inline lv_coord_t control_modal_prominent_card_tab_content_gap(const ControlModalLayout &layout) {
  if (control_modal_uses_compact_portrait_tuning(layout))
    return control_modal_scaled_px(30, layout.short_side);
  return control_modal_card_tab_content_gap(layout);
}

inline lv_coord_t control_modal_shared_tab_content_gap(const ControlModalLayout &layout) {
  if (control_modal_uses_p4_86_tuning(layout))
    return control_modal_scaled_px(CONTROL_MODAL_P4_86_TAB_CONTENT_GAP_REF_PX, layout.short_side);
  if (control_modal_uses_compact_portrait_tuning(layout))
    return control_modal_scaled_px(CONTROL_MODAL_JC4880P443_TAB_CONTENT_GAP_REF_PX, layout.short_side);
  return control_modal_prominent_card_tab_content_gap(layout);
}

inline ControlModalTabLayout control_modal_calc_tab_layout(
    const ControlModalLayout &layout, int tab_count, bool show_tab_bar,
    bool avoid_back_button = true) {
  ControlModalTabLayout tabs_layout;
  tabs_layout.tab_count = tab_count < 1 ? 1 : tab_count;
  tabs_layout.show_tab_bar = show_tab_bar && tabs_layout.tab_count > 1;
  tabs_layout.tab_size = control_modal_shared_tab_size(layout);
  tabs_layout.selected_tab_size = tabs_layout.tab_size + tabs_layout.tab_size / 8;
  tabs_layout.tab_frame_pad = tabs_layout.tab_size / 5;
  tabs_layout.tab_gap = control_modal_control_tab_gap(layout, tabs_layout.tab_size);
  tabs_layout.tabs_total_w =
    tabs_layout.tab_size * tabs_layout.tab_count + tabs_layout.tab_gap * (tabs_layout.tab_count - 1);
  tabs_layout.tab_frame_w = tabs_layout.tabs_total_w + tabs_layout.tab_frame_pad * 2;
  tabs_layout.tab_frame_h = tabs_layout.tab_size + tabs_layout.tab_frame_pad * 2;
  tabs_layout.tab_safe_left = layout.back_inset_x + layout.back_size + layout.inset / 2;
  tabs_layout.centered_left = (layout.panel_w - tabs_layout.tab_frame_w) / 2;
  tabs_layout.row_left = tabs_layout.centered_left;
  tabs_layout.content_gap = control_modal_shared_tab_content_gap(layout);
  lv_coord_t min_tab_size = control_modal_control_tab_min_size(layout);
  while (tabs_layout.show_tab_bar &&
         avoid_back_button &&
         !control_modal_uses_compact_portrait_tuning(layout) &&
         !control_modal_uses_p4_86_tuning(layout) &&
         tabs_layout.row_left < tabs_layout.tab_safe_left &&
         tabs_layout.tab_size > min_tab_size) {
    tabs_layout.tab_size--;
    tabs_layout.selected_tab_size = tabs_layout.tab_size + tabs_layout.tab_size / 8;
    tabs_layout.tab_frame_pad = tabs_layout.tab_size / 5;
    tabs_layout.tab_gap = control_modal_control_tab_gap(layout, tabs_layout.tab_size);
    tabs_layout.tabs_total_w =
      tabs_layout.tab_size * tabs_layout.tab_count + tabs_layout.tab_gap * (tabs_layout.tab_count - 1);
    tabs_layout.tab_frame_w = tabs_layout.tabs_total_w + tabs_layout.tab_frame_pad * 2;
    tabs_layout.tab_frame_h = tabs_layout.tab_size + tabs_layout.tab_frame_pad * 2;
    tabs_layout.centered_left = (layout.panel_w - tabs_layout.tab_frame_w) / 2;
    tabs_layout.row_left = tabs_layout.centered_left;
  }
  if (avoid_back_button && tabs_layout.row_left < tabs_layout.tab_safe_left)
    tabs_layout.row_left = tabs_layout.tab_safe_left;
  if (!tabs_layout.show_tab_bar) tabs_layout.tab_frame_h = 0;
  return tabs_layout;
}

inline void control_modal_apply_tab_row(lv_obj_t *tab_row,
                                        const ControlModalLayout &layout,
                                        const ControlModalTabLayout &tabs_layout) {
  if (!tab_row) return;
  if (tabs_layout.show_tab_bar) {
    lv_obj_clear_flag(tab_row, LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_size(tab_row, tabs_layout.tab_frame_w, tabs_layout.tab_frame_h);
    lv_obj_set_style_radius(tab_row, tabs_layout.tab_frame_h / 2, LV_PART_MAIN);
    lv_obj_align(tab_row, LV_ALIGN_TOP_LEFT, tabs_layout.row_left, layout.inset + 2);
  } else {
    lv_obj_add_flag(tab_row, LV_OBJ_FLAG_HIDDEN);
  }
}

inline void control_modal_center_tab_icon(lv_obj_t *label) {
  if (!label) return;
  lv_obj_update_layout(label);
  lv_obj_set_style_transform_pivot_x(label, lv_obj_get_width(label) / 2, LV_PART_MAIN);
  lv_obj_set_style_transform_pivot_y(label, lv_obj_get_height(label) / 2, LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

inline uint16_t control_modal_tab_icon_zoom(const ControlModalLayout &layout) {
  return (control_modal_uses_compact_portrait_tuning(layout) ||
          control_modal_uses_p4_86_tuning(layout)) ? 220 : 180;
}

inline void control_modal_layout_tab_button(lv_obj_t *tab_btn,
                                            const ControlModalLayout &layout,
                                            const ControlModalTabLayout &tabs_layout,
                                            int index, bool active,
                                            int width_compensation_percent = 100) {
  if (!tab_btn || !tabs_layout.show_tab_bar) return;
  lv_coord_t tab_btn_size = active ? tabs_layout.selected_tab_size : tabs_layout.tab_size;
  lv_obj_set_size(tab_btn, tab_btn_size, tab_btn_size);
  apply_width_compensation(tab_btn, width_compensation_percent);
  lv_obj_set_style_radius(tab_btn, tab_btn_size / 2, LV_PART_MAIN);
  lv_coord_t first_tab_x = (tabs_layout.tab_frame_w - tabs_layout.tabs_total_w) / 2;
  lv_coord_t tab_x = first_tab_x + index * (tabs_layout.tab_size + tabs_layout.tab_gap);
  lv_obj_align(tab_btn, LV_ALIGN_LEFT_MID, tab_x - (tab_btn_size - tabs_layout.tab_size) / 2, 0);
  lv_obj_t *label = lv_obj_get_child(tab_btn, 0);
  if (label) lv_obj_set_style_transform_zoom(label, control_modal_tab_icon_zoom(layout), LV_PART_MAIN);
  control_modal_center_tab_icon(label);
}

inline lv_coord_t control_modal_card_radius(lv_obj_t *btn) {
  if (btn) return lv_obj_get_style_radius(btn, LV_PART_MAIN);
  ControlModalGridMetrics &metrics = control_modal_grid_metrics();
  return metrics.first_card ? lv_obj_get_style_radius(metrics.first_card, LV_PART_MAIN) : 18;
}

inline lv_coord_t control_modal_home_card_width(lv_obj_t *btn,
                                                const ControlModalLayout &layout) {
  lv_coord_t width = 0;
  if (btn) {
    lv_obj_update_layout(btn);
    width = lv_obj_get_width(btn);
  }

  ControlModalGridMetrics &metrics = control_modal_grid_metrics();
  if (width <= 0 && metrics.first_card) {
    lv_obj_update_layout(metrics.first_card);
    width = lv_obj_get_width(metrics.first_card);
  }
  if (width <= 0 && metrics.page) {
    lv_obj_update_layout(metrics.page);
    lv_coord_t gap = lv_obj_get_style_pad_column(metrics.page, LV_PART_MAIN);
    width = (layout.panel_w - gap * (metrics.cols - 1)) / metrics.cols;
  }
  if (width <= 0) width = layout.panel_w - layout.inset * 3;

  lv_coord_t max_width = layout.panel_w - layout.inset * 3;
  if (max_width > 0 && width > max_width) width = max_width;
  if (width < 1) width = 1;
  return width;
}

inline ControlModalLayout control_modal_calc_layout(int width_compensation_percent,
                                                    bool allow_compact_portrait_tuning = true) {
  ControlModalLayout layout;
  lv_disp_t *disp = lv_disp_get_default();
  layout.sw = disp ? lv_disp_get_hor_res(disp) : 480;
  layout.sh = disp ? lv_disp_get_ver_res(disp) : 480;
  layout.short_side = layout.sw < layout.sh ? layout.sw : layout.sh;

  layout.panel_x = 4;
  layout.panel_y = 0;
  layout.panel_w = layout.sw - layout.panel_x - 4;
  layout.panel_h = layout.sh;
  ControlModalGridMetrics &metrics = control_modal_grid_metrics();
  if (metrics.page) {
    lv_obj_update_layout(metrics.page);
    layout.panel_x = lv_obj_get_style_pad_left(metrics.page, LV_PART_MAIN);
    layout.panel_y = lv_obj_get_style_pad_top(metrics.page, LV_PART_MAIN);
    layout.panel_w = layout.sw - layout.panel_x - lv_obj_get_style_pad_right(metrics.page, LV_PART_MAIN);
    layout.panel_h = layout.sh - layout.panel_y - lv_obj_get_style_pad_bottom(metrics.page, LV_PART_MAIN);
  }

  layout.back_size = control_modal_scaled_px(CONTROL_MODAL_BACK_BUTTON_REF_PX, layout.short_side);
  layout.btn_size = control_modal_scaled_px(CONTROL_MODAL_BUTTON_REF_PX, layout.short_side);
  layout.inset = control_modal_scaled_px(CONTROL_MODAL_INSET_REF_PX, layout.short_side);
  if (layout.inset < 8) layout.inset = 8;
  layout.back_inset_x = layout.inset;
  layout.back_inset_y = layout.inset;
  if (allow_compact_portrait_tuning && control_modal_uses_compact_portrait_tuning(layout)) {
    lv_coord_t back_offset = control_modal_scaled_px(12, layout.short_side);
    layout.back_inset_x += back_offset;
    layout.back_inset_y += back_offset;
  }
  layout.arc_stroke = control_modal_scaled_px(CONTROL_MODAL_ARC_STROKE_REF_PX, layout.short_side);
  layout.controls_gap = control_modal_scaled_px(CONTROL_MODAL_CONTROLS_GAP_REF_PX, layout.short_side);
  layout.title_gap = control_modal_scaled_px(CONTROL_MODAL_TITLE_GAP_REF_PX, layout.short_side);

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
    control_modal_controls_down_px(layout);
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

inline void control_modal_style_panel(lv_obj_t *panel, lv_coord_t radius) {
  if (!panel) return;
  lv_obj_set_style_bg_color(panel, lv_color_hex(TERTIARY_GREY), LV_PART_MAIN);
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
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, layout.back_inset_x, layout.back_inset_y);
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

inline void control_modal_apply_pressed_fill(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  apply_push_button_transition(btn);
}

inline void control_modal_apply_pressed_fill_color(lv_obj_t *btn,
                                                   uint32_t pressed_color) {
  if (!btn) return;
  control_modal_apply_pressed_fill(btn);
  lv_obj_set_style_bg_color(btn, lv_color_hex(pressed_color),
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
}

inline lv_obj_t *control_modal_icon_label(lv_obj_t *btn) {
  return btn && lv_obj_get_child_cnt(btn) > 0 ? lv_obj_get_child(btn, 0) : nullptr;
}

inline lv_obj_t *control_modal_create_flat_icon_button(
    lv_obj_t *parent,
    const char *icon,
    const lv_font_t *font,
    uint32_t bg_color,
    lv_opa_t bg_opa,
    int width_compensation_percent = 100,
    uint16_t icon_zoom = 256) {
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, bg_opa, LV_PART_MAIN);
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
    if (icon_zoom != 256) lv_obj_set_style_transform_zoom(label, icon_zoom, LV_PART_MAIN);
    lv_obj_center(label);
  }
  return btn;
}

inline lv_obj_t *control_modal_create_round_button(lv_obj_t *parent, lv_coord_t size,
                                                  const char *text,
                                                  const lv_font_t *font,
                                                  uint32_t border_color,
                                                  uint32_t bg_color,
                                                  int width_compensation_percent = 100) {
  (void) border_color;
  lv_obj_t *btn = lv_btn_create(parent);
  if (!btn) return nullptr;
  lv_obj_set_size(btn, size, size);
  apply_width_compensation(btn, width_compensation_percent);
  lv_obj_set_style_radius(btn, size / 2, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
  lv_obj_t *label = lv_label_create(btn);
  if (!label) {
    lv_obj_del(btn);
    return nullptr;
  }
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
  lv_obj_center(label);
  return btn;
}

inline void control_modal_style_chrome_button(lv_obj_t *btn,
                                              const ControlModalLayout &layout,
                                              bool top_right) {
  if (!btn) return;
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_size(btn, layout.back_size, layout.back_size);
  lv_obj_set_style_radius(btn, layout.back_size / 2, LV_PART_MAIN);
  if (top_right) lv_obj_align(btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  else control_modal_apply_back_button_layout(btn, layout);
}

inline void control_modal_style_translucent_chrome_button(lv_obj_t *btn) {
  if (!btn) return;
  lv_obj_set_style_bg_color(btn, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);
}

inline ControlModalShell control_modal_open_shell(ControlModalKind kind,
                                                  lv_obj_t *source_btn,
                                                  int width_compensation_percent,
                                                  const lv_font_t *icon_font,
                                                  const char *button_text,
                                                  bool button_top_right,
                                                  ControlModalCloseCallback close_callback) {
  control_modal_close_active();

  ControlModalShell shell;
  shell.layout = control_modal_calc_layout(width_compensation_percent);
  lv_coord_t radius = control_modal_card_radius(source_btn);
  shell.content_w = shell.layout.panel_w - shell.layout.inset * 2;
  if (shell.content_w < 120) shell.content_w = shell.layout.panel_w;

  shell.overlay = lv_obj_create(lv_layer_top());
  if (!shell.overlay) {
    ESP_LOGW("control_modal", "Unable to create modal overlay");
    return shell;
  }
  control_modal_style_overlay(shell.overlay);

  shell.panel = lv_obj_create(shell.overlay);
  if (!shell.panel) {
    ESP_LOGW("control_modal", "Unable to create modal panel");
    lv_obj_del(shell.overlay);
    shell.overlay = nullptr;
    return shell;
  }
  control_modal_style_panel(shell.panel, radius);
  control_modal_apply_panel_layout(shell.overlay, shell.panel, shell.layout, radius);

  if (button_text) {
    shell.close_btn = control_modal_create_round_button(
      shell.panel, 32, button_text, icon_font,
      DARK_BORDER, SECONDARY_GREY, width_compensation_percent);
    if (!shell.close_btn) {
      ESP_LOGW("control_modal", "Unable to create modal close button");
      lv_obj_del(shell.overlay);
      shell.overlay = nullptr;
      shell.panel = nullptr;
      return shell;
    }
    control_modal_style_chrome_button(shell.close_btn, shell.layout, button_top_right);
    lv_obj_add_event_cb(shell.close_btn, [](lv_event_t *) {
      control_modal_force_close_active();
    }, LV_EVENT_CLICKED, nullptr);
  }

  control_modal_set_active(kind, shell.overlay, close_callback);
  return shell;
}

inline void control_modal_style_nested_overlay(lv_obj_t *overlay) {
  if (!overlay) return;
  lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(overlay, lv_color_hex(DARK_OVERLAY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(overlay, 0, LV_PART_MAIN);
  lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
}

inline void control_modal_style_nested_panel(lv_obj_t *panel, lv_coord_t radius) {
  if (!panel) return;
  lv_obj_set_height(panel, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(panel, lv_color_hex(TERTIARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(panel, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(panel, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(panel, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(panel, 14, LV_PART_MAIN);
  lv_obj_set_style_pad_row(panel, 10, LV_PART_MAIN);
  lv_obj_set_layout(panel, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(panel, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_align(panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
}

inline ControlModalNestedShell control_modal_open_nested_menu(lv_coord_t width,
                                                              lv_coord_t radius,
                                                              ControlModalCloseCallback close_callback) {
  control_modal_close_nested_menu();

  ControlModalNestedShell shell;
  shell.overlay = lv_obj_create(lv_layer_top());
  control_modal_style_nested_overlay(shell.overlay);
  if (close_callback) {
    lv_obj_add_event_cb(shell.overlay, [](lv_event_t *) {
      control_modal_close_nested_menu();
    }, LV_EVENT_CLICKED, nullptr);
  }

  shell.panel = lv_obj_create(shell.overlay);
  lv_obj_set_width(shell.panel, width);
  control_modal_style_nested_panel(shell.panel, radius);
  ControlModalNestedActive &active = control_modal_nested_active();
  active.overlay = shell.overlay;
  active.close_callback = close_callback;
  active.closing = false;
  return shell;
}

inline void control_modal_style_toast_box(lv_obj_t *box, lv_coord_t radius,
                                          uint32_t bg_color) {
  if (!box) return;
  lv_obj_set_style_bg_color(box, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(box, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(box, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(box, radius, LV_PART_MAIN);
  lv_obj_set_style_pad_all(box, 12, LV_PART_MAIN);
  lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
}

inline ControlModalToastShell control_modal_open_toast(lv_coord_t width,
                                                       lv_coord_t radius,
                                                       uint32_t bg_color) {
  ControlModalToastShell shell;
  shell.box = lv_obj_create(lv_layer_top());
  lv_obj_set_width(shell.box, width);
  control_modal_style_toast_box(shell.box, radius, bg_color);
  return shell;
}

inline lv_obj_t *control_modal_create_title(lv_obj_t *parent,
                                            const std::string &text,
                                            lv_coord_t width,
                                            const lv_font_t *font,
                                            int width_compensation_percent) {
  lv_obj_t *title = lv_label_create(parent);
  lv_label_set_text(title, text.c_str());
  lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);
  lv_obj_set_width(title, width);
  lv_obj_set_style_text_color(title, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(title, font, LV_PART_MAIN);
  apply_width_compensation(title, width_compensation_percent);
  return title;
}

inline lv_obj_t *control_modal_create_scroll_list(lv_obj_t *parent,
                                                  lv_coord_t width,
                                                  lv_coord_t height,
                                                  lv_coord_t gap) {
  lv_obj_t *list = lv_obj_create(parent);
  lv_obj_set_size(list, width, height);
  lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(list, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(list, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(list, gap, LV_PART_MAIN);
  lv_obj_set_layout(list, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(list, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_scroll_dir(list, LV_DIR_VER);
  return list;
}

inline lv_obj_t *control_modal_create_list_row(lv_obj_t *parent,
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
  control_modal_apply_pressed_fill(btn);

  lv_obj_t *value = lv_label_create(btn);
  lv_label_set_text(value, label.c_str());
  lv_label_set_long_mode(value, LV_LABEL_LONG_DOT);
  lv_obj_set_width(value, lv_pct(100));
  lv_obj_set_style_text_color(value, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(value, font, LV_PART_MAIN);
  apply_width_compensation(value, width_compensation_percent);
  lv_obj_center(value);
  return btn;
}

inline lv_obj_t *control_modal_create_text_button(
    lv_obj_t *parent,
    const std::string &text,
    lv_coord_t max_width,
    lv_coord_t min_width,
    lv_coord_t min_height,
    lv_coord_t radius,
    uint32_t bg_color,
    const lv_font_t *font) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, min_width, min_height);
  lv_obj_set_style_radius(btn, radius, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
  control_modal_apply_pressed_fill(btn);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text.c_str());
  lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(label, font, LV_PART_MAIN);

  lv_obj_update_layout(label);
  lv_coord_t pad_x = min_height / 2;
  if (pad_x < 14) pad_x = 14;
  lv_coord_t pad_y = min_height / 5;
  if (pad_y < 8) pad_y = 8;

  lv_coord_t natural_width = lv_obj_get_width(label) + pad_x * 2;
  if (natural_width < min_width) natural_width = min_width;
  if (natural_width > max_width) natural_width = max_width;

  lv_coord_t label_width = natural_width - pad_x * 2;
  if (label_width < 24) label_width = 24;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, label_width);
  lv_obj_update_layout(label);

  lv_coord_t button_height = lv_obj_get_height(label) + pad_y * 2;
  if (button_height < min_height) button_height = min_height;
  lv_obj_set_size(btn, natural_width, button_height);
  lv_obj_center(label);
  return btn;
}
