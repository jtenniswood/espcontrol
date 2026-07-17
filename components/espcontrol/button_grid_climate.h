#pragma once

#include "climate_target_logic.h"

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Climate control card helpers ─────────────────────────────────────

constexpr uint32_t CLIMATE_COOLING_COLOR = 0x2979FF;
constexpr int CLIMATE_SUPPORT_TARGET_TEMPERATURE =
  espcontrol::climate::SUPPORT_TARGET_TEMPERATURE;
constexpr int CLIMATE_SUPPORT_TARGET_TEMPERATURE_RANGE =
  espcontrol::climate::SUPPORT_TARGET_TEMPERATURE_RANGE;
constexpr int CLIMATE_DEFAULT_TARGET_TENTHS = 200;
constexpr int CLIMATE_DEFAULT_LOW_TENTHS = 180;
constexpr int CLIMATE_DEFAULT_HIGH_TENTHS = 220;
constexpr int CLIMATE_DEFAULT_MIN_TENTHS = 50;
constexpr int CLIMATE_DEFAULT_MAX_TENTHS = 350;
constexpr int CLIMATE_DEFAULT_STEP_TENTHS = 5;
constexpr int CLIMATE_WHOLE_NUMBER_STEP_TENTHS = 10;
constexpr uint32_t CLIMATE_TEMP_DEBOUNCE_MS = 450;
constexpr int CLIMATE_MODAL_ARC_SIZE_PERCENT = 88;
constexpr int CLIMATE_MODAL_COMPACT_PORTRAIT_ARC_SIZE_PERCENT = 96;
constexpr lv_coord_t CLIMATE_MODAL_ARC_UP_REF_PX = 30;
constexpr lv_coord_t CLIMATE_MODAL_SQUARE_ARC_UP_REF_PX = 24;
constexpr lv_coord_t CLIMATE_MODAL_STEP_BUTTONS_UP_REF_PX = 42;
constexpr lv_coord_t CLIMATE_MODAL_WIDE_LANDSCAPE_STEP_BUTTONS_UP_REF_PX = 18;
constexpr lv_coord_t CLIMATE_MODAL_WIDE_LANDSCAPE_VALUE_DOWN_REF_PX = 24;
constexpr lv_coord_t CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_CHIP_BOTTOM_PX = 36;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_LANDSCAPE_STEP_BUTTONS_UP_REF_PX = 18;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_LANDSCAPE_VALUE_DOWN_REF_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_LANDSCAPE_OPTION_CHIP_BOTTOM_PX = 16;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_LANDSCAPE_CONTROLS_DOWN_REF_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_STEP_BUTTONS_UP_REF_PX = 36;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_TAB_CONTENT_GAP_REF_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_STATUS_DOWN_REF_PX = 16;
constexpr uint16_t CLIMATE_MODAL_COMPACT_PORTRAIT_TARGET_ZOOM = 214;
constexpr lv_coord_t CLIMATE_MODAL_SQUARE_STEP_BUTTONS_UP_REF_PX = 18;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_CONTROL_STEP_BUTTONS_UP_REF_PX = 18;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_LABELS_DOWN_REF_PX = 22;
constexpr lv_coord_t CLIMATE_MODAL_SQUARE_LABELS_DOWN_REF_PX = 18;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_CONTROL_LABELS_DOWN_REF_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_CONTROL_OPTION_CHIP_W_REF_PX = 200;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_CONTROL_OPTION_CHIP_GAP_REF_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_SQUARE_OPTION_CHIP_W_REF_PX = 280;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_SQUARE_TAB_REF_PX = 50;
constexpr lv_coord_t CLIMATE_MODAL_LARGE_SQUARE_TAB_CONTENT_GAP_REF_PX = 30;
constexpr lv_coord_t CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_CHIP_BOTTOM_PX = 4;
constexpr lv_coord_t CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_TILE_MAX_PX = 178;
constexpr lv_coord_t CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_TILE_MIN_PX = 132;
constexpr lv_coord_t CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_TILE_MAX_PX = 162;
constexpr lv_coord_t CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_TILE_MIN_PX = 124;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_MAX_PX = 220;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_MIN_PX = 220;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_H_PX = 120;
constexpr int CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_ICON_ZOOM = 178;
constexpr lv_coord_t CLIMATE_MODAL_OPTION_CHIP_MIN_H_PX = 56;
constexpr lv_coord_t CLIMATE_MODAL_OPTION_CHIP_PAD_Y_REF_PX = 6;
constexpr lv_coord_t CLIMATE_MODAL_OPTION_CHIP_TEXT_GAP_PX = 2;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_W_PX = 200;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_GAP_PX = 12;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_PAD_X_PX = 16;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_ICON_GAP_PX = 14;
constexpr lv_coord_t CLIMATE_MODAL_COMPACT_PORTRAIT_STEP_BUTTON_REF_PX = 96;
constexpr lv_coord_t CLIMATE_MODAL_STEP_BUTTON_GAP_REF_PX = 16;
constexpr uint16_t CLIMATE_MODAL_STEP_ICON_ZOOM = 214;
constexpr uint16_t CLIMATE_MODAL_TAB_ICON_ZOOM = 210;
constexpr int CLIMATE_OPTION_ROW_WIDTH_PERCENT = 88;
constexpr int CLIMATE_OPTION_MAX_OPTIONS = 32;
constexpr lv_coord_t CLIMATE_OPTION_MENU_ROW_MIN_H_PX = 52;
constexpr lv_coord_t CLIMATE_OPTION_MENU_ROW_PAD_Y_PX = 8;
constexpr lv_coord_t CLIMATE_OPTION_MENU_ROW_GAP_PX = 2;

struct ClimateControlCtx {
  std::string entity_id;
  std::string configured_label;
  std::string friendly_name;
  std::string hvac_mode = "off";
  std::string hvac_action;
  std::vector<std::string> hvac_modes;
  std::string fan_mode;
  std::vector<std::string> fan_modes;
  std::string swing_mode;
  std::vector<std::string> swing_modes;
  std::string preset_mode;
  std::vector<std::string> preset_modes;
  std::string options;
  bool available = true;
  bool has_target = false;
  bool has_current = false;
  bool has_low = false;
  bool has_high = false;
  bool supported_features_known = false;
  int supported_features = 0;
  bool edit_high = false;
  int current_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  int target_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  int low_tenths = CLIMATE_DEFAULT_LOW_TENTHS;
  int high_tenths = CLIMATE_DEFAULT_HIGH_TENTHS;
  int min_tenths = CLIMATE_DEFAULT_MIN_TENTHS;
  int max_tenths = CLIMATE_DEFAULT_MAX_TENTHS;
  bool custom_min = false;
  bool custom_max = false;
  bool received_min = false;
  bool received_max = false;
  int step_tenths = CLIMATE_DEFAULT_STEP_TENTHS;
  int configured_step_tenths = CLIMATE_WHOLE_NUMBER_STEP_TENTHS;
  int precision = 0;
  std::string label_display = "label";
  std::string number_display = "target";
  int pending_target_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  bool pending_temp_send = false;
  lv_timer_t *debounce_timer = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  lv_obj_t *btn = nullptr;
  lv_obj_t *icon_lbl = nullptr;
  lv_obj_t *label_lbl = nullptr;
  lv_obj_t *sensor_container = nullptr;
  lv_obj_t *value_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  const char *icon_off_glyph = nullptr;
  const char *icon_on_glyph = nullptr;
  int width_compensation_percent = 100;
  const lv_font_t *number_font = nullptr;
  const lv_font_t *range_number_font = nullptr;
  const lv_font_t *unit_font = nullptr;
  const lv_font_t *label_font = nullptr;
  const lv_font_t *option_title_font = nullptr;
  const lv_font_t *option_value_font = nullptr;
  const lv_font_t *option_menu_font = nullptr;
  const lv_font_t *card_icon_font = nullptr;
  const lv_font_t *icon_font = nullptr;
};

struct ClimateOptionClick {
  ClimateControlCtx *ctx = nullptr;
  std::string kind;
  std::string value;
};

enum class ClimateControlTab : uint8_t {
  TEMPERATURE = 0,
  MODE = 1,
  PRESET = 2,
  FAN = 3,
  SWING = 4,
};

struct ClimateControlVisibleTabs {
  ClimateControlTab tabs[5] = {
    ClimateControlTab::TEMPERATURE,
    ClimateControlTab::MODE,
    ClimateControlTab::PRESET,
    ClimateControlTab::FAN,
    ClimateControlTab::SWING,
  };
  uint8_t count = 0;

  bool contains(ClimateControlTab tab) const {
    for (uint8_t i = 0; i < count; i++) {
      if (tabs[i] == tab) return true;
    }
    return false;
  }

  void add(ClimateControlTab tab) {
    if (count >= 5 || contains(tab)) return;
    tabs[count++] = tab;
  }
};

inline ClimateControlVisibleTabs climate_control_visible_tabs(ClimateControlCtx *ctx);

struct ClimateControlModalUi {
  lv_obj_t *overlay = nullptr;
  lv_obj_t *panel = nullptr;
  lv_obj_t *back_btn = nullptr;
  lv_obj_t *tab_row = nullptr;
  lv_obj_t *temperature_tab = nullptr;
  lv_obj_t *mode_tab = nullptr;
  lv_obj_t *preset_tab = nullptr;
  lv_obj_t *fan_tab = nullptr;
  lv_obj_t *swing_tab = nullptr;
  lv_obj_t *mode_btn = nullptr;
  lv_obj_t *menu_view = nullptr;
  lv_obj_t *menu_close_btn = nullptr;
  lv_obj_t *menu_mode_btn = nullptr;
  lv_obj_t *menu_preset_btn = nullptr;
  lv_obj_t *option_list_view = nullptr;
  lv_obj_t *arc = nullptr;
  lv_obj_t *heat_arc = nullptr;
  lv_obj_t *cool_arc = nullptr;
  lv_obj_t *current_dot = nullptr;
  lv_obj_t *handle_dot = nullptr;
  lv_obj_t *low_handle_dot = nullptr;
  lv_obj_t *high_handle_dot = nullptr;
  lv_obj_t *target_row = nullptr;
  lv_obj_t *target_lbl = nullptr;
  lv_obj_t *low_target_lbl = nullptr;
  lv_obj_t *target_separator_lbl = nullptr;
  lv_obj_t *high_target_lbl = nullptr;
  lv_obj_t *unit_lbl = nullptr;
  lv_obj_t *status_lbl = nullptr;
  lv_obj_t *target_chip = nullptr;
  lv_obj_t *minus_btn = nullptr;
  lv_obj_t *plus_btn = nullptr;
  lv_obj_t *chips = nullptr;
  lv_obj_t *mode_chip = nullptr;
  lv_obj_t *preset_chip = nullptr;
  lv_obj_t *fan_chip = nullptr;
  lv_obj_t *swing_chip = nullptr;
  lv_obj_t *menu_overlay = nullptr;
  ClimateOptionClick option_clicks[CLIMATE_OPTION_MAX_OPTIONS];
  ClimateControlCtx *active = nullptr;
  int option_click_count = 0;
  bool updating_arc = false;
  bool dragging_arc = false;
  bool drag_target_selected = false;
  bool has_drag_preview = false;
  int drag_preview_tenths = CLIMATE_DEFAULT_TARGET_TENTHS;
  bool action_menu_open = false;
  ClimateControlTab tab = ClimateControlTab::TEMPERATURE;
};

inline ClimateControlModalUi &climate_control_modal_ui() {
  static ClimateControlModalUi ui;
  return ui;
}

inline ClimateOptionClick *climate_next_option_click(ClimateControlModalUi &ui,
                                                     ClimateControlCtx *ctx,
                                                     const std::string &kind,
                                                     const std::string &value) {
  if (ui.option_click_count >= CLIMATE_OPTION_MAX_OPTIONS) return nullptr;
  ClimateOptionClick *click = &ui.option_clicks[ui.option_click_count++];
  click->ctx = ctx;
  click->kind = kind;
  click->value = value;
  return click;
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

inline bool climate_parse_supported_features(esphome::StringRef value, int &features) {
  std::string text = climate_lower(climate_trim(
    string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
  if (climate_unavailable_value(text) || text == "none" || text == "null") return false;
  char *end = nullptr;
  long parsed = std::strtol(text.c_str(), &end, 10);
  if (end == text.c_str()) return false;
  features = static_cast<int>(parsed);
  return true;
}

inline void climate_normalize_range(ClimateControlCtx *ctx) {
  if (!ctx || ctx->max_tenths > ctx->min_tenths) return;
  bool min_known = ctx->custom_min || ctx->received_min;
  bool max_known = ctx->custom_max || ctx->received_max;
  if (!min_known && max_known) {
    ctx->min_tenths = ctx->max_tenths - 10;
  } else {
    ctx->max_tenths = ctx->min_tenths + 10;
  }
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
  climate_normalize_range(ctx);
}

inline int climate_clamp_tenths(ClimateControlCtx *ctx, int value) {
  if (!ctx) return value;
  climate_normalize_range(ctx);
  if (value < ctx->min_tenths) value = ctx->min_tenths;
  if (value > ctx->max_tenths) value = ctx->max_tenths;
  return value;
}

inline int climate_effective_step_tenths(ClimateControlCtx *ctx) {
  if (!ctx) return CLIMATE_DEFAULT_STEP_TENTHS;
  if (ctx->configured_step_tenths == CLIMATE_DEFAULT_STEP_TENTHS ||
      ctx->configured_step_tenths == CLIMATE_WHOLE_NUMBER_STEP_TENTHS)
    return ctx->configured_step_tenths;
  return CLIMATE_WHOLE_NUMBER_STEP_TENTHS;
}

inline int climate_round_to_step(ClimateControlCtx *ctx, int value) {
  if (!ctx) return value;
  int step = climate_effective_step_tenths(ctx);
  int base = ctx->precision <= 0 ? 0 : ctx->min_tenths;
  int delta = value - base;
  int rounded = base + ((delta >= 0 ? delta + step / 2 : delta - step / 2) / step) * step;
  return climate_clamp_tenths(ctx, rounded);
}

using ClimateTargetKind = espcontrol::climate::TargetKind;

inline ClimateTargetKind climate_target_kind(ClimateControlCtx *ctx) {
  if (!ctx) return ClimateTargetKind::NONE;
  return espcontrol::climate::target_kind(
    ctx->supported_features_known, ctx->supported_features,
    ctx->has_target, ctx->has_low, ctx->has_high);
}

inline bool climate_dual_target(ClimateControlCtx *ctx) {
  return climate_target_kind(ctx) == ClimateTargetKind::RANGE;
}

inline bool climate_target_values_complete(ClimateControlCtx *ctx) {
  if (!ctx) return false;
  ClimateTargetKind kind = climate_target_kind(ctx);
  bool complete = espcontrol::climate::target_values_complete(
    kind, ctx->has_target, ctx->has_low, ctx->has_high);
  if (complete && kind == ClimateTargetKind::RANGE)
    return ctx->low_tenths < ctx->high_tenths;
  return complete;
}

inline void climate_select_target_for_mode(ClimateControlCtx *ctx,
                                           const std::string &mode) {
  if (!ctx) return;
  espcontrol::climate::TargetSelection selection =
    espcontrol::climate::target_selection_for_mode(mode);
  if (selection == espcontrol::climate::TargetSelection::HIGH) ctx->edit_high = true;
  else if (selection == espcontrol::climate::TargetSelection::LOW) ctx->edit_high = false;
}

inline void climate_select_nearest_target(ClimateControlCtx *ctx, int value) {
  if (!ctx || !climate_dual_target(ctx) || !ctx->has_low || !ctx->has_high) return;
  ctx->edit_high = espcontrol::climate::nearest_target_is_high(
    value, ctx->low_tenths, ctx->high_tenths);
}

inline int climate_selected_target(ClimateControlCtx *ctx) {
  if (!ctx) return CLIMATE_DEFAULT_TARGET_TENTHS;
  ClimateTargetKind kind = climate_target_kind(ctx);
  if (kind == ClimateTargetKind::RANGE && ctx->has_low && ctx->has_high)
    return ctx->edit_high ? ctx->high_tenths : ctx->low_tenths;
  if (kind == ClimateTargetKind::SINGLE && ctx->has_target) return ctx->target_tenths;
  return climate_clamp_tenths(ctx, CLIMATE_DEFAULT_TARGET_TENTHS);
}

inline int climate_display_target(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ctx && ui.active == ctx && ui.dragging_arc && ui.has_drag_preview)
    return climate_clamp_tenths(ctx, ui.drag_preview_tenths);
  return climate_selected_target(ctx);
}

inline int climate_display_low_target(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ctx && ui.active == ctx && ui.dragging_arc && ui.has_drag_preview &&
      !ctx->edit_high)
    return climate_clamp_tenths(ctx, ui.drag_preview_tenths);
  return ctx ? ctx->low_tenths : CLIMATE_DEFAULT_LOW_TENTHS;
}

inline int climate_display_high_target(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ctx && ui.active == ctx && ui.dragging_arc && ui.has_drag_preview &&
      ctx->edit_high)
    return climate_clamp_tenths(ctx, ui.drag_preview_tenths);
  return ctx ? ctx->high_tenths : CLIMATE_DEFAULT_HIGH_TENTHS;
}

inline int climate_constrain_selected_target(ClimateControlCtx *ctx, int value) {
  if (!ctx) return CLIMATE_DEFAULT_TARGET_TENTHS;
  value = climate_clamp_tenths(ctx, value);
  if (climate_dual_target(ctx)) {
    int gap = climate_effective_step_tenths(ctx);
    value = espcontrol::climate::constrain_range_target(
      value, ctx->edit_high, ctx->low_tenths, ctx->high_tenths,
      ctx->min_tenths, ctx->max_tenths, gap);
  }
  return value;
}

inline std::string climate_format_tenths(int value, int precision) {
  char buf[20];
  if (precision <= 0) {
    int rounded = (value >= 0 ? value + 5 : value - 5) / 10;
    snprintf(buf, sizeof(buf), "%d", rounded);
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

inline int climate_target_display_precision(ClimateControlCtx *ctx) {
  if (!ctx) return 0;
  return ctx->configured_step_tenths == CLIMATE_DEFAULT_STEP_TENTHS && ctx->precision <= 0
    ? 1
    : ctx->precision;
}

inline std::string climate_option_label(const std::string &raw) {
  std::string value = climate_lower(climate_trim(raw));
  if (value == "off") return espcontrol_i18n(std::string("Off"));
  if (value == "heat") return espcontrol_i18n(std::string("Heat"));
  if (value == "cool") return espcontrol_i18n(std::string("Cool"));
  if (value == "heat_cool") return espcontrol_i18n(std::string("Heat/Cool"));
  if (value == "auto") return espcontrol_i18n(std::string("Auto"));
  if (value == "dry") return espcontrol_i18n(std::string("Dry"));
  if (value == "fan_only") return espcontrol_i18n(std::string("Fan"));
  if (value == "nohold") return espcontrol_i18n(std::string("Nohold"));
  if (value == "holduntil" || value == "hold_until") return espcontrol_i18n(std::string("Holduntil"));
  if (value == "permanenthold" || value == "permanent_hold") return espcontrol_i18n(std::string("Permanenthold"));
  if (value == "temporaryhold" || value == "temporary_hold") return espcontrol_i18n(std::string("Temporaryhold"));
  if (value == "vacationhold" || value == "vacation_hold") return espcontrol_i18n(std::string("Vacationhold"));
  return espcontrol_i18n(sentence_cap_text(value));
}

inline size_t climate_utf8_char_count(const std::string &value) {
  size_t count = 0;
  for (unsigned char ch : value) {
    if ((ch & 0xC0) != 0x80) count++;
  }
  return count;
}

inline lv_coord_t climate_option_menu_width(const std::vector<std::string> &options,
                                            const std::string &,
                                            const lv_font_t *font) {
  lv_coord_t line_h = font && font->line_height > 0 ? font->line_height : 32;
  lv_coord_t widest_text = 0;
  for (const auto &option : options) {
    size_t len = climate_utf8_char_count(climate_option_label(option));
    lv_coord_t estimated_w = static_cast<lv_coord_t>(len) * line_h * 3 / 5;
    if (estimated_w > widest_text) widest_text = estimated_w;
  }
  lv_coord_t width = widest_text + 72;
  if (width < 180) width = 180;
  if (width > 340) width = 340;
  return width;
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

inline bool climate_action_is_working(const std::string &action) {
  return action == "heating" || action == "cooling" ||
         action == "drying" || action == "fan";
}

inline std::string climate_action_label(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return espcontrol_i18n(std::string("Unavailable"));
  if (ctx->hvac_action == "heating") return espcontrol_i18n(std::string("Heating"));
  if (ctx->hvac_action == "cooling") return espcontrol_i18n(std::string("Cooling"));
  if (ctx->hvac_action == "drying") return espcontrol_i18n(std::string("Drying"));
  if (ctx->hvac_action == "fan") return espcontrol_i18n(std::string("Fan"));
  if (ctx->hvac_mode == "off") return espcontrol_i18n(std::string("Off"));
  if (ctx->hvac_action.empty() || ctx->hvac_action == "unknown" ||
      ctx->hvac_action == "unavailable") return climate_option_label(ctx->hvac_mode);
  if (ctx->hvac_action == "idle") return espcontrol_i18n(std::string("Idle"));
  if (ctx->hvac_action == "off") return espcontrol_i18n(std::string("Off"));
  return espcontrol_i18n(std::string("Idle"));
}

inline bool climate_is_active(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return false;
  if (climate_action_is_working(ctx->hvac_action)) return true;
  if (ctx->hvac_mode == "off") return false;
  if (ctx->hvac_action.empty() || ctx->hvac_action == "unknown" ||
      ctx->hvac_action == "unavailable") {
    return !climate_unavailable_value(ctx->hvac_mode);
  }
  return !(ctx->hvac_action == "idle" || ctx->hvac_action == "off");
}

inline bool climate_temperature_controls_enabled(ClimateControlCtx *ctx) {
  return ctx && ctx->available &&
         (ctx->hvac_mode != "off" || climate_action_is_working(ctx->hvac_action));
}

inline bool climate_modal_temperature_controls_enabled(ClimateControlCtx *ctx) {
  return ctx && ctx->available && climate_target_values_complete(ctx);
}

inline bool climate_temperature_target_available(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return false;
  return !ctx->supported_features_known ||
         climate_target_kind(ctx) != ClimateTargetKind::NONE;
}

inline bool climate_control_tab_from_token(const std::string &value, ClimateControlTab &tab) {
  if (value == "temperature") {
    tab = ClimateControlTab::TEMPERATURE;
    return true;
  }
  if (value == "mode") {
    tab = ClimateControlTab::MODE;
    return true;
  }
  if (value == "preset") {
    tab = ClimateControlTab::PRESET;
    return true;
  }
  if (value == "fan") {
    tab = ClimateControlTab::FAN;
    return true;
  }
  if (value == "swing") {
    tab = ClimateControlTab::SWING;
    return true;
  }
  return false;
}

inline bool climate_control_tab_supported(ClimateControlCtx *ctx, ClimateControlTab tab) {
  if (!ctx || !ctx->available) return false;
  switch (tab) {
    case ClimateControlTab::TEMPERATURE:
      return climate_temperature_target_available(ctx);
    case ClimateControlTab::MODE:
      return !ctx->hvac_modes.empty();
    case ClimateControlTab::PRESET:
      return !ctx->preset_modes.empty();
    case ClimateControlTab::FAN:
      return !ctx->fan_modes.empty();
    case ClimateControlTab::SWING:
      return !ctx->swing_modes.empty();
  }
  return false;
}

inline ClimateControlVisibleTabs climate_control_visible_tabs(ClimateControlCtx *ctx) {
  ClimateControlVisibleTabs visible;
  std::string value = cfg_option_value(ctx ? ctx->options : "", CLIMATE_CONTROL_TABS_OPTION);
  if (value.empty()) value = CLIMATE_CONTROL_DEFAULT_TABS_VALUE;

  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find('|', start);
    std::string token = value.substr(start, end == std::string::npos ? std::string::npos : end - start);
    ClimateControlTab tab = ClimateControlTab::TEMPERATURE;
    if (climate_control_tab_from_token(token, tab) &&
        climate_control_tab_supported(ctx, tab)) {
      visible.add(tab);
    }
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (visible.count == 0 && climate_control_tab_supported(ctx, ClimateControlTab::TEMPERATURE)) {
    visible.add(ClimateControlTab::TEMPERATURE);
  }
  if (visible.count == 0) {
    ClimateControlTab fallbacks[4] = {
      ClimateControlTab::MODE,
      ClimateControlTab::PRESET,
      ClimateControlTab::FAN,
      ClimateControlTab::SWING,
    };
    for (ClimateControlTab fallback : fallbacks) {
      if (climate_control_tab_supported(ctx, fallback)) {
        visible.add(fallback);
        break;
      }
    }
  }
  return visible;
}

inline bool climate_control_tab_visible(ClimateControlCtx *ctx, ClimateControlTab tab) {
  ClimateControlVisibleTabs tabs = climate_control_visible_tabs(ctx);
  return tabs.contains(tab);
}

inline ClimateControlTab climate_control_first_visible_tab(ClimateControlCtx *ctx) {
  ClimateControlVisibleTabs tabs = climate_control_visible_tabs(ctx);
  return tabs.count == 0 ? ClimateControlTab::TEMPERATURE : tabs.tabs[0];
}

inline ControlModalTabLayout climate_control_calc_tab_layout(
    ClimateControlCtx *ctx, const ControlModalLayout &layout) {
  ClimateControlVisibleTabs visible_tabs = climate_control_visible_tabs(ctx);
  int tab_count = static_cast<int>(visible_tabs.count);
  bool show_tab_bar = tab_count > 1;
  return control_modal_calc_tab_layout(layout, tab_count, show_tab_bar);
}

inline void climate_control_ensure_visible_tab(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (climate_control_tab_visible(ctx, ui.tab)) return;
  ui.tab = climate_control_first_visible_tab(ctx);
}

inline lv_obj_t *climate_control_tab_button(ClimateControlModalUi &ui, ClimateControlTab tab) {
  switch (tab) {
    case ClimateControlTab::TEMPERATURE: return ui.temperature_tab;
    case ClimateControlTab::MODE: return ui.mode_tab;
    case ClimateControlTab::PRESET: return ui.preset_tab;
    case ClimateControlTab::FAN: return ui.fan_tab;
    case ClimateControlTab::SWING: return ui.swing_tab;
  }
  return nullptr;
}

inline float climate_arc_angle_for_tenths(ClimateControlCtx *ctx, int value) {
  if (!ctx || ctx->max_tenths <= ctx->min_tenths) return 135.0f;
  value = climate_clamp_tenths(ctx, value);
  int span = ctx->max_tenths - ctx->min_tenths;
  float offset = (float)(value - ctx->min_tenths) * 270.0f / (float)span;
  float angle = 135.0f + offset;
  return angle >= 360.0f ? angle - 360.0f : angle;
}

inline void climate_layout_arc_dot(ClimateControlCtx *ctx, const ControlModalLayout &layout,
                                   lv_obj_t *dot, int tenths, lv_coord_t size,
                                   lv_coord_t radius) {
  if (!ctx || !dot) return;
  float angle = climate_arc_angle_for_tenths(ctx, tenths);
  float radians = angle * 3.14159265f / 180.0f;
  float x_radius = radius;
  float y_radius = radius;
  int width_percent = normalize_width_compensation_percent(ctx->width_compensation_percent);
  lv_coord_t visible_w = layout.arc_size;
  lv_coord_t visible_h = layout.arc_size;
  if (width_compensation_vertical_axis()) {
    y_radius = y_radius * width_percent / 100.0f;
    visible_h = layout.arc_size * width_percent / 100;
  } else {
    x_radius = x_radius * width_percent / 100.0f;
    visible_w = layout.arc_size * width_percent / 100;
  }
  lv_coord_t arc_left = layout.panel_w / 2 + layout.arc_center_x - layout.arc_size / 2;
  lv_coord_t arc_top = layout.panel_h / 2 + layout.arc_center_y - layout.arc_size / 2;
  lv_coord_t center_x = arc_left + visible_w / 2;
  lv_coord_t center_y = arc_top + visible_h / 2;
  lv_obj_set_size(dot, size, size);
  lv_obj_set_style_radius(dot, size / 2, LV_PART_MAIN);
  lv_obj_set_pos(dot,
    center_x + (lv_coord_t)(std::cos(radians) * x_radius) - size / 2,
    center_y + (lv_coord_t)(std::sin(radians) * y_radius) - size / 2);
}

inline void climate_layout_current_dot(ClimateControlCtx *ctx, const ControlModalLayout &layout) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  lv_coord_t dot_size = control_modal_scaled_px(10, layout.short_side);
  if (dot_size < 8) dot_size = 8;
  lv_coord_t radius = layout.arc_size / 2 - layout.arc_stroke / 2;
  if (radius < 0) radius = layout.arc_size / 2;
  climate_layout_arc_dot(ctx, layout, ui.current_dot, ctx ? ctx->current_tenths : CLIMATE_DEFAULT_TARGET_TENTHS, dot_size, radius);
}

inline void climate_layout_handle_dot(ClimateControlCtx *ctx, const ControlModalLayout &layout) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  lv_coord_t pad = layout.short_side < 520 ? 4 : 6;
  lv_coord_t handle_size = layout.arc_stroke + pad * 2;
  lv_coord_t radius = layout.arc_size / 2 - layout.arc_stroke / 2;
  if (radius < 0) radius = layout.arc_size / 2;
  climate_layout_arc_dot(ctx, layout, ui.handle_dot, climate_display_target(ctx), handle_size, radius);
}

inline void climate_layout_dual_handle_dots(ClimateControlCtx *ctx,
                                            const ControlModalLayout &layout) {
  if (!ctx) return;
  ClimateControlModalUi &ui = climate_control_modal_ui();
  lv_coord_t pad = layout.short_side < 520 ? 4 : 6;
  lv_coord_t handle_size = layout.arc_stroke + pad * 2;
  lv_coord_t radius = layout.arc_size / 2 - layout.arc_stroke / 2;
  if (radius < 0) radius = layout.arc_size / 2;
  if (ctx->has_low)
    climate_layout_arc_dot(ctx, layout, ui.low_handle_dot,
                           climate_display_low_target(ctx),
                           handle_size, radius);
  if (ctx->has_high)
    climate_layout_arc_dot(ctx, layout, ui.high_handle_dot,
                           climate_display_high_target(ctx),
                           handle_size, radius);
}

inline void climate_style_dual_handle(lv_obj_t *dot, bool selected) {
  if (!dot) return;
  lv_obj_set_style_bg_color(dot, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_border_color(dot, lv_color_hex(DARK_TRACK_BACKGROUND), LV_PART_MAIN);
  lv_obj_set_style_border_width(dot, selected ? 3 : 0, LV_PART_MAIN);
}

inline void climate_apply_background_arc_width(lv_obj_t *arc, const ControlModalLayout &layout) {
  if (!arc) return;
  lv_coord_t extra = control_modal_scaled_px(4, layout.short_side);
  if (extra < 2) extra = 2;
  lv_obj_set_style_arc_width(arc, layout.arc_stroke + extra, LV_PART_MAIN);
}

inline bool climate_control_uses_square_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_square_tuning(layout);
}

inline bool climate_control_uses_compact_control_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_compact_control_tuning(layout);
}

inline bool climate_control_uses_large_square_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_large_square_tuning(layout);
}

inline bool climate_control_uses_large_landscape_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_large_landscape_tuning(layout);
}

inline bool climate_control_uses_wide_landscape_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_wide_landscape_tuning(layout);
}

inline bool climate_control_uses_compact_portrait_modal_tuning(const ControlModalLayout &layout) {
  return control_modal_uses_compact_portrait_tuning(layout) &&
         !climate_control_uses_compact_control_modal_tuning(layout) &&
         layout.sh > layout.sw;
}

inline lv_coord_t climate_control_step_buttons_up_ref(const ControlModalLayout &layout) {
  if (climate_control_uses_wide_landscape_modal_tuning(layout))
    return CLIMATE_MODAL_WIDE_LANDSCAPE_STEP_BUTTONS_UP_REF_PX;
  if (climate_control_uses_large_landscape_modal_tuning(layout))
    return CLIMATE_MODAL_LARGE_LANDSCAPE_STEP_BUTTONS_UP_REF_PX;
  if (climate_control_uses_compact_portrait_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_PORTRAIT_STEP_BUTTONS_UP_REF_PX;
  if (climate_control_uses_compact_control_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_CONTROL_STEP_BUTTONS_UP_REF_PX;
  if (climate_control_uses_square_modal_tuning(layout))
    return CLIMATE_MODAL_SQUARE_STEP_BUTTONS_UP_REF_PX;
  return CLIMATE_MODAL_STEP_BUTTONS_UP_REF_PX;
}

inline lv_coord_t climate_control_step_buttons_down_ref(const ControlModalLayout &layout) {
  return 0;
}

inline lv_coord_t climate_control_labels_down_ref(const ControlModalLayout &layout) {
  if (climate_control_uses_wide_landscape_modal_tuning(layout))
    return CLIMATE_MODAL_WIDE_LANDSCAPE_VALUE_DOWN_REF_PX;
  if (climate_control_uses_large_landscape_modal_tuning(layout))
    return CLIMATE_MODAL_LARGE_LANDSCAPE_VALUE_DOWN_REF_PX;
  if (climate_control_uses_compact_portrait_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_PORTRAIT_LABELS_DOWN_REF_PX;
  if (climate_control_uses_compact_control_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_CONTROL_LABELS_DOWN_REF_PX;
  if (climate_control_uses_square_modal_tuning(layout))
    return CLIMATE_MODAL_SQUARE_LABELS_DOWN_REF_PX;
  return 0;
}

inline lv_coord_t climate_control_tab_content_gap(const ControlModalLayout &layout) {
  return control_modal_shared_tab_content_gap(layout);
}

inline lv_coord_t climate_control_status_translate_y(const ControlModalLayout &layout) {
  return climate_control_uses_compact_portrait_modal_tuning(layout)
    ? control_modal_scaled_px(CLIMATE_MODAL_COMPACT_PORTRAIT_STATUS_DOWN_REF_PX, layout.short_side)
    : 0;
}

inline uint16_t climate_control_target_zoom(const ControlModalLayout &layout) {
  return climate_control_uses_compact_portrait_modal_tuning(layout)
    ? CLIMATE_MODAL_COMPACT_PORTRAIT_TARGET_ZOOM
    : 256;
}

inline lv_coord_t climate_font_line_height(const lv_font_t *font, lv_coord_t fallback) {
  return font && font->line_height > 0 ? font->line_height : fallback;
}

inline lv_coord_t climate_option_chip_pad_y(const ControlModalLayout &layout) {
  lv_coord_t pad = control_modal_scaled_px(CLIMATE_MODAL_OPTION_CHIP_PAD_Y_REF_PX, layout.short_side);
  return pad < 4 ? 4 : pad;
}

inline lv_coord_t climate_option_chip_height(ClimateControlCtx *ctx,
                                             const ControlModalLayout &layout) {
  lv_coord_t fallback_text_h = control_modal_scaled_px(22, layout.short_side);
  lv_coord_t fallback_icon_h = control_modal_scaled_px(33, layout.short_side);
  lv_coord_t title_h = climate_font_line_height(
    ctx ? ctx->option_title_font : nullptr, fallback_text_h);
  lv_coord_t value_h = climate_font_line_height(
    ctx ? ctx->option_value_font : nullptr, fallback_text_h);
  lv_coord_t icon_h = climate_font_line_height(
    ctx ? (ctx->card_icon_font ? ctx->card_icon_font : ctx->icon_font) : nullptr,
    fallback_icon_h);
  lv_coord_t text_h = title_h + CLIMATE_MODAL_OPTION_CHIP_TEXT_GAP_PX + value_h;
  lv_coord_t content_h = icon_h > text_h ? icon_h : text_h;
  lv_coord_t chip_h = content_h + climate_option_chip_pad_y(layout) * 2;
  return chip_h < CLIMATE_MODAL_OPTION_CHIP_MIN_H_PX
    ? CLIMATE_MODAL_OPTION_CHIP_MIN_H_PX
    : chip_h;
}

inline bool climate_control_uses_wide_landscape_option_fit(const ControlModalLayout &layout) {
  return climate_control_uses_large_landscape_modal_tuning(layout) ||
         climate_control_uses_wide_landscape_modal_tuning(layout) ||
         climate_control_uses_compact_portrait_modal_tuning(layout);
}

inline lv_coord_t climate_control_wide_option_tile_max(const ControlModalLayout &layout) {
  if (climate_control_uses_compact_portrait_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_MAX_PX;
  return climate_control_uses_wide_landscape_modal_tuning(layout)
    ? CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_TILE_MAX_PX
    : CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_TILE_MAX_PX;
}

inline lv_coord_t climate_control_wide_option_tile_min(const ControlModalLayout &layout) {
  if (climate_control_uses_compact_portrait_modal_tuning(layout))
    return CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_MIN_PX;
  return climate_control_uses_wide_landscape_modal_tuning(layout)
    ? CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_TILE_MIN_PX
    : CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_TILE_MIN_PX;
}

inline lv_coord_t climate_control_option_tile_height(const ControlModalLayout &layout,
                                                     lv_coord_t tile_w) {
  return climate_control_uses_compact_portrait_modal_tuning(layout)
    ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_TILE_H_PX
    : tile_w;
}

inline void climate_apply_bottom_chip_padding(lv_obj_t *chip,
                                              const ControlModalLayout &layout) {
  if (!chip) return;
  lv_coord_t pad_y = climate_option_chip_pad_y(layout);
  bool compact_portrait = climate_control_uses_compact_portrait_modal_tuning(layout);
  lv_obj_set_style_pad_top(chip, pad_y, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(chip, pad_y, LV_PART_MAIN);
  lv_obj_set_style_pad_left(chip,
    compact_portrait ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_PAD_X_PX : 26,
    LV_PART_MAIN);
  lv_obj_set_style_pad_right(chip,
    compact_portrait ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_PAD_X_PX : 22,
    LV_PART_MAIN);
  lv_obj_set_style_pad_column(chip,
    compact_portrait ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_ICON_GAP_PX : 24,
    LV_PART_MAIN);
  lv_obj_t *icon_lbl = lv_obj_get_child(chip, 0);
  lv_obj_t *text_col = lv_obj_get_child(chip, 1);
  if (icon_lbl) lv_obj_set_style_translate_y(icon_lbl, 0, LV_PART_MAIN);
  if (text_col) lv_obj_set_style_translate_y(text_col, 0, LV_PART_MAIN);
}

inline ControlModalLayout climate_control_calc_layout(ClimateControlCtx *ctx) {
  ControlModalLayout layout = control_modal_calc_layout(
    ctx ? ctx->width_compensation_percent : 100,
    false);
  int arc_size_percent = climate_control_uses_compact_portrait_modal_tuning(layout)
    ? CLIMATE_MODAL_COMPACT_PORTRAIT_ARC_SIZE_PERCENT
    : CLIMATE_MODAL_ARC_SIZE_PERCENT;
  layout.arc_size = layout.arc_size * arc_size_percent / 100;
  if (layout.arc_size < 74) layout.arc_size = 74;

  int width_percent = normalize_width_compensation_percent(ctx ? ctx->width_compensation_percent : 100);
  lv_coord_t visible_arc_w = compensated_width(layout.arc_size, width_percent);
  layout.arc_center_x = (layout.arc_size - visible_arc_w) / 2;
  lv_coord_t arc_up_ref = climate_control_uses_square_modal_tuning(layout)
    ? CLIMATE_MODAL_SQUARE_ARC_UP_REF_PX
    : CLIMATE_MODAL_ARC_UP_REF_PX;
  layout.arc_center_y = -control_modal_scaled_px(arc_up_ref, layout.short_side);
  if (climate_control_uses_large_landscape_modal_tuning(layout)) {
    layout.arc_center_y += control_modal_scaled_px(
      CLIMATE_MODAL_LARGE_LANDSCAPE_CONTROLS_DOWN_REF_PX, layout.short_side);
  }
  layout.value_center_y = layout.arc_center_y + layout.arc_stroke / 2;
  layout.controls_center_y = layout.arc_center_y + layout.arc_size / 2 -
    layout.btn_size / 2 - layout.inset +
    control_modal_controls_down_px(layout);
  ControlModalTabLayout tabs_layout = climate_control_calc_tab_layout(ctx, layout);
  if (tabs_layout.show_tab_bar) {
    lv_coord_t tab_bottom = layout.inset + 2 + tabs_layout.tab_frame_h;
    lv_coord_t desired_control_top = tab_bottom + climate_control_tab_content_gap(layout);
    lv_coord_t current_control_top = layout.panel_h / 2 + layout.arc_center_y - layout.arc_size / 2;
    if (current_control_top < desired_control_top) {
      lv_coord_t control_shift = desired_control_top - current_control_top;
      layout.arc_center_y += control_shift;
      layout.value_center_y += control_shift;
      layout.controls_center_y += control_shift;
    }
  }
  return layout;
}

inline void climate_raise_arc_markers() {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.current_dot) lv_obj_move_foreground(ui.current_dot);
  if (ui.handle_dot) lv_obj_move_foreground(ui.handle_dot);
  if (ui.low_handle_dot) lv_obj_move_foreground(ui.low_handle_dot);
  if (ui.high_handle_dot) lv_obj_move_foreground(ui.high_handle_dot);
}

inline bool climate_uses_cooling_arc(ClimateControlCtx *ctx) {
  return ctx && ctx->available && !climate_dual_target(ctx) &&
         ctx->hvac_mode == "cool";
}

inline bool climate_has_active_arc_mode(ClimateControlCtx *ctx) {
  return ctx && ctx->available && ctx->hvac_mode != "off" &&
         !climate_unavailable_value(ctx->hvac_mode);
}

inline uint32_t climate_modal_arc_color(ClimateControlCtx *ctx) {
  if (!climate_has_active_arc_mode(ctx)) return SECONDARY_GREY;
  return ctx ? ctx->accent_color : DEFAULT_SLIDER_COLOR;
}

inline uint32_t climate_heating_color(ClimateControlCtx *ctx) {
  return ctx ? ctx->accent_color : DEFAULT_SLIDER_COLOR;
}

inline int climate_modal_arc_value(ClimateControlCtx *ctx, bool temp_enabled, int target) {
  if (!ctx) return CLIMATE_DEFAULT_TARGET_TENTHS;
  int value = temp_enabled ? climate_clamp_tenths(ctx, target) : ctx->min_tenths;
  if (climate_uses_cooling_arc(ctx)) return ctx->min_tenths + ctx->max_tenths - value;
  return value;
}

inline int climate_target_from_modal_arc_value(ClimateControlCtx *ctx, int value) {
  if (!ctx) return CLIMATE_DEFAULT_TARGET_TENTHS;
  value = climate_clamp_tenths(ctx, value);
  if (climate_uses_cooling_arc(ctx)) return ctx->min_tenths + ctx->max_tenths - value;
  return value;
}

inline uint32_t climate_active_color(ClimateControlCtx *ctx) {
  if (!ctx) return DEFAULT_SLIDER_COLOR;
  return ctx->accent_color;
}

inline void climate_apply_step_button_icon_size(lv_obj_t *btn) {
  if (!btn || CLIMATE_MODAL_STEP_ICON_ZOOM == 256) return;
  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (!label) return;
  lv_obj_update_layout(label);
  lv_coord_t offset_x = lv_obj_get_width(label) * (256 - CLIMATE_MODAL_STEP_ICON_ZOOM) / 512;
  lv_coord_t offset_y = lv_obj_get_height(label) * (256 - CLIMATE_MODAL_STEP_ICON_ZOOM) / 512;
  lv_obj_set_style_transform_zoom(label, CLIMATE_MODAL_STEP_ICON_ZOOM, LV_PART_MAIN);
  lv_obj_align(label, LV_ALIGN_CENTER, offset_x, offset_y);
}

inline std::string climate_card_target_value(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return "--";
  int precision = climate_target_display_precision(ctx);
  ClimateTargetKind kind = climate_target_kind(ctx);
  if (kind == ClimateTargetKind::RANGE && ctx->has_low && ctx->has_high)
    return climate_format_tenths(ctx->low_tenths, precision) + "-" +
           climate_format_tenths(ctx->high_tenths, precision);
  if (kind == ClimateTargetKind::SINGLE && ctx->has_target)
    return climate_format_tenths(ctx->target_tenths, precision);
  return "--";
}

inline std::string climate_card_actual_value(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available || !ctx->has_current) return "--";
  return climate_format_tenths(ctx->current_tenths, ctx->precision);
}

inline std::string climate_card_value_with_unit(const std::string &value) {
  if (value.empty() || value == "--") return "--";
  return value + display_temperature_unit_symbol();
}

inline std::string climate_card_value(ClimateControlCtx *ctx) {
  if (!ctx) return "--";
  return ctx->number_display == "actual"
    ? climate_card_actual_value(ctx)
    : climate_card_target_value(ctx);
}

inline std::string climate_card_label(ClimateControlCtx *ctx) {
  if (!ctx) return espcontrol_i18n(std::string("Climate"));
  if (ctx->label_display == "status") return climate_action_label(ctx);
  if (ctx->label_display == "actual") return climate_card_value_with_unit(climate_card_actual_value(ctx));
  if (ctx->label_display == "target") return climate_card_value_with_unit(climate_card_target_value(ctx));
  return ctx->configured_label.empty() ? espcontrol_i18n(std::string("Climate")) : ctx->configured_label;
}

inline void climate_layout_card_icon(lv_obj_t *icon_lbl) {
  if (!icon_lbl) return;
  lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, 0, 0);
}

inline void climate_layout_card_sensor(lv_obj_t *sensor_container) {
  if (!sensor_container) return;
  lv_obj_align(sensor_container, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_move_foreground(sensor_container);
}

inline void climate_layout_card_label(lv_obj_t *label_lbl) {
  if (!label_lbl) return;
  lv_obj_align(label_lbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  configure_button_label_wrap(label_lbl);
  lv_obj_move_foreground(label_lbl);
}

inline void climate_update_card(ClimateControlCtx *ctx) {
  if (!ctx) return;
  std::string value = climate_card_value(ctx);
  bool show_icon = ctx->number_display == "icon";
  if (ctx->icon_lbl) {
    if (show_icon) {
      if (ctx->icon_font)
        lv_obj_set_style_text_font(ctx->icon_lbl, ctx->icon_font, LV_PART_MAIN);
      lv_label_set_text(ctx->icon_lbl,
        climate_temperature_controls_enabled(ctx) ? ctx->icon_on_glyph : ctx->icon_off_glyph);
      climate_layout_card_icon(ctx->icon_lbl);
      lv_obj_clear_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(ctx->icon_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (ctx->sensor_container) {
    if (show_icon) lv_obj_add_flag(ctx->sensor_container, LV_OBJ_FLAG_HIDDEN);
    else {
      lv_obj_clear_flag(ctx->sensor_container, LV_OBJ_FLAG_HIDDEN);
      climate_layout_card_sensor(ctx->sensor_container);
    }
  }
  if (!show_icon && ctx->value_lbl) lv_label_set_text(ctx->value_lbl, value.c_str());
  if (!show_icon && ctx->unit_lbl) lv_label_set_text(ctx->unit_lbl, (value.empty() || value == "--") ? "" : display_temperature_unit_symbol());
  if (ctx->label_lbl) {
    lv_label_set_text(ctx->label_lbl, climate_card_label(ctx).c_str());
    climate_layout_card_label(ctx->label_lbl);
  }
  if (ctx->btn) {
    set_card_checked_state(ctx->btn, climate_is_active(ctx));
  }
}

inline void climate_send_action(const std::string &entity_id,
                                const char *service,
                                const std::vector<std::pair<const char *, std::string>> &data) {
  if (entity_id.empty() || service == nullptr || service[0] == '\0') return;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, data.size() + 1)) return;
  ha_action_add_entity(req, entity_id);
  for (const auto &item : data) {
    ha_action_add_data(req, item.first, item.second.c_str());
  }
  ha_action_send(req);
}

inline std::string climate_service_temp_value(int tenths) {
  char buf[16];
  int abs_tenths = std::abs(tenths);
  snprintf(buf, sizeof(buf), "%s%d.%d", tenths < 0 ? "-" : "", abs_tenths / 10, abs_tenths % 10);
  return buf;
}

inline void climate_send_temperature(ClimateControlCtx *ctx) {
  if (!ctx || ctx->entity_id.empty() || !climate_modal_temperature_controls_enabled(ctx)) return;
  ClimateTargetKind kind = climate_target_kind(ctx);
  espcontrol::climate::CommandKind command = espcontrol::climate::command_kind(
    kind, climate_target_values_complete(ctx));
  if (command == espcontrol::climate::CommandKind::RANGE) {
    climate_send_action(ctx->entity_id, "climate.set_temperature", {
      {"target_temp_low", climate_service_temp_value(ctx->low_tenths)},
      {"target_temp_high", climate_service_temp_value(ctx->high_tenths)},
    });
  } else if (command == espcontrol::climate::CommandKind::SINGLE) {
    climate_send_action(ctx->entity_id, "climate.set_temperature", {
      {"temperature", climate_service_temp_value(ctx->target_tenths)},
    });
  } else {
    return;
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

inline void climate_cancel_temperature_send(ClimateControlCtx *ctx) {
  if (!ctx) return;
  ctx->pending_temp_send = false;
  if (ctx->debounce_timer) lv_timer_pause(ctx->debounce_timer);
}

inline void climate_apply_selected_target(ClimateControlCtx *ctx, int value, bool send_now, bool debounce);
inline void climate_control_set_modal_value(ClimateControlCtx *ctx);

inline void climate_update_drag_preview(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  int target = climate_display_target(ctx);
  if (climate_dual_target(ctx)) {
    if (ui.low_target_lbl && ctx->has_low)
      lv_label_set_text(ui.low_target_lbl, climate_format_tenths(
        climate_display_low_target(ctx), climate_target_display_precision(ctx)).c_str());
    if (ui.high_target_lbl && ctx->has_high)
      lv_label_set_text(ui.high_target_lbl, climate_format_tenths(
        climate_display_high_target(ctx), climate_target_display_precision(ctx)).c_str());
    if (ui.heat_arc && ui.cool_arc) {
      uint16_t low_angle = static_cast<uint16_t>(
        climate_arc_angle_for_tenths(ctx, climate_display_low_target(ctx)) + 0.5f);
      uint16_t high_angle = static_cast<uint16_t>(
        climate_arc_angle_for_tenths(ctx, climate_display_high_target(ctx)) + 0.5f);
      lv_arc_set_bg_angles(ui.heat_arc, 135, low_angle);
      lv_arc_set_bg_angles(ui.cool_arc, high_angle, 45);
    }
  } else if (ui.target_lbl) {
    lv_label_set_text(ui.target_lbl, climate_format_tenths(
      target, climate_target_display_precision(ctx)).c_str());
  }
  if (ui.panel) {
    ControlModalLayout layout = climate_control_calc_layout(ctx);
    if (climate_dual_target(ctx)) climate_layout_dual_handle_dots(ctx, layout);
    else climate_layout_handle_dot(ctx, layout);
    climate_raise_arc_markers();
  }
}

inline void climate_apply_selected_target(ClimateControlCtx *ctx, int value, bool send_now, bool debounce) {
  if (!ctx) return;
  if (!climate_modal_temperature_controls_enabled(ctx)) {
    climate_control_set_modal_value(ctx);
    return;
  }
  value = climate_round_to_step(ctx, climate_constrain_selected_target(ctx, value));
  value = climate_constrain_selected_target(ctx, value);
  if (climate_dual_target(ctx)) {
    if (ctx->edit_high) {
      ctx->high_tenths = climate_clamp_tenths(ctx, value);
      ctx->has_high = true;
    } else {
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

inline void climate_preview_selected_target(ClimateControlCtx *ctx, int value) {
  if (!ctx || !climate_modal_temperature_controls_enabled(ctx)) return;
  ClimateControlModalUi &ui = climate_control_modal_ui();
  value = climate_round_to_step(ctx, climate_constrain_selected_target(ctx, value));
  value = climate_constrain_selected_target(ctx, value);
  if (ui.has_drag_preview && ui.drag_preview_tenths == value) return;
  ui.drag_preview_tenths = value;
  ui.has_drag_preview = true;
  climate_update_drag_preview(ctx);
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
  control_modal_delete_nested_overlay(ui.menu_overlay);
}

inline void climate_hide_inline_option_list() {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (ui.option_list_view) lv_obj_del(ui.option_list_view);
  ui.option_list_view = nullptr;
}

inline void climate_update_chip(lv_obj_t *chip, const char *title, const std::string &value,
                                bool visible, bool show_value = true) {
  if (!chip) return;
  if (!visible) {
    lv_obj_add_flag(chip, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *label = lv_obj_get_child(chip, 0);
  if (!label) return;
  std::string text = espcontrol_i18n(title);
  if (show_value) text += " " + (value.empty() ? espcontrol_i18n(std::string("None")) : climate_option_label(value));
  lv_label_set_text(label, text.c_str());
}

inline void climate_update_option_chip(lv_obj_t *chip, const char *title,
                                       const std::string &value, bool visible) {
  if (!chip) return;
  if (!visible) {
    lv_obj_add_flag(chip, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  lv_obj_clear_flag(chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_t *text_col = lv_obj_get_child(chip, 1);
  if (!text_col) return;
  lv_obj_t *title_lbl = lv_obj_get_child(text_col, 0);
  lv_obj_t *value_lbl = lv_obj_get_child(text_col, 1);
  if (title_lbl) lv_label_set_text(title_lbl, espcontrol_i18n(title));
  if (value_lbl) {
    std::string text = value.empty() ? espcontrol_i18n(std::string("None")) : climate_option_label(value);
    lv_label_set_text(value_lbl, text.c_str());
  }
}

inline std::string climate_target_chip_value(ClimateControlCtx *ctx) {
  if (!ctx) return "";
  return ctx->edit_high ? "High" : "Low";
}

inline void climate_update_target_chip(lv_obj_t *chip, ClimateControlCtx *ctx,
                                       bool visible) {
  climate_update_option_chip(chip, "Target", climate_target_chip_value(ctx), visible);
}

inline void climate_send_option(ClimateControlCtx *ctx, const std::string &kind, const std::string &value) {
  if (!ctx || value.empty()) return;
  if (kind == "hvac") {
    std::string service_value = climate_hvac_service_value(value);
    climate_select_target_for_mode(ctx, service_value);
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
  } else if (kind == "target") {
    ctx->edit_high = climate_lower(climate_trim(value)) == "high";
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
  if (kind == "target") return ctx->edit_high ? "high" : "low";
  return "";
}

inline bool climate_option_selected(ClimateControlCtx *ctx,
                                    const std::string &kind,
                                    const std::string &value) {
  if (!ctx) return false;
  if (kind == "hvac") return climate_hvac_service_value(value) == ctx->hvac_mode;
  std::string current = climate_option_current_value(ctx, kind);
  return climate_lower(climate_trim(value)) == climate_lower(climate_trim(current));
}

inline const char *climate_option_icon(const std::string &kind, const std::string &value) {
  std::string mode = kind == "hvac" ? climate_hvac_service_value(value) : climate_lower(value);
  if (kind == "preset") {
    if (mode == "none" || mode == "off") return find_icon("Power");
    if (mode == "home") return find_icon("Home");
    if (mode == "away" || mode == "vacation") return find_icon("Remote Home");
    if (mode == "eco" || mode == "energy" || mode == "energy_saving") return find_icon("Leaf");
    if (mode == "sleep" || mode == "night") return find_icon("Clock");
    if (mode == "boost" || mode == "turbo") return find_icon("Gauge Full");
    if (mode == "comfort") return find_icon("Home-Thermostat");
    if (mode == "activity" || mode == "presence") return find_icon("Motion Sensor");
    return find_icon("Air Filter");
  }
  if (kind == "fan") {
    if (mode == "off") return find_icon("Fan Off");
    if (mode == "auto") return find_icon("Fan Auto");
    if (mode == "low" || mode == "min" || mode == "1") return find_icon("Fan Speed 1");
    if (mode == "medium" || mode == "med" || mode == "2") return find_icon("Fan Speed 2");
    if (mode == "high" || mode == "max" || mode == "3" || mode == "turbo") return find_icon("Fan Speed 3");
    return find_icon("Fan");
  }
  if (kind == "swing") {
    if (mode == "off") return find_icon("Motion Sensor Off");
    if (mode == "vertical" || mode == "up_down") return find_icon("Swap Vertical");
    if (mode == "horizontal" || mode == "left_right") return find_icon("Swap Horizontal");
    if (mode == "both" || mode == "auto") return find_icon("Swap Horizontal");
    return find_icon("Swap Horizontal");
  }
  if (mode == "off") return find_icon("Power");
  if (mode == "heat") return find_icon("Fire");
  if (mode == "cool") return find_icon("Snowflake");
  if (mode == "heat_cool" || mode == "auto") return find_icon("Thermostat Auto");
  if (mode == "dry") return find_icon("Water");
  if (mode == "fan_only" || mode == "fan") return find_icon("Fan");
  return find_icon("Thermostat");
}

inline lv_obj_t *climate_create_option_chip(lv_obj_t *parent, const char *icon,
                                            const char *title,
                                            const lv_font_t *icon_font,
                                            const lv_font_t *title_font,
                                            const lv_font_t *value_font,
                                            int width_compensation_percent) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, compensated_width(240, width_compensation_percent), 94);
  lv_obj_set_flex_grow(btn, 0);
  lv_obj_add_flag(btn, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_set_style_radius(btn, 47, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_left(btn, 26, LV_PART_MAIN);
  lv_obj_set_style_pad_right(btn, 22, LV_PART_MAIN);
  lv_obj_set_style_pad_top(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_column(btn, 24, LV_PART_MAIN);
  lv_obj_set_layout(btn, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(btn, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_START, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  control_modal_apply_pressed_fill(btn);

  lv_obj_t *icon_lbl = lv_label_create(btn);
  lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_label_set_text(icon_lbl, icon);
  lv_obj_set_style_text_color(icon_lbl, lv_color_hex(DARK_TEXT_SOFT), LV_PART_MAIN);
  lv_obj_set_style_text_align(icon_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (icon_font) lv_obj_set_style_text_font(icon_lbl, icon_font, LV_PART_MAIN);

  lv_obj_t *text_col = lv_obj_create(btn);
  lv_obj_add_flag(text_col, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_set_size(text_col, 0, LV_SIZE_CONTENT);
  lv_obj_set_flex_grow(text_col, 1);
  lv_obj_set_style_bg_opa(text_col, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(text_col, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(text_col, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_row(text_col, 2, LV_PART_MAIN);
  lv_obj_set_layout(text_col, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(text_col, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(text_col, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(text_col, LV_FLEX_ALIGN_START, LV_PART_MAIN);
  lv_obj_clear_flag(text_col, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(text_col, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title_lbl = lv_label_create(text_col);
  lv_obj_add_flag(title_lbl, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_set_width(title_lbl, lv_pct(100));
  lv_label_set_text(title_lbl, espcontrol_i18n(title));
  lv_label_set_long_mode(title_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(title_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (title_font) lv_obj_set_style_text_font(title_lbl, title_font, LV_PART_MAIN);

  lv_obj_t *value_lbl = lv_label_create(text_col);
  lv_obj_add_flag(value_lbl, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
  lv_obj_set_width(value_lbl, lv_pct(100));
  lv_label_set_text(value_lbl, espcontrol_i18n("None"));
  lv_label_set_long_mode(value_lbl, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(value_lbl, lv_color_hex(DARK_TEXT_SOFT), LV_PART_MAIN);
  lv_obj_set_style_text_align(value_lbl, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
  if (value_font) lv_obj_set_style_text_font(value_lbl, value_font, LV_PART_MAIN);

  return btn;
}

inline void climate_set_obj_visible(lv_obj_t *obj, bool visible) {
  if (!obj) return;
  if (visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN);
  else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

inline void climate_set_arc_enabled(lv_obj_t *arc, bool enabled) {
  if (!arc) return;
  if (enabled) {
    lv_obj_clear_state(arc, LV_STATE_DISABLED);
    lv_obj_add_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  } else {
    lv_obj_add_state(arc, LV_STATE_DISABLED);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
  }
}

inline void climate_set_dial_controls_visible(bool visible) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  bool show_current = visible && ui.active && ui.active->available && ui.active->has_current;
  bool show_handle = visible && ui.active && climate_modal_temperature_controls_enabled(ui.active);
  bool dual = visible && ui.active && climate_dual_target(ui.active);
  bool show_dual_handles = dual && show_handle;
  bool show_step_buttons = visible && !dual;
  climate_set_obj_visible(ui.arc, visible);
  climate_set_obj_visible(ui.heat_arc, dual && show_handle);
  climate_set_obj_visible(ui.cool_arc, dual && show_handle);
  climate_set_obj_visible(ui.current_dot, show_current);
  climate_set_obj_visible(ui.handle_dot, show_handle && !dual);
  climate_set_obj_visible(ui.low_handle_dot, show_dual_handles);
  climate_set_obj_visible(ui.high_handle_dot, show_dual_handles);
  climate_set_obj_visible(ui.target_row, visible);
  climate_set_obj_visible(ui.status_lbl, visible);
  climate_set_obj_visible(ui.minus_btn, show_step_buttons);
  climate_set_obj_visible(ui.plus_btn, show_step_buttons);
  climate_set_obj_visible(ui.target_chip, false);
  climate_set_obj_visible(ui.chips, false);
}

inline void climate_set_step_button_enabled(lv_obj_t *btn, bool enabled) {
  if (!btn) return;
  lv_style_selector_t disabled_selector =
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DISABLED);
  lv_obj_set_style_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, lv_color_hex(SECONDARY_GREY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, disabled_selector);
  lv_obj_set_style_opa(btn, LV_OPA_COVER, disabled_selector);
  lv_obj_set_style_border_color(btn, lv_color_hex(DARK_CONTROL_NEUTRAL), LV_PART_MAIN);
  lv_obj_set_style_border_color(btn, lv_color_hex(DARK_TRACK_BACKGROUND), disabled_selector);

  lv_obj_t *label = lv_obj_get_child(btn, 0);
  if (label) {
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TRACK_BACKGROUND), disabled_selector);
  }

  if (enabled) {
    lv_obj_clear_state(btn, LV_STATE_DISABLED);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    if (label) lv_obj_clear_state(label, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(btn, LV_STATE_DISABLED);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    if (label) lv_obj_add_state(label, LV_STATE_DISABLED);
  }
}

inline void climate_control_style_tab(lv_obj_t *btn, bool active, uint32_t accent_color) {
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

inline void climate_control_layout_modal(ClimateControlCtx *ctx);
inline void climate_control_apply_tab_visibility();

inline void climate_center_tab_icon(lv_obj_t *label) {
  control_modal_center_tab_icon(label);
}

inline lv_obj_t *climate_control_create_tab_button(lv_obj_t *parent, const char *icon,
                                                   const lv_font_t *font,
                                                   ClimateControlTab tab,
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
    climate_center_tab_icon(label);
  }
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    ClimateControlTab tab = static_cast<ClimateControlTab>(
      reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    ClimateControlModalUi &ui = climate_control_modal_ui();
    ui.tab = tab;
    climate_control_apply_tab_visibility();
    climate_control_layout_modal(ui.active);
  }, LV_EVENT_CLICKED, reinterpret_cast<void *>(static_cast<uintptr_t>(tab)));
  return btn;
}

inline lv_obj_t *climate_create_menu_tile(lv_obj_t *parent, const char *icon,
                                          const char *title,
                                          const lv_font_t *icon_font,
                                          const lv_font_t *title_font,
                                          const lv_font_t *value_font,
                                          int width_compensation_percent) {
  return climate_create_option_chip(parent, icon, title, icon_font, title_font,
    value_font, width_compensation_percent);
}

inline void climate_update_menu_tile(lv_obj_t *btn, const char *title,
                                     const std::string &value, bool visible) {
  climate_update_option_chip(btn, title, value, visible);
}

inline void climate_hide_action_menu();
inline void climate_control_layout_modal(ClimateControlCtx *ctx);

inline void climate_open_inline_option_list(ClimateControlCtx *ctx, const std::string &kind) {
  if (!ctx) return;
  ClimateControlModalUi &ui = climate_control_modal_ui();
  ControlModalLayout layout = climate_control_calc_layout(ctx);
  bool compact_portrait_layout = climate_control_uses_compact_portrait_modal_tuning(layout);
  const std::vector<std::string> *options = nullptr;
  const char *title = nullptr;
  if (kind == "hvac") {
    options = &ctx->hvac_modes;
    title = "Mode";
  } else if (kind == "preset") {
    options = &ctx->preset_modes;
    title = "Preset";
  } else if (kind == "fan") {
    options = &ctx->fan_modes;
    title = "Fan";
  } else if (kind == "swing") {
    options = &ctx->swing_modes;
    title = "Swing";
  }
  if (!ui.panel || !options || options->empty()) return;

  climate_hide_inline_option_list();
  ui.option_click_count = 0;

  ui.option_list_view = lv_obj_create(ui.panel);
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
  lv_obj_set_style_flex_flow(ui.option_list_view, LV_FLEX_FLOW_ROW_WRAP, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.option_list_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.option_list_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_add_flag(ui.option_list_view, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(ui.option_list_view, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ui.option_list_view, LV_SCROLLBAR_MODE_OFF);

  auto add_section = [&](lv_obj_t *parent,
                         const char *section_title,
                         const std::string &section_kind,
                         const std::vector<std::string> &section_options) {
    if (section_options.empty()) return;
    (void) section_title;

    for (const auto &option : section_options) {
      ClimateOptionClick *click = climate_next_option_click(ui, ctx, section_kind, option);
      if (!click) break;
      bool selected = climate_option_selected(ctx, section_kind, option);
      uint32_t bg_color = selected ? ctx->accent_color : ctx->secondary_color;
      uint32_t text_color = selected ? DARK_TEXT_PRIMARY : readable_text_color_for_bg(bg_color);
      lv_obj_t *btn = lv_btn_create(parent);
      lv_obj_set_size(btn, 118, 118);
      lv_obj_set_style_radius(btn, control_modal_card_radius(ctx->btn), LV_PART_MAIN);
      lv_obj_set_style_bg_color(btn, lv_color_hex(bg_color), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
      lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
      lv_obj_set_style_pad_top(btn, compact_portrait_layout ? 0 : 12, LV_PART_MAIN);
      lv_obj_set_style_pad_bottom(btn, compact_portrait_layout ? 0 : 12, LV_PART_MAIN);
      lv_obj_set_style_pad_left(btn, compact_portrait_layout ? 16 : 14, LV_PART_MAIN);
      lv_obj_set_style_pad_right(btn, compact_portrait_layout ? 16 : 14, LV_PART_MAIN);
      lv_obj_set_style_pad_row(btn, compact_portrait_layout ? 0 : 8, LV_PART_MAIN);
      lv_obj_set_style_pad_column(btn, 0, LV_PART_MAIN);
      lv_obj_set_layout(btn, compact_portrait_layout ? LV_LAYOUT_NONE : LV_LAYOUT_FLEX);
      lv_obj_set_style_flex_flow(btn,
        compact_portrait_layout ? LV_FLEX_FLOW_ROW : LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
      lv_obj_set_style_flex_main_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      lv_obj_set_style_flex_cross_place(btn, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      control_modal_apply_pressed_fill(btn);

      lv_obj_t *content_parent = btn;
      if (compact_portrait_layout) {
        content_parent = lv_obj_create(btn);
        lv_obj_set_size(content_parent, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(content_parent, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(content_parent, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(content_parent, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(content_parent, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(content_parent, 2, LV_PART_MAIN);
        lv_obj_set_style_pad_column(content_parent, 0, LV_PART_MAIN);
        lv_obj_set_layout(content_parent, LV_LAYOUT_FLEX);
        lv_obj_set_style_flex_flow(content_parent, LV_FLEX_FLOW_COLUMN, LV_PART_MAIN);
        lv_obj_set_style_flex_main_place(content_parent, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_flex_cross_place(content_parent, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_clear_flag(content_parent, LV_OBJ_FLAG_SCROLLABLE);
      }

      lv_obj_t *icon_lbl = lv_label_create(content_parent);
      lv_label_set_text(icon_lbl, climate_option_icon(section_kind, option));
      lv_obj_set_style_text_color(icon_lbl, lv_color_hex(text_color), LV_PART_MAIN);
      lv_obj_set_style_text_align(icon_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (compact_portrait_layout && ctx->card_icon_font) {
        lv_obj_set_style_text_font(icon_lbl, ctx->card_icon_font, LV_PART_MAIN);
      } else if (ctx->icon_font) {
        lv_obj_set_style_text_font(icon_lbl, ctx->icon_font, LV_PART_MAIN);
      }
      if (compact_portrait_layout && !ctx->card_icon_font) lv_obj_set_style_transform_zoom(
        icon_lbl, CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_ICON_ZOOM, LV_PART_MAIN);

      lv_obj_t *label = lv_label_create(content_parent);
      lv_label_set_text(label, climate_option_label(option).c_str());
      lv_label_set_long_mode(label, compact_portrait_layout ? LV_LABEL_LONG_CLIP : LV_LABEL_LONG_WRAP);
      lv_obj_set_width(label, compact_portrait_layout ? LV_SIZE_CONTENT : lv_pct(100));
      lv_obj_set_style_text_color(label, lv_color_hex(text_color), LV_PART_MAIN);
      lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
      if (ctx->option_menu_font) lv_obj_set_style_text_font(label, ctx->option_menu_font, LV_PART_MAIN);
      if (compact_portrait_layout) lv_obj_center(content_parent);

      lv_obj_add_event_cb(btn, [](lv_event_t *e) {
        ClimateOptionClick *click = (ClimateOptionClick *)lv_event_get_user_data(e);
        if (click) climate_send_option(click->ctx, click->kind, click->value);
        if (click) climate_open_inline_option_list(click->ctx, click->kind);
      }, LV_EVENT_CLICKED, click);
    }
  };

  add_section(ui.option_list_view, title, kind, *options);
  lv_obj_move_foreground(ui.option_list_view);
  climate_control_layout_modal(ctx);
}

inline const char *climate_control_tab_kind(ClimateControlTab tab) {
  switch (tab) {
    case ClimateControlTab::MODE: return "hvac";
    case ClimateControlTab::PRESET: return "preset";
    case ClimateControlTab::FAN: return "fan";
    case ClimateControlTab::SWING: return "swing";
    case ClimateControlTab::TEMPERATURE: return "";
  }
  return "";
}

inline void climate_control_apply_tab_visibility() {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  ClimateControlCtx *ctx = ui.active;
  if (!ctx) return;
  climate_control_ensure_visible_tab(ctx);
  ClimateControlVisibleTabs visible_tabs = climate_control_visible_tabs(ctx);
  bool show_tab_bar = visible_tabs.count > 1;
  bool show_temperature = ui.tab == ClimateControlTab::TEMPERATURE;
  bool show_mode = ui.tab == ClimateControlTab::MODE;
  bool show_preset = ui.tab == ClimateControlTab::PRESET;
  bool show_fan = ui.tab == ClimateControlTab::FAN;
  bool show_swing = ui.tab == ClimateControlTab::SWING;

  climate_set_obj_visible(ui.tab_row, show_tab_bar);
  climate_set_obj_visible(ui.temperature_tab,
    show_tab_bar && visible_tabs.contains(ClimateControlTab::TEMPERATURE));
  climate_set_obj_visible(ui.mode_tab,
    show_tab_bar && visible_tabs.contains(ClimateControlTab::MODE));
  climate_set_obj_visible(ui.preset_tab,
    show_tab_bar && visible_tabs.contains(ClimateControlTab::PRESET));
  climate_set_obj_visible(ui.fan_tab,
    show_tab_bar && visible_tabs.contains(ClimateControlTab::FAN));
  climate_set_obj_visible(ui.swing_tab,
    show_tab_bar && visible_tabs.contains(ClimateControlTab::SWING));

  climate_control_style_tab(ui.temperature_tab, show_temperature, ctx->accent_color);
  climate_control_style_tab(ui.mode_tab, show_mode, ctx->accent_color);
  climate_control_style_tab(ui.preset_tab, show_preset, ctx->accent_color);
  climate_control_style_tab(ui.fan_tab, show_fan, ctx->accent_color);
  climate_control_style_tab(ui.swing_tab, show_swing, ctx->accent_color);

  climate_set_dial_controls_visible(show_temperature);
  if (show_temperature) {
    climate_hide_inline_option_list();
  } else {
    climate_open_inline_option_list(ctx, climate_control_tab_kind(ui.tab));
  }
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
  std::vector<std::string> target_options;
  if (kind == "hvac") options = &ctx->hvac_modes;
  else if (kind == "fan") options = &ctx->fan_modes;
  else if (kind == "swing") options = &ctx->swing_modes;
  else if (kind == "preset") options = &ctx->preset_modes;
  else if (kind == "target") {
    target_options = {"low", "high"};
    options = &target_options;
  }
  if (!options || options->empty()) return;

  ui.option_click_count = 0;
  ControlModalNestedShell shell = control_modal_open_nested_menu(
    climate_option_menu_width(*options, kind, ctx->option_menu_font), 14, climate_hide_option_menu);
  ui.menu_overlay = shell.overlay;
  lv_obj_t *box = shell.panel;
  lv_obj_set_style_pad_row(box, CLIMATE_OPTION_MENU_ROW_GAP_PX, LV_PART_MAIN);

  lv_coord_t option_h = ctx->option_menu_font
    ? ctx->option_menu_font->line_height + CLIMATE_OPTION_MENU_ROW_PAD_Y_PX * 2
    : CLIMATE_OPTION_MENU_ROW_MIN_H_PX;
  if (option_h < CLIMATE_OPTION_MENU_ROW_MIN_H_PX)
    option_h = CLIMATE_OPTION_MENU_ROW_MIN_H_PX;

  for (const auto &option : *options) {
    ClimateOptionClick *click = climate_next_option_click(ui, ctx, kind, option);
    if (!click) break;
    lv_obj_t *btn = lv_btn_create(box);
    lv_obj_set_width(btn, lv_pct(100));
    lv_obj_set_height(btn, option_h);
    lv_obj_set_style_radius(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_top(btn, CLIMATE_OPTION_MENU_ROW_PAD_Y_PX, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(btn, CLIMATE_OPTION_MENU_ROW_PAD_Y_PX, LV_PART_MAIN);
    lv_obj_set_style_pad_left(btn, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_right(btn, 12, LV_PART_MAIN);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, climate_option_label(option).c_str());
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    if (ctx->option_menu_font) lv_obj_set_style_text_font(label, ctx->option_menu_font, LV_PART_MAIN);
    lv_obj_center(label);
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      ClimateOptionClick *click = (ClimateOptionClick *)lv_event_get_user_data(e);
      if (click) climate_send_option(click->ctx, click->kind, click->value);
      climate_hide_option_menu();
    }, LV_EVENT_CLICKED, click);
  }
  lv_obj_move_foreground(ui.menu_overlay);
}

inline void climate_control_set_modal_value(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || ui.active != ctx) return;
  bool temp_enabled = climate_modal_temperature_controls_enabled(ctx);
  bool show_dial = climate_temperature_target_available(ctx);
  bool dual = climate_dual_target(ctx);
  int target = climate_display_target(ctx);
  if (ui.arc) {
    climate_set_obj_visible(ui.arc, show_dial);
    if (show_dial && !ui.dragging_arc) {
      ui.updating_arc = true;
      lv_arc_set_range(ui.arc, ctx->min_tenths, ctx->max_tenths);
      lv_arc_set_mode(ui.arc, climate_uses_cooling_arc(ctx) ? LV_ARC_MODE_REVERSE : LV_ARC_MODE_NORMAL);
      lv_arc_set_value(ui.arc, climate_modal_arc_value(ctx, temp_enabled, target));
      lv_obj_set_style_arc_color(ui.arc, lv_color_hex(
        dual ? DARK_TRACK_BACKGROUND : climate_modal_arc_color(ctx)), LV_PART_INDICATOR);
      ui.updating_arc = false;
    }
    climate_set_arc_enabled(ui.arc, temp_enabled);
  }
  bool show_dual_arcs = show_dial && dual && temp_enabled;
  climate_set_obj_visible(ui.heat_arc, show_dual_arcs);
  climate_set_obj_visible(ui.cool_arc, show_dual_arcs);
  if (show_dual_arcs) {
    uint16_t low_angle = static_cast<uint16_t>(
      climate_arc_angle_for_tenths(ctx, climate_display_low_target(ctx)) + 0.5f);
    uint16_t high_angle = static_cast<uint16_t>(
      climate_arc_angle_for_tenths(ctx, climate_display_high_target(ctx)) + 0.5f);
    lv_arc_set_bg_angles(ui.heat_arc, 135, low_angle);
    lv_arc_set_bg_angles(ui.cool_arc, high_angle, 45);
  }
  if (ui.current_dot) {
    bool show_current = show_dial && ctx->has_current;
    climate_set_obj_visible(ui.current_dot, show_current);
    if (show_current && ui.panel) climate_layout_current_dot(ctx, climate_control_calc_layout(ctx));
  }
  if (ui.handle_dot) {
    climate_set_obj_visible(ui.handle_dot, temp_enabled && !dual);
    if (temp_enabled && !dual && ui.panel)
      climate_layout_handle_dot(ctx, climate_control_calc_layout(ctx));
  }
  bool show_dual_handles = temp_enabled && dual;
  climate_set_obj_visible(ui.low_handle_dot, show_dual_handles);
  climate_set_obj_visible(ui.high_handle_dot, show_dual_handles);
  if (show_dual_handles && ui.panel) {
    climate_layout_dual_handle_dots(ctx, climate_control_calc_layout(ctx));
    climate_style_dual_handle(ui.low_handle_dot, !ctx->edit_high);
    climate_style_dual_handle(ui.high_handle_dot, ctx->edit_high);
  }
  climate_raise_arc_markers();
  if (ui.target_row) climate_set_obj_visible(ui.target_row, true);
  if (ui.target_lbl) {
    climate_set_obj_visible(ui.target_lbl, !dual);
    if (!ctx->available || !ctx->has_target) lv_label_set_text(ui.target_lbl, "--");
    else lv_label_set_text(ui.target_lbl, climate_format_tenths(
      target, climate_target_display_precision(ctx)).c_str());
    lv_obj_clear_flag(ui.target_lbl, LV_OBJ_FLAG_CLICKABLE);
  }
  climate_set_obj_visible(ui.low_target_lbl, dual);
  climate_set_obj_visible(ui.target_separator_lbl, dual);
  climate_set_obj_visible(ui.high_target_lbl, dual);
  if (ui.low_target_lbl) {
    lv_label_set_text(ui.low_target_lbl, ctx->has_low
      ? climate_format_tenths(climate_display_low_target(ctx),
          climate_target_display_precision(ctx)).c_str()
      : "--");
    lv_obj_set_style_text_color(ui.low_target_lbl,
                                lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_opa(ui.low_target_lbl, LV_OPA_COVER, LV_PART_MAIN);
  }
  if (ui.high_target_lbl) {
    lv_label_set_text(ui.high_target_lbl, ctx->has_high
      ? climate_format_tenths(climate_display_high_target(ctx),
          climate_target_display_precision(ctx)).c_str()
      : "--");
    lv_obj_set_style_text_color(ui.high_target_lbl,
                                lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_opa(ui.high_target_lbl, LV_OPA_COVER, LV_PART_MAIN);
  }
  if (ui.unit_lbl) {
    lv_label_set_text(ui.unit_lbl, show_dial ? display_temperature_unit_symbol() : "");
    climate_set_obj_visible(ui.unit_lbl, show_dial);
  }
  if (ui.status_lbl) {
    lv_label_set_text(ui.status_lbl, climate_action_label(ctx).c_str());
  }
  climate_update_target_chip(ui.target_chip, ctx, false);
  climate_update_option_chip(ui.mode_chip, "Mode", ctx->hvac_mode, false);
  climate_update_option_chip(ui.preset_chip, "Preset", ctx->preset_mode, false);
  climate_update_option_chip(ui.fan_chip, "Fan", ctx->fan_mode, false);
  climate_update_option_chip(ui.swing_chip, "Swing", ctx->swing_mode, false);
  climate_set_obj_visible(ui.chips, false);
  climate_set_step_button_enabled(ui.minus_btn, temp_enabled && !dual);
  climate_set_step_button_enabled(ui.plus_btn, temp_enabled && !dual);
  climate_control_apply_tab_visibility();
}

inline void climate_control_layout_modal(ClimateControlCtx *ctx) {
  ClimateControlModalUi &ui = climate_control_modal_ui();
  if (!ctx || !ui.overlay || !ui.panel) return;
  ControlModalLayout layout = climate_control_calc_layout(ctx);
  bool compact_control = climate_control_uses_compact_control_modal_tuning(layout);
  bool compact_portrait = climate_control_uses_compact_portrait_modal_tuning(layout);
  if (ui.target_lbl) {
    lv_obj_set_style_transform_zoom(ui.target_lbl, climate_control_target_zoom(layout), LV_PART_MAIN);
  }
  if (ui.low_target_lbl)
    lv_obj_set_style_transform_zoom(ui.low_target_lbl, climate_control_target_zoom(layout), LV_PART_MAIN);
  if (ui.high_target_lbl)
    lv_obj_set_style_transform_zoom(ui.high_target_lbl, climate_control_target_zoom(layout), LV_PART_MAIN);
  if (ui.status_lbl) lv_obj_update_layout(ui.status_lbl);
  if (ui.target_row) lv_obj_update_layout(ui.target_row);
  lv_coord_t title_h = ui.status_lbl ? lv_obj_get_height(ui.status_lbl) : 0;
  lv_coord_t value_h = ui.target_row ? lv_obj_get_height(ui.target_row) : 0;
  lv_coord_t labels_down_ref = climate_control_labels_down_ref(layout);
  lv_coord_t value_center_y = layout.value_center_y -
    control_modal_scaled_px(22, layout.short_side) +
    control_modal_scaled_px(labels_down_ref, layout.short_side);
  lv_coord_t step_buttons_up_ref = climate_control_step_buttons_up_ref(layout);
  lv_coord_t controls_center_y = layout.controls_center_y -
    control_modal_scaled_px(step_buttons_up_ref, layout.short_side);
  lv_coord_t step_buttons_center_y = controls_center_y +
    control_modal_scaled_px(climate_control_step_buttons_down_ref(layout), layout.short_side);
  lv_coord_t title_center_y = value_center_y -
    (value_h / 2 + layout.title_gap + title_h / 2);
  bool roomy_landscape = layout.panel_w >= 900 && layout.panel_h <= 600;
  bool medium_landscape = layout.panel_w >= 760 && layout.panel_h <= 520;
  lv_coord_t chip_h = climate_option_chip_height(ctx, layout);
  lv_coord_t chip_gap = compact_portrait
    ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_GAP_PX
    : control_modal_scaled_px(compact_control ? CLIMATE_MODAL_COMPACT_CONTROL_OPTION_CHIP_GAP_REF_PX : 24,
        layout.short_side);
  if (!compact_portrait && chip_gap < (compact_control ? 10 : 16)) chip_gap = compact_control ? 10 : 16;

  control_modal_apply_panel_layout(ui.overlay, ui.panel, layout,
    control_modal_card_radius(ctx->btn));
  control_modal_apply_back_button_layout(ui.back_btn, layout);
  ClimateControlVisibleTabs visible_tabs = climate_control_visible_tabs(ctx);
  ControlModalTabLayout tabs_layout = climate_control_calc_tab_layout(ctx, layout);
  int tab_count = tabs_layout.tab_count;
  bool show_tab_bar = tabs_layout.show_tab_bar;
  lv_coord_t tab_frame_h = tabs_layout.tab_frame_h;
  control_modal_apply_tab_row(ui.tab_row, layout, tabs_layout);
  for (int i = 0; show_tab_bar && i < tab_count; i++) {
    lv_obj_t *tab_btn = climate_control_tab_button(ui, visible_tabs.tabs[i]);
    if (!tab_btn) continue;
    bool active = visible_tabs.tabs[i] == ui.tab;
    control_modal_layout_tab_button(tab_btn, layout, tabs_layout, i, active);
  }
  if (ui.mode_btn) {
    lv_obj_set_size(ui.mode_btn, layout.back_size, layout.back_size);
    lv_obj_set_style_radius(ui.mode_btn, layout.back_size / 2, LV_PART_MAIN);
    lv_obj_align(ui.mode_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  }
  if (ui.menu_close_btn) {
    lv_obj_set_size(ui.menu_close_btn, layout.back_size, layout.back_size);
    lv_obj_set_style_radius(ui.menu_close_btn, layout.back_size / 2, LV_PART_MAIN);
    lv_obj_align(ui.menu_close_btn, LV_ALIGN_TOP_RIGHT, -layout.inset, layout.inset);
  }
  control_modal_apply_arc_layout(ui.arc, layout, ctx->width_compensation_percent);
  climate_apply_background_arc_width(ui.arc, layout);
  control_modal_apply_arc_layout(ui.heat_arc, layout, ctx->width_compensation_percent);
  control_modal_apply_arc_layout(ui.cool_arc, layout, ctx->width_compensation_percent);
  if (ui.heat_arc)
    {
      lv_obj_set_style_arc_width(ui.heat_arc, layout.arc_stroke, LV_PART_MAIN);
      lv_obj_set_style_arc_width(ui.heat_arc, 0, LV_PART_INDICATOR);
    }
  if (ui.cool_arc)
    {
      lv_obj_set_style_arc_width(ui.cool_arc, layout.arc_stroke, LV_PART_MAIN);
      lv_obj_set_style_arc_width(ui.cool_arc, 0, LV_PART_INDICATOR);
    }
  if (ui.current_dot) climate_layout_current_dot(ctx, layout);
  if (climate_dual_target(ctx)) climate_layout_dual_handle_dots(ctx, layout);
  else if (ui.handle_dot) climate_layout_handle_dot(ctx, layout);
  climate_raise_arc_markers();
  lv_obj_align(ui.status_lbl, LV_ALIGN_CENTER, 0, title_center_y);
  lv_obj_set_style_translate_y(ui.status_lbl, climate_control_status_translate_y(layout), LV_PART_MAIN);
  lv_obj_align(ui.target_row, LV_ALIGN_CENTER, 0, value_center_y);
  ControlModalLayout controls_layout = layout;
  controls_layout.btn_size = control_modal_scaled_px(
    compact_portrait ? CLIMATE_MODAL_COMPACT_PORTRAIT_STEP_BUTTON_REF_PX : 64,
    layout.short_side);
  if (controls_layout.btn_size < 48) controls_layout.btn_size = 48;
  controls_layout.controls_gap = control_modal_scaled_px(CLIMATE_MODAL_STEP_BUTTON_GAP_REF_PX, layout.short_side);
  if (controls_layout.controls_gap < 6) controls_layout.controls_gap = 6;
  controls_layout.controls_center_y = step_buttons_center_y;
  control_modal_apply_step_buttons_layout(ui.minus_btn, ui.plus_btn, controls_layout);
  lv_obj_set_width(ui.chips, lv_pct(CLIMATE_OPTION_ROW_WIDTH_PERCENT));
  lv_obj_set_height(ui.chips, chip_h);
  lv_obj_set_style_pad_column(ui.chips, chip_gap, LV_PART_MAIN);
  lv_obj_add_flag(ui.chips, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(ui.chips, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(ui.chips, LV_SCROLLBAR_MODE_OFF);
  uint8_t visible_chip_count = 0;
  lv_coord_t chip_row_w = layout.panel_w * CLIMATE_OPTION_ROW_WIDTH_PERCENT / 100;
  bool large_square = climate_control_uses_large_square_modal_tuning(layout);
  lv_coord_t option_chip_w = compensated_width(
    compact_control ? CLIMATE_MODAL_COMPACT_CONTROL_OPTION_CHIP_W_REF_PX :
      (large_square ? CLIMATE_MODAL_LARGE_SQUARE_OPTION_CHIP_W_REF_PX :
        (compact_portrait ? CLIMATE_MODAL_COMPACT_PORTRAIT_OPTION_CHIP_W_PX :
          (layout.short_side < 520 ? (roomy_landscape ? 224 : (medium_landscape ? 240 : 180)) : 240))),
    ctx->width_compensation_percent);
  if (compact_portrait && visible_chip_count == 2) {
    lv_coord_t fitted_w = (chip_row_w - chip_gap) / 2;
    if (fitted_w > option_chip_w) option_chip_w = fitted_w;
  }
  if (climate_control_uses_wide_landscape_option_fit(layout) && visible_chip_count > 1) {
    lv_coord_t fitted_w = (chip_row_w - chip_gap * (visible_chip_count - 1)) / visible_chip_count;
    lv_coord_t min_chip_w = climate_control_uses_wide_landscape_modal_tuning(layout) ? 132 : 148;
    if (fitted_w >= min_chip_w && fitted_w < option_chip_w) option_chip_w = fitted_w;
  }
  auto layout_option_chip = [&](lv_obj_t *chip) {
    if (!chip) return;
    lv_obj_set_size(chip, option_chip_w, chip_h);
    lv_obj_set_style_radius(chip, chip_h / 2, LV_PART_MAIN);
    climate_apply_bottom_chip_padding(chip, layout);
  };
  layout_option_chip(ui.target_chip);
  layout_option_chip(ui.mode_chip);
  layout_option_chip(ui.preset_chip);
  layout_option_chip(ui.fan_chip);
  layout_option_chip(ui.swing_chip);
  lv_coord_t chip_content_w = visible_chip_count == 0 ? 0 :
    visible_chip_count * option_chip_w + (visible_chip_count - 1) * chip_gap;
  if (chip_content_w > chip_row_w) {
    lv_obj_set_style_flex_main_place(ui.chips, LV_FLEX_ALIGN_START, LV_PART_MAIN);
  } else {
    lv_obj_scroll_to_x(ui.chips, 0, LV_ANIM_OFF);
    lv_obj_set_style_flex_main_place(ui.chips, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  }
  lv_coord_t chip_bottom = climate_control_uses_wide_landscape_modal_tuning(layout)
    ? CLIMATE_MODAL_WIDE_LANDSCAPE_OPTION_CHIP_BOTTOM_PX
    : (climate_control_uses_large_landscape_modal_tuning(layout)
      ? CLIMATE_MODAL_LARGE_LANDSCAPE_OPTION_CHIP_BOTTOM_PX
      : (roomy_landscape ? CLIMATE_MODAL_ROOMY_LANDSCAPE_OPTION_CHIP_BOTTOM_PX : layout.inset));
  lv_obj_align(ui.chips, LV_ALIGN_BOTTOM_MID, 0, -chip_bottom);
  if (ui.menu_view) {
    lv_obj_set_size(ui.menu_view, layout.panel_w, layout.panel_h);
    lv_obj_set_pos(ui.menu_view, 0, 0);
    lv_coord_t menu_gap = control_modal_scaled_px(16, layout.short_side);
    lv_coord_t tile_w = layout.panel_w - layout.inset * 2;
    if (tile_w > 280) tile_w = 280;
    lv_coord_t tile_h = layout.short_side < 520 ? 86 : 94;
    lv_coord_t visual_tile_w = compensated_width(tile_w, ctx->width_compensation_percent);
    lv_obj_set_size(ui.menu_mode_btn, visual_tile_w, tile_h);
    lv_obj_set_size(ui.menu_preset_btn, visual_tile_w, tile_h);
    lv_obj_set_style_radius(ui.menu_mode_btn, tile_h / 2, LV_PART_MAIN);
    lv_obj_set_style_radius(ui.menu_preset_btn, tile_h / 2, LV_PART_MAIN);
    lv_obj_align(ui.menu_mode_btn, LV_ALIGN_CENTER, 0, -(tile_h + menu_gap) / 2);
    lv_obj_align(ui.menu_preset_btn, LV_ALIGN_CENTER, 0, (tile_h + menu_gap) / 2);
    if (ui.option_list_view) {
      lv_coord_t list_top = show_tab_bar
        ? layout.inset + tab_frame_h + climate_control_tab_content_gap(layout)
        : layout.inset + layout.back_size + 8;
      lv_coord_t list_bottom = layout.inset;
      lv_obj_set_size(ui.option_list_view, layout.panel_w, layout.panel_h);
      lv_obj_set_style_pad_top(ui.option_list_view, list_top, LV_PART_MAIN);
      lv_obj_set_style_pad_left(ui.option_list_view, layout.inset, LV_PART_MAIN);
      lv_obj_set_style_pad_right(ui.option_list_view, layout.inset, LV_PART_MAIN);
      lv_obj_set_style_pad_bottom(ui.option_list_view, list_bottom, LV_PART_MAIN);
      lv_coord_t tile_gap = control_modal_scaled_px(layout.short_side < 520 ? 10 : 12, layout.short_side);
      if (tile_gap < 8) tile_gap = 8;
      lv_coord_t content_w = layout.panel_w - layout.inset * 2;
      lv_coord_t tile_min_w = compensated_width(layout.short_side < 520 ? 138 : 168,
        ctx->width_compensation_percent);
      if (tile_min_w < 118) tile_min_w = 118;
      int column_count = content_w >= tile_min_w * 3 + tile_gap * 2 ? 3 : 2;
      if (content_w < tile_min_w * 2 + tile_gap) column_count = 1;
      lv_coord_t tile_w = (content_w - tile_gap * (column_count - 1)) / column_count;
      lv_obj_set_style_pad_row(ui.option_list_view, tile_gap, LV_PART_MAIN);
      lv_obj_set_style_pad_column(ui.option_list_view, tile_gap, LV_PART_MAIN);
      lv_obj_set_style_flex_main_place(ui.option_list_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
      lv_obj_set_style_flex_cross_place(ui.option_list_view, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);

      uint32_t child_count = lv_obj_get_child_count(ui.option_list_view);
      if (climate_control_uses_wide_landscape_option_fit(layout)) {
        lv_coord_t max_tile_w = climate_control_wide_option_tile_max(layout);
        lv_coord_t min_tile_w = climate_control_wide_option_tile_min(layout);
        int max_columns = climate_control_uses_wide_landscape_modal_tuning(layout) ? 5 : 6;
        int fitted_columns = (content_w + tile_gap) / (max_tile_w + tile_gap);
        if (fitted_columns < 1) fitted_columns = 1;
        if (fitted_columns > max_columns) fitted_columns = max_columns;
        if (child_count > 0 && fitted_columns > static_cast<int>(child_count))
          fitted_columns = static_cast<int>(child_count);
        lv_coord_t fitted_tile_w = (content_w - tile_gap * (fitted_columns - 1)) / fitted_columns;
        tile_w = fitted_tile_w < max_tile_w ? fitted_tile_w : max_tile_w;
        if (tile_w < min_tile_w) tile_w = min_tile_w;
      }
      for (uint32_t i = 0; i < child_count; i++) {
        lv_obj_t *tile = lv_obj_get_child(ui.option_list_view, i);
        if (!lv_obj_has_flag(tile, LV_OBJ_FLAG_CLICKABLE)) continue;
        lv_obj_set_size(tile, tile_w, climate_control_option_tile_height(layout, tile_w));
        lv_obj_set_style_radius(tile, control_modal_card_radius(ctx->btn), LV_PART_MAIN);
        if (climate_control_uses_compact_portrait_modal_tuning(layout)) {
          lv_obj_t *content = lv_obj_get_child(tile, 0);
          if (content) lv_obj_center(content);
        } else {
          lv_obj_t *label = lv_obj_get_child(tile, 1);
          if (label) lv_obj_set_width(label, lv_pct(100));
        }
      }
    }
  }
  if (ui.option_list_view) lv_obj_move_foreground(ui.option_list_view);
  if (ui.tab_row) lv_obj_move_foreground(ui.tab_row);
  lv_obj_move_foreground(ui.back_btn);
  if (ui.mode_btn) lv_obj_move_foreground(ui.mode_btn);
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
  control_modal_delete_overlay(ControlModalKind::CLIMATE, ui.overlay);
  ui = ClimateControlModalUi();
}

inline void delete_climate_control_context(ClimateControlCtx *ctx) {
  if (!ctx) return;
  if (climate_control_modal_ui().active == ctx) climate_control_hide_modal();
  if (ctx->debounce_timer) {
    lv_timer_del(ctx->debounce_timer);
    ctx->debounce_timer = nullptr;
  }
  ClimateControlCtx **refs = climate_control_refs();
  int &count = climate_control_ref_count();
  int write_index = 0;
  for (int read_index = 0; read_index < count; read_index++) {
    if (refs[read_index] == ctx) continue;
    refs[write_index++] = refs[read_index];
  }
  count = write_index;
  delete ctx;
}

inline void climate_control_open_modal(ClimateControlCtx *ctx) {
  if (!ctx || !ctx->available) return;
  ControlModalShell shell = control_modal_open_shell(
    ControlModalKind::CLIMATE, ctx->btn, ctx->width_compensation_percent,
    ctx->icon_font, climate_control_hide_modal);
  ClimateControlModalUi &ui = climate_control_modal_ui();
  ui.active = ctx;
  ui.overlay = shell.overlay;
  ui.panel = shell.panel;
  ui.back_btn = shell.close_btn;
  ui.tab = climate_control_first_visible_tab(ctx);

  ui.tab_row = control_modal_create_tab_row(ui.panel);
  ui.temperature_tab = climate_control_create_tab_button(
    ui.tab_row, find_icon("Thermometer"), ctx->icon_font,
    ClimateControlTab::TEMPERATURE, ctx->width_compensation_percent);
  ui.mode_tab = climate_control_create_tab_button(
    ui.tab_row, find_icon("Fire"), ctx->icon_font,
    ClimateControlTab::MODE, ctx->width_compensation_percent);
  ui.preset_tab = climate_control_create_tab_button(
    ui.tab_row, find_icon("Air Filter"), ctx->icon_font,
    ClimateControlTab::PRESET, ctx->width_compensation_percent);
  ui.fan_tab = climate_control_create_tab_button(
    ui.tab_row, find_icon("Fan"), ctx->icon_font,
    ClimateControlTab::FAN, ctx->width_compensation_percent);
  ui.swing_tab = climate_control_create_tab_button(
    ui.tab_row, find_icon("Arrow Up Down"), ctx->icon_font,
    ClimateControlTab::SWING, ctx->width_compensation_percent);

  ui.menu_view = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_opa(ui.menu_view, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.menu_view, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.menu_view, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.menu_view, LV_OBJ_FLAG_HIDDEN);

  ui.menu_close_btn = control_modal_create_round_button(ui.panel, 32, "\U000F0156", ctx->icon_font,
    DARK_BORDER, SECONDARY_GREY, ctx->width_compensation_percent);
  lv_obj_set_style_bg_opa(ui.menu_close_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.menu_close_btn, 0, LV_PART_MAIN);
  lv_obj_add_flag(ui.menu_close_btn, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.menu_close_btn, [](lv_event_t *) {
    climate_hide_action_menu();
  }, LV_EVENT_CLICKED, nullptr);

  ui.menu_mode_btn = climate_create_menu_tile(ui.menu_view, find_icon("Fire"), "Mode",
    ctx->icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.menu_mode_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "hvac");
  }, LV_EVENT_CLICKED, nullptr);
  ui.menu_preset_btn = climate_create_menu_tile(ui.menu_view, find_icon("Air Filter"), "Preset",
    ctx->icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  lv_obj_add_event_cb(ui.menu_preset_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "preset");
  }, LV_EVENT_CLICKED, nullptr);

  ui.arc = lv_arc_create(ui.panel);
  lv_arc_set_bg_angles(ui.arc, 135, 45);
  lv_arc_set_range(ui.arc, ctx->min_tenths, ctx->max_tenths);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ui.arc, lv_color_hex(DARK_TRACK_BACKGROUND), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ui.arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(ui.arc, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(ui.arc, 0, LV_PART_KNOB);
  lv_obj_add_flag(ui.arc, LV_OBJ_FLAG_ADV_HITTEST);
  auto create_range_arc = [&](uint32_t color) {
    lv_obj_t *segment = lv_arc_create(ui.panel);
    lv_arc_set_bg_angles(segment, 135, 45);
    lv_arc_set_range(segment, ctx->min_tenths, ctx->max_tenths);
    lv_arc_set_value(segment, ctx->min_tenths);
    lv_obj_set_style_bg_opa(segment, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(segment, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_color(segment, lv_color_hex(color), LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(segment, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(segment, 0, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(segment, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(segment, 0, LV_PART_KNOB);
    lv_obj_set_style_shadow_width(segment, 0, LV_PART_KNOB);
    lv_obj_clear_flag(segment, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(segment, LV_OBJ_FLAG_HIDDEN);
    return segment;
  };
  ui.heat_arc = create_range_arc(climate_heating_color(ctx));
  ui.cool_arc = create_range_arc(CLIMATE_COOLING_COLOR);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    ui.drag_target_selected = false;
    if (!ui.active || !climate_dual_target(ui.active) ||
        !ui.low_handle_dot || !ui.high_handle_dot) return;
    lv_indev_t *indev = lv_event_get_indev(e);
    if (!indev) indev = lv_indev_active();
    if (!indev) return;
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    lv_area_t low_area;
    lv_area_t high_area;
    lv_obj_get_coords(ui.low_handle_dot, &low_area);
    lv_obj_get_coords(ui.high_handle_dot, &high_area);
    int32_t low_x = (low_area.x1 + low_area.x2) / 2;
    int32_t low_y = (low_area.y1 + low_area.y2) / 2;
    int32_t high_x = (high_area.x1 + high_area.x2) / 2;
    int32_t high_y = (high_area.y1 + high_area.y2) / 2;
    ui.active->edit_high = espcontrol::climate::nearest_handle_is_high(
      point.x, point.y, low_x, low_y, high_x, high_y);
    ui.drag_target_selected = true;
    climate_control_set_modal_value(ui.active);
  }, LV_EVENT_PRESSED, nullptr);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    ui.dragging_arc = true;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int target = climate_target_from_modal_arc_value(ui.active, lv_arc_get_value(arc));
    if (climate_dual_target(ui.active) && !ui.drag_target_selected) {
      climate_select_nearest_target(ui.active, target);
      ui.drag_target_selected = true;
    }
    climate_preview_selected_target(ui.active, target);
  }, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *e) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.updating_arc || !ui.active) return;
    lv_obj_t *arc = static_cast<lv_obj_t *>(lv_event_get_target(e));
    int value = ui.has_drag_preview
      ? ui.drag_preview_tenths
      : climate_target_from_modal_arc_value(ui.active, lv_arc_get_value(arc));
    ui.dragging_arc = false;
    ui.drag_target_selected = false;
    ui.has_drag_preview = false;
    climate_apply_selected_target(ui.active, value, true, false);
  }, LV_EVENT_RELEASED, nullptr);
  lv_obj_add_event_cb(ui.arc, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (!ui.active) return;
    ui.dragging_arc = false;
    ui.drag_target_selected = false;
    ui.has_drag_preview = false;
    climate_control_set_modal_value(ui.active);
  }, LV_EVENT_PRESS_LOST, nullptr);

  ui.current_dot = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.current_dot, lv_color_hex(DARK_CONTROL_NEUTRAL), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.current_dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.current_dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.current_dot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.current_dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.current_dot, LV_OBJ_FLAG_HIDDEN);

  ui.handle_dot = lv_obj_create(ui.panel);
  lv_obj_set_style_bg_color(ui.handle_dot, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(ui.handle_dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.handle_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(ui.handle_dot, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.handle_dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(ui.handle_dot, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(ui.handle_dot, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(ui.handle_dot, LV_OBJ_FLAG_HIDDEN);

  auto create_range_handle = [&]() {
    lv_obj_t *dot = lv_obj_create(ui.panel);
    lv_obj_set_style_bg_color(dot, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(dot, lv_color_hex(DARK_TRACK_BACKGROUND), LV_PART_MAIN);
    lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(dot, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(dot, 0, LV_PART_MAIN);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
    return dot;
  };
  ui.low_handle_dot = create_range_handle();
  ui.high_handle_dot = create_range_handle();

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
  lv_obj_set_style_text_color(ui.target_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.target_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->number_font) lv_obj_set_style_text_font(ui.target_lbl, ctx->number_font, LV_PART_MAIN);
  apply_width_compensation(ui.target_lbl, ctx->width_compensation_percent);

  auto create_range_target_label = [&]() {
    lv_obj_t *label = lv_label_create(ui.target_row);
    lv_obj_set_style_text_color(label, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
    lv_obj_set_style_text_opa(label, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(label, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    const lv_font_t *range_font = ctx->range_number_font
      ? ctx->range_number_font : ctx->number_font;
    if (range_font) lv_obj_set_style_text_font(label, range_font, LV_PART_MAIN);
    apply_width_compensation(label, ctx->width_compensation_percent);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(label, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(label, LV_OBJ_FLAG_HIDDEN);
    return label;
  };
  ui.low_target_lbl = create_range_target_label();

  ui.target_separator_lbl = lv_label_create(ui.target_row);
  lv_label_set_text(ui.target_separator_lbl, "-");
  lv_obj_set_style_text_color(ui.target_separator_lbl,
                              lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.target_separator_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  const lv_font_t *range_font = ctx->range_number_font
    ? ctx->range_number_font : ctx->number_font;
  if (range_font)
    lv_obj_set_style_text_font(ui.target_separator_lbl, range_font, LV_PART_MAIN);
  lv_obj_add_flag(ui.target_separator_lbl, LV_OBJ_FLAG_HIDDEN);

  ui.high_target_lbl = create_range_target_label();

  ui.unit_lbl = lv_label_create(ui.target_row);
  lv_obj_set_style_text_color(ui.unit_lbl, lv_color_hex(DARK_TEXT_PRIMARY), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.unit_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->unit_font) lv_obj_set_style_text_font(ui.unit_lbl, ctx->unit_font, LV_PART_MAIN);
  apply_width_compensation(ui.unit_lbl, ctx->width_compensation_percent);

  ui.status_lbl = lv_label_create(ui.panel);
  lv_obj_set_style_text_color(ui.status_lbl, lv_color_hex(DARK_TEXT_MUTED), LV_PART_MAIN);
  lv_obj_set_style_text_align(ui.status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
  if (ctx->label_font) lv_obj_set_style_text_font(ui.status_lbl, ctx->label_font, LV_PART_MAIN);

  ui.minus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Minus"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, SECONDARY_GREY, ctx->width_compensation_percent);
  ui.plus_btn = control_modal_create_round_button(ui.panel, 72, find_icon("Plus"), ctx->icon_font,
    DARK_CONTROL_NEUTRAL, SECONDARY_GREY, ctx->width_compensation_percent);
  climate_apply_step_button_icon_size(ui.minus_btn);
  climate_apply_step_button_icon_size(ui.plus_btn);
  lv_obj_add_event_cb(ui.minus_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_apply_selected_target(ui.active,
      climate_selected_target(ui.active) - climate_effective_step_tenths(ui.active), false, true);
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.plus_btn, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_apply_selected_target(ui.active,
      climate_selected_target(ui.active) + climate_effective_step_tenths(ui.active), false, true);
  }, LV_EVENT_CLICKED, nullptr);

  ui.chips = lv_obj_create(ui.panel);
  lv_obj_set_width(ui.chips, lv_pct(CLIMATE_OPTION_ROW_WIDTH_PERCENT));
  lv_obj_set_style_bg_opa(ui.chips, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(ui.chips, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(ui.chips, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_column(ui.chips, 24, LV_PART_MAIN);
  lv_obj_set_layout(ui.chips, LV_LAYOUT_FLEX);
  lv_obj_set_style_flex_flow(ui.chips, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
  lv_obj_set_style_flex_main_place(ui.chips, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_set_style_flex_cross_place(ui.chips, LV_FLEX_ALIGN_CENTER, LV_PART_MAIN);
  lv_obj_add_flag(ui.chips, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scroll_dir(ui.chips, LV_DIR_HOR);
  lv_obj_set_scrollbar_mode(ui.chips, LV_SCROLLBAR_MODE_OFF);

  const lv_font_t *chip_icon_font = ctx->card_icon_font ? ctx->card_icon_font : ctx->icon_font;
  ui.target_chip = climate_create_option_chip(ui.chips, find_icon("Thermometer"), "Target",
    chip_icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  ui.mode_chip = climate_create_option_chip(ui.chips, find_icon("Fire"), "Mode",
    chip_icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  ui.preset_chip = climate_create_option_chip(ui.chips, find_icon("Air Filter"), "Preset",
    chip_icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  ui.fan_chip = climate_create_option_chip(ui.chips, find_icon("Fan"), "Fan",
    chip_icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  ui.swing_chip = climate_create_option_chip(ui.chips, find_icon("Swap Horizontal"), "Swing",
    chip_icon_font, ctx->option_title_font, ctx->option_value_font,
    ctx->width_compensation_percent);
  lv_obj_add_flag(ui.mode_chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.preset_chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.fan_chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui.swing_chip, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_event_cb(ui.target_chip, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "target");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.mode_chip, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "hvac");
  }, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(ui.preset_chip, [](lv_event_t *) {
    ClimateControlModalUi &ui = climate_control_modal_ui();
    if (ui.active) climate_open_option_menu(ui.active, "preset");
  }, LV_EVENT_CLICKED, nullptr);
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
                                         const ParsedCfg &p,
                                         const lv_font_t *icon_font) {
  bool show_icon = normalize_climate_number_display(cfg_option_value(p.options, "number_display")) == "icon";
  if (icon_lbl) {
    if (show_icon && icon_font)
      lv_obj_set_style_text_font(icon_lbl, icon_font, LV_PART_MAIN);
    lv_label_set_text(icon_lbl, (p.icon.empty() || p.icon == "Auto") ? find_icon("Thermostat") : find_icon(p.icon.c_str()));
    if (show_icon) {
      climate_layout_card_icon(icon_lbl);
      lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
    }
  }
  if (sensor_container) {
    if (show_icon) lv_obj_add_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(sensor_container, LV_OBJ_FLAG_HIDDEN);
    climate_layout_card_sensor(sensor_container);
  }
  if (sensor_lbl) lv_label_set_text(sensor_lbl, "--");
  if (unit_lbl) lv_label_set_text(unit_lbl, "");
  if (text_lbl) {
    lv_label_set_text(text_lbl, p.label.empty() ? espcontrol_i18n("Climate") : p.label.c_str());
    climate_layout_card_label(text_lbl);
  }
  apply_push_button_transition(btn);
}

inline ClimateControlCtx *create_climate_control_context(
    lv_obj_t *btn, lv_obj_t *icon_lbl, lv_obj_t *label_lbl, const ParsedCfg &p,
    uint32_t accent_color, uint32_t secondary_color, uint32_t tertiary_color,
    const lv_font_t *number_font, const lv_font_t *range_number_font,
    const lv_font_t *unit_font,
    const lv_font_t *label_font, const lv_font_t *option_title_font,
    const lv_font_t *option_value_font, const lv_font_t *option_menu_font,
    const lv_font_t *card_icon_font, const lv_font_t *icon_font,
    int width_compensation_percent,
    lv_obj_t *sensor_container, lv_obj_t *value_lbl, lv_obj_t *unit_lbl) {
  ClimateControlCtx *ctx = new ClimateControlCtx();
  ctx->entity_id = p.entity;
  ctx->configured_label = p.label;
  ctx->precision = parse_precision(p.precision);
  climate_apply_saved_range(ctx, p.precision);
  ctx->label_display = normalize_climate_label_display(cfg_option_value(p.options, "label_display"));
  ctx->number_display = normalize_climate_number_display(cfg_option_value(p.options, "number_display"));
  ctx->configured_step_tenths = normalize_climate_temperature_step(
    cfg_option_value(p.options, "temperature_step")) == "0.5"
      ? CLIMATE_DEFAULT_STEP_TENTHS
      : CLIMATE_WHOLE_NUMBER_STEP_TENTHS;
  ctx->options = p.options;
  ctx->accent_color = accent_color;
  ctx->secondary_color = secondary_color;
  ctx->tertiary_color = tertiary_color;
  ctx->btn = btn;
  ctx->icon_lbl = icon_lbl;
  ctx->label_lbl = label_lbl;
  ctx->sensor_container = sensor_container;
  ctx->value_lbl = value_lbl;
  ctx->unit_lbl = unit_lbl;
  ctx->icon_off_glyph = (p.icon.empty() || p.icon == "Auto") ? find_icon("Thermostat") : find_icon(p.icon.c_str());
  ctx->icon_on_glyph = (p.icon_on.empty() || p.icon_on == "Auto") ? ctx->icon_off_glyph : find_icon(p.icon_on.c_str());
  ctx->number_font = number_font;
  ctx->range_number_font = range_number_font ? range_number_font : number_font;
  ctx->unit_font = unit_font;
  ctx->label_font = label_font;
  ctx->option_title_font = option_title_font ? option_title_font : label_font;
  ctx->option_value_font = option_value_font ? option_value_font : label_font;
  ctx->option_menu_font = option_menu_font ? option_menu_font : ctx->option_value_font;
  ctx->card_icon_font = card_icon_font ? card_icon_font : icon_font;
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
  const uint32_t generation = ha_subscription_generation();
  auto active = [generation]() {
    return generation == ha_subscription_generation();
  };
  auto refresh = [ctx, active]() {
    if (!active()) return;
    climate_update_card(ctx);
    climate_control_set_modal_value(ctx);
  };
  ha_subscribe_state(
    ctx->entity_id,
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef state) {
        if (!active()) return;
        std::string mode = climate_hvac_service_value(
          string_ref_limited(state, HA_SHORT_STATE_MAX_LEN));
        if (mode != ctx->hvac_mode) climate_select_target_for_mode(ctx, mode);
        ctx->hvac_mode = mode;
        ctx->available = !climate_unavailable_value(ctx->hvac_mode);
        if (!ctx->available) ctx->hvac_mode = "off";
        if (!ctx->available) climate_control_hide_modal();
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("friendly_name"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        ctx->friendly_name = string_ref_limited(value, HA_FRIENDLY_NAME_MAX_LEN);
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("hvac_action"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        ctx->hvac_action = climate_lower(climate_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("supported_features"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        ClimateTargetKind previous_kind = climate_target_kind(ctx);
        int features = 0;
        ctx->supported_features_known =
          climate_parse_supported_features(value, features);
        ctx->supported_features = ctx->supported_features_known ? features : 0;
        ClimateTargetKind next_kind = climate_target_kind(ctx);
        if (espcontrol::climate::capability_change_invalidates_pending(
              previous_kind, next_kind, climate_target_values_complete(ctx)))
          climate_cancel_temperature_send(ctx);
        refresh();
      })
  );
  auto subscribe_temp = [ctx, refresh, active](const char *attr, int ClimateControlCtx::*field, bool ClimateControlCtx::*has_field) {
    ha_subscribe_attribute(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, active, field, has_field](esphome::StringRef value) {
          if (!active()) return;
          int tenths = 0;
          if (climate_parse_tenths(value, tenths)) {
            // Home Assistant can send temperatures before min/max attributes on boot.
            // Store the real value and clamp only where the dial needs a bounded range.
            ctx->*field = tenths;
            ctx->*has_field = true;
          } else {
            ctx->*has_field = false;
          }
          if (!climate_target_values_complete(ctx))
            climate_cancel_temperature_send(ctx);
          refresh();
        })
    );
  };
  subscribe_temp("current_temperature", &ClimateControlCtx::current_tenths, &ClimateControlCtx::has_current);
  subscribe_temp("temperature", &ClimateControlCtx::target_tenths, &ClimateControlCtx::has_target);
  subscribe_temp("target_temp_low", &ClimateControlCtx::low_tenths, &ClimateControlCtx::has_low);
  subscribe_temp("target_temp_high", &ClimateControlCtx::high_tenths, &ClimateControlCtx::has_high);
  ha_subscribe_attribute(
    ctx->entity_id, std::string("min_temp"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        int tenths = 0;
        if (!ctx->custom_min && climate_parse_tenths(value, tenths)) {
          ctx->min_tenths = tenths;
          ctx->received_min = true;
        }
        climate_normalize_range(ctx);
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("max_temp"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        int tenths = 0;
        if (!ctx->custom_max && climate_parse_tenths(value, tenths)) {
          ctx->max_tenths = tenths;
          ctx->received_max = true;
        }
        climate_normalize_range(ctx);
        refresh();
      })
  );
  ha_subscribe_attribute(
    ctx->entity_id, std::string("target_temp_step"),
    std::function<void(esphome::StringRef)>(
      [ctx, refresh, active](esphome::StringRef value) {
        if (!active()) return;
        int tenths = 0;
        if (climate_parse_tenths(value, tenths) && tenths > 0 && tenths <= 100)
          ctx->step_tenths = tenths;
        else
          ctx->step_tenths = CLIMATE_DEFAULT_STEP_TENTHS;
        refresh();
      })
  );
  auto subscribe_text = [ctx, refresh, active](const char *attr, std::string ClimateControlCtx::*field) {
    ha_subscribe_attribute(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, active, field](esphome::StringRef value) {
          if (!active()) return;
          ctx->*field = climate_lower(climate_trim(string_ref_limited(value, HA_SHORT_STATE_MAX_LEN)));
          refresh();
        })
    );
  };
  subscribe_text("fan_mode", &ClimateControlCtx::fan_mode);
  subscribe_text("swing_mode", &ClimateControlCtx::swing_mode);
  subscribe_text("preset_mode", &ClimateControlCtx::preset_mode);
  auto subscribe_list = [ctx, refresh, active](const char *attr, std::vector<std::string> ClimateControlCtx::*field) {
    ha_subscribe_attribute(
      ctx->entity_id, std::string(attr),
      std::function<void(esphome::StringRef)>(
        [ctx, refresh, active, field](esphome::StringRef value) {
          if (!active()) return;
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
