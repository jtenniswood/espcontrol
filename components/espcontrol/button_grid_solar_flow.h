// =============================================================================
// SOLAR FLOW CARD — LVGL rendering for mode=flow
// =============================================================================
// Energy-flow diagram: lv_arc ring nodes (Solar, Home, Battery, Grid) whose
// arc fill shows power as % of solar production. Connected by thin lines.
// Called from solar_apply_card_face when ctx->mode == "flow".
// =============================================================================
#pragma once

#include "button_grid_solar.h"

// ── Widget containers ─────────────────────────────────────────────────────────

struct SolarFlowNode {
  lv_obj_t *arc      = nullptr;  // lv_arc ring (270° sweep, fill = % of solar)
  lv_obj_t *val_lbl  = nullptr;  // primary value text (inside arc)
  lv_obj_t *sub_lbl  = nullptr;  // secondary value (grid: export/import lines)
  lv_obj_t *name_lbl = nullptr;  // node name below arc ("Solar", "Home", etc.)
};

struct SolarFlowWidgets {
  SolarFlowNode solar_node;
  SolarFlowNode home_node;
  SolarFlowNode battery_node;
  SolarFlowNode grid_node;

  lv_obj_t *line_top  = nullptr;  // solar → center
  lv_obj_t *line_bot  = nullptr;  // center → battery (hidden when no battery)
  lv_obj_t *line_left = nullptr;  // grid → center   (hidden when no grid)
  lv_obj_t *line_right= nullptr;  // center → home
  lv_obj_t *center    = nullptr;  // small center junction dot

  bool layout_2x2  = false;
  bool initialized = false;
};

// ── Color constants ───────────────────────────────────────────────────────────

static constexpr uint32_t FLOW_COLOR_SOLAR   = 0xF59E0B;  // amber
static constexpr uint32_t FLOW_COLOR_HOME    = 0x3B82F6;  // blue
static constexpr uint32_t FLOW_COLOR_BATTERY = 0x22C55E;  // green
static constexpr uint32_t FLOW_COLOR_GRID    = 0x8B5CF6;  // purple
static constexpr uint32_t FLOW_COLOR_EXPORT  = 0xFCD34D;  // yellow (to-grid)
static constexpr uint32_t FLOW_COLOR_IMPORT  = 0xF87171;  // red (from-grid)
static constexpr uint32_t FLOW_TRACK_OPA     = 25;        // track bg opacity %

// Darken a color by mixing with black (factor 0-100, 100 = full black)
static inline uint32_t flow_darken(uint32_t c, int factor) {
  int r = ((c >> 16) & 0xFF) * (100 - factor) / 100;
  int g = ((c >>  8) & 0xFF) * (100 - factor) / 100;
  int b = ( c        & 0xFF) * (100 - factor) / 100;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

// ── Create a single arc node ──────────────────────────────────────────────────

inline SolarFlowNode solar_flow_make_node(lv_obj_t *parent,
                                          int x, int y, int size,
                                          uint32_t color,
                                          const lv_font_t *font,
                                          const char *name) {
  SolarFlowNode n;

  // ── lv_arc ring (270° sweep, 135° start, 45° end) ──
  n.arc = lv_arc_create(parent);
  lv_obj_set_size(n.arc, size, size);
  lv_obj_set_pos(n.arc, x - size / 2, y - size / 2);
  lv_arc_set_bg_angles(n.arc, 135, 45);   // same sweep as climate card
  lv_arc_set_range(n.arc, 0, 100);
  lv_arc_set_value(n.arc, 0);
  lv_obj_clear_flag(n.arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(n.arc, LV_OBJ_FLAG_SCROLLABLE);

  // Arc background (track)
  lv_obj_set_style_bg_opa(n.arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_border_width(n.arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(n.arc, lv_color_hex(flow_darken(color, 65)), LV_PART_MAIN);
  lv_obj_set_style_arc_width(n.arc, 5, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(n.arc, true, LV_PART_MAIN);

  // Arc indicator (filled portion)
  lv_obj_set_style_arc_color(n.arc, lv_color_hex(color), LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(n.arc, 5, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(n.arc, true, LV_PART_INDICATOR);

  // Remove knob completely
  lv_obj_set_style_bg_opa(n.arc, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_width(n.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_shadow_width(n.arc, 0, LV_PART_KNOB);
  lv_obj_set_style_pad_all(n.arc, 0, LV_PART_KNOB);

  // ── Value label (centered inside arc) ──
  n.val_lbl = lv_label_create(n.arc);
  lv_obj_set_style_text_color(n.val_lbl, lv_color_hex(color), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(n.val_lbl, font, LV_PART_MAIN);
  lv_label_set_text(n.val_lbl, "--");
  lv_obj_align(n.val_lbl, LV_ALIGN_CENTER, 0, -4);

  // ── Unit / secondary label ──
  n.sub_lbl = lv_label_create(n.arc);
  lv_obj_set_style_text_color(n.sub_lbl, lv_color_hex(0x888888), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(n.sub_lbl, font, LV_PART_MAIN);
  lv_label_set_text(n.sub_lbl, "");
  lv_obj_align(n.sub_lbl, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_flag(n.sub_lbl, LV_OBJ_FLAG_HIDDEN);

  // ── Name label below arc ──
  if (name) {
    n.name_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(n.name_lbl, lv_color_hex(flow_darken(color, 20)), LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(n.name_lbl, font, LV_PART_MAIN);
    lv_label_set_text(n.name_lbl, name);
    lv_obj_align_to(n.name_lbl, n.arc, LV_ALIGN_OUT_BOTTOM_MID, 0, 2);
  }

  return n;
}

// ── Create a connecting line ──────────────────────────────────────────────────

inline lv_obj_t *solar_flow_make_line(lv_obj_t *parent,
                                      lv_coord_t x1, lv_coord_t y1,
                                      lv_coord_t x2, lv_coord_t y2,
                                      uint32_t color) {
  lv_obj_t *line = lv_obj_create(parent);
  bool vertical = (x1 == x2);
  lv_coord_t lw = 2;
  if (vertical) {
    lv_obj_set_pos(line, x1 - lw / 2, y1);
    lv_obj_set_size(line, lw, y2 - y1);
  } else {
    lv_obj_set_pos(line, x1, y1 - lw / 2);
    lv_obj_set_size(line, x2 - x1, lw);
  }
  lv_obj_set_style_bg_color(line, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(line, LV_OPA_40, LV_PART_MAIN);
  lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
  return line;
}

// ── Center dot ────────────────────────────────────────────────────────────────

inline lv_obj_t *solar_flow_make_dot(lv_obj_t *parent, int x, int y, uint32_t color) {
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, 7, 7);
  lv_obj_set_pos(dot, x - 3, y - 3);
  lv_obj_set_style_radius(dot, LV_PCT(50), LV_PART_MAIN);
  lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  return dot;
}

// ── Layout initialization ─────────────────────────────────────────────────────

inline void solar_flow_init_widgets(SolarCardCtx *ctx, bool layout_2x2,
                                    const lv_font_t *font) {
  lv_obj_t   *btn = ctx->btn;
  lv_coord_t  W   = lv_obj_get_width(btn);
  lv_coord_t  H   = lv_obj_get_height(btn);
  SolarFlowWidgets *fw = ctx->flow_widgets;

  // Node size: 20% larger, whole layout shifted left 5%
  int offset_x = -(int)(W * 0.05f);
  int node_sz  = layout_2x2 ? (int)(W * 0.264f) : (int)(W * 0.432f);
  if (node_sz < 44) node_sz = 44;
  if (node_sz > 90) node_sz = 90;

  int cx = W / 2 + offset_x, cy = H / 2;
  int solar_x   = cx;                         int solar_y   = (int)(H * 0.18f);
  int home_x    = (int)(W * 0.82f) + offset_x; int home_y  = cy;
  int battery_x = cx;                          int battery_y = (int)(H * 0.82f);
  int grid_x    = (int)(W * 0.18f) + offset_x; int grid_y  = cy;

  if (!layout_2x2) {
    solar_y = (int)(H * 0.22f);
    home_x  = cx; home_y = (int)(H * 0.75f);
  }

  int r = node_sz / 2;

  // Lines first (rendered behind nodes)
  if (layout_2x2) {
    fw->line_top  = solar_flow_make_line(btn, cx, solar_y   + r, cx,        cy,            FLOW_COLOR_SOLAR);
    fw->line_bot  = solar_flow_make_line(btn, cx, cy,            cx,        battery_y - r, FLOW_COLOR_BATTERY);
    fw->line_left = solar_flow_make_line(btn, grid_x + r,  cy,   cx,        cy,            FLOW_COLOR_GRID);
    fw->line_right= solar_flow_make_line(btn, cx,          cy,   home_x - r, cy,           FLOW_COLOR_SOLAR);
    fw->center    = solar_flow_make_dot(btn, cx, cy, 0x555555);
  } else {
    fw->line_top  = solar_flow_make_line(btn, cx, solar_y + r, cx, home_y - r, FLOW_COLOR_SOLAR);
  }

  // Nodes (rendered on top of lines)
  fw->solar_node = solar_flow_make_node(btn, solar_x, solar_y, node_sz, FLOW_COLOR_SOLAR,   font, "Solar");
  fw->home_node  = solar_flow_make_node(btn, home_x,  home_y,  node_sz, FLOW_COLOR_HOME,    font, "Home");
  if (layout_2x2) {
    fw->battery_node = solar_flow_make_node(btn, battery_x, battery_y, node_sz, FLOW_COLOR_BATTERY, font, "Battery");
    fw->grid_node    = solar_flow_make_node(btn, grid_x,    grid_y,    node_sz, FLOW_COLOR_GRID,    font, "Grid");
  }

  if (layout_2x2 && fw->center) lv_obj_move_foreground(fw->center);

  fw->layout_2x2  = layout_2x2;
  fw->initialized = true;
}

// ── Format a value for node display ──────────────────────────────────────────

inline std::string solar_flow_fmt(const SolarField &f, bool invert) {
  if (!f.available || f.value.empty()) return "--";
  char *ep = nullptr;
  double v = std::strtod(f.value.c_str(), &ep);
  if (ep == f.value.c_str()) return f.value;
  if (invert) v = -v;
  std::string unit_out;
  return solar_format_qty(v, f.unit, unit_out) + " " + unit_out;
}

// Parse a SolarField to kW (returns 0 if unavailable/non-numeric)
inline double solar_flow_to_kw(const SolarField &f, bool invert = false) {
  if (!f.available || f.value.empty()) return 0.0;
  char *ep = nullptr;
  double v = std::strtod(f.value.c_str(), &ep);
  if (ep == f.value.c_str()) return 0.0;
  if (invert) v = -v;
  // Normalise to kW
  if (f.unit == "W") v /= 1000.0;
  return v;
}

// ── Update a node's value label and arc fill ──────────────────────────────────

inline void solar_flow_update_node(SolarFlowNode &n, const std::string &val,
                                   int arc_pct) {
  if (!n.arc) return;
  lv_arc_set_value(n.arc, arc_pct < 0 ? 0 : (arc_pct > 100 ? 100 : arc_pct));
  if (n.val_lbl) {
    lv_label_set_text(n.val_lbl, val.c_str());
    lv_obj_align(n.val_lbl, LV_ALIGN_CENTER, 0, n.sub_lbl ? -4 : 0);
  }
}

// ── Main flow card face update ────────────────────────────────────────────────

inline void solar_flow_apply_card_face(SolarCardCtx *ctx) {
  if (!ctx || !ctx->btn) return;

  // Hide standard hero widgets
  if (ctx->value_lbl)  lv_obj_add_flag(ctx->value_lbl,  LV_OBJ_FLAG_HIDDEN);
  if (ctx->unit_lbl)   lv_obj_add_flag(ctx->unit_lbl,   LV_OBJ_FLAG_HIDDEN);
  if (ctx->label_lbl)  lv_obj_add_flag(ctx->label_lbl,  LV_OBJ_FLAG_HIDDEN);
  if (ctx->icon_lbl)   lv_obj_add_flag(ctx->icon_lbl,   LV_OBJ_FLAG_HIDDEN);
  if (ctx->corner_lbl) lv_obj_add_flag(ctx->corner_lbl, LV_OBJ_FLAG_HIDDEN);

  lv_coord_t W = lv_obj_get_width(ctx->btn);
  lv_coord_t H = lv_obj_get_height(ctx->btn);
  if (W < 20 || H < 20) return;

  bool layout_2x2 = (W >= 180 && H >= 180);
  bool has_battery = !ctx->battery.entity_id.empty();
  bool has_grid    = !ctx->from_grid.entity_id.empty() || !ctx->to_grid.entity_id.empty();
  bool want_2x2    = layout_2x2 && (has_battery || has_grid);

  if (!ctx->flow_widgets) ctx->flow_widgets = new SolarFlowWidgets();
  SolarFlowWidgets *fw = ctx->flow_widgets;

  if (!fw->initialized || fw->layout_2x2 != want_2x2) {
    // Delete only tracked flow widgets (never iterate btn children)
    auto del_if = [](lv_obj_t *&o) { if (o) { lv_obj_del(o); o = nullptr; } };
    auto del_node = [&](SolarFlowNode &n) {
      del_if(n.arc); n.val_lbl = nullptr; n.sub_lbl = nullptr;
      del_if(n.name_lbl);
    };
    del_node(fw->solar_node);   del_node(fw->home_node);
    del_node(fw->battery_node); del_node(fw->grid_node);
    del_if(fw->line_top);  del_if(fw->line_bot);
    del_if(fw->line_left); del_if(fw->line_right);
    del_if(fw->center);
    *fw = SolarFlowWidgets{};
    solar_flow_init_widgets(ctx, want_2x2, ctx->label_font);
  }

  // Background: solid off_color
  lv_style_selector_t sel = static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT;
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(ctx->off_color), sel);
  lv_obj_set_style_bg_grad_dir(ctx->btn, LV_GRAD_DIR_NONE, sel);

  // Compute solar kW as the 100% baseline for arc fills
  double solar_kw = solar_flow_to_kw(ctx->production, false);
  auto arc_pct = [&](double kw) -> int {
    if (solar_kw <= 0.001) return 0;
    double pct = std::fabs(kw) / solar_kw * 100.0;
    return (int)(pct + 0.5);
  };

  // ── Solar node ──
  solar_flow_update_node(fw->solar_node,
    solar_flow_fmt(ctx->production, false), 100);

  // ── Home node ──
  double home_kw = solar_flow_to_kw(ctx->consumption, false);
  solar_flow_update_node(fw->home_node,
    solar_flow_fmt(ctx->consumption, false), arc_pct(home_kw));

  if (fw->layout_2x2) {
    auto show = [](lv_obj_t *o, bool v) {
      if (!o) return;
      if (v) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
      else   lv_obj_add_flag(o,   LV_OBJ_FLAG_HIDDEN);
    };

    // ── Battery node ──
    if (has_battery && fw->battery_node.arc) {
      double bat_kw = solar_flow_to_kw(ctx->battery, ctx->invert_production);
      solar_flow_update_node(fw->battery_node,
        solar_flow_fmt(ctx->battery, ctx->invert_production), arc_pct(bat_kw));
    }
    show(fw->battery_node.arc,      has_battery);
    show(fw->battery_node.name_lbl, has_battery);
    show(fw->line_bot, has_battery);

    // ── Grid node ──
    if (has_grid && fw->grid_node.arc) {
      std::string to_str   = ctx->to_grid.available   && !ctx->to_grid.value.empty()
        ? solar_flow_fmt(ctx->to_grid,   false) : "--";
      std::string from_str = ctx->from_grid.available && !ctx->from_grid.value.empty()
        ? solar_flow_fmt(ctx->from_grid, false) : "--";

      double to_kw   = solar_flow_to_kw(ctx->to_grid,   false);
      double from_kw = solar_flow_to_kw(ctx->from_grid, false);
      int    pct     = arc_pct(to_kw > from_kw ? to_kw : from_kw);

      lv_arc_set_value(fw->grid_node.arc, pct > 100 ? 100 : pct);

      if (fw->grid_node.val_lbl) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "+%s", to_str.c_str());
        lv_label_set_text(fw->grid_node.val_lbl, buf);
        lv_obj_set_style_text_color(fw->grid_node.val_lbl,
          lv_color_hex(FLOW_COLOR_EXPORT), LV_PART_MAIN);
        lv_obj_align(fw->grid_node.val_lbl, LV_ALIGN_CENTER, 0, -6);
      }
      if (fw->grid_node.sub_lbl) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "-%s", from_str.c_str());
        lv_label_set_text(fw->grid_node.sub_lbl, buf);
        lv_obj_set_style_text_color(fw->grid_node.sub_lbl,
          lv_color_hex(FLOW_COLOR_IMPORT), LV_PART_MAIN);
        lv_obj_clear_flag(fw->grid_node.sub_lbl, LV_OBJ_FLAG_HIDDEN);
      }
    }
    show(fw->grid_node.arc,      has_grid);
    show(fw->grid_node.name_lbl, has_grid);
    show(fw->line_left, has_grid);
  }
}
