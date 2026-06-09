// =============================================================================
// SOLAR FLOW CARD — LVGL rendering for mode=flow
// =============================================================================
// Energy-flow diagram: circular nodes (Solar, Home, Battery, Grid) connected
// by lines. Direction dots show power flow direction. Static (no animation).
// Called from solar_apply_card_face when ctx->mode == "flow".
// =============================================================================
#pragma once

#include "button_grid_solar.h"

// ── Widget containers ─────────────────────────────────────────────────────────

struct SolarFlowNode {
  lv_obj_t *ring     = nullptr;  // circle container (border = ring effect)
  lv_obj_t *val_lbl  = nullptr;  // primary value text inside ring
  lv_obj_t *sub_lbl  = nullptr;  // secondary value (grid: second line)
  lv_obj_t *name_lbl = nullptr;  // node name below the ring ("Solar", etc.)
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
  lv_obj_t *center    = nullptr;  // small center junction circle

  bool layout_2x2  = false;  // true when tile is wide enough for full cross
  bool initialized = false;
};

// ── Color constants ───────────────────────────────────────────────────────────

static constexpr uint32_t FLOW_COLOR_SOLAR   = 0xF59E0B;  // amber
static constexpr uint32_t FLOW_COLOR_HOME    = 0x3B82F6;  // blue
static constexpr uint32_t FLOW_COLOR_BATTERY = 0x22C55E;  // green
static constexpr uint32_t FLOW_COLOR_GRID    = 0x8B5CF6;  // purple
static constexpr uint32_t FLOW_COLOR_EXPORT  = 0xFCD34D;  // yellow (to-grid)
static constexpr uint32_t FLOW_COLOR_IMPORT  = 0xF87171;  // red (from-grid)
static constexpr uint32_t FLOW_BG            = 0x1C1C1C;  // node background

// ── Create a single circular node ─────────────────────────────────────────────

inline SolarFlowNode solar_flow_make_node(lv_obj_t *parent,
                                          int x, int y,
                                          int size,
                                          uint32_t ring_color,
                                          const lv_font_t *font,
                                          const char *name) {
  SolarFlowNode n;
  n.ring = lv_obj_create(parent);
  lv_obj_set_size(n.ring, size, size);
  lv_obj_set_pos(n.ring, x - size / 2, y - size / 2);
  lv_obj_set_style_radius(n.ring, LV_PCT(50), LV_PART_MAIN);
  lv_obj_set_style_bg_color(n.ring, lv_color_hex(FLOW_BG), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(n.ring, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(n.ring, 3, LV_PART_MAIN);
  lv_obj_set_style_border_color(n.ring, lv_color_hex(ring_color), LV_PART_MAIN);
  lv_obj_set_style_pad_all(n.ring, 0, LV_PART_MAIN);
  lv_obj_clear_flag(n.ring, LV_OBJ_FLAG_SCROLLABLE);

  n.val_lbl = lv_label_create(n.ring);
  lv_obj_set_style_text_color(n.val_lbl, lv_color_hex(ring_color), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(n.val_lbl, font, LV_PART_MAIN);
  lv_label_set_text(n.val_lbl, "--");
  lv_obj_align(n.val_lbl, LV_ALIGN_CENTER, 0, 0);

  n.sub_lbl = lv_label_create(n.ring);
  lv_obj_set_style_text_color(n.sub_lbl, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
  if (font) lv_obj_set_style_text_font(n.sub_lbl, font, LV_PART_MAIN);
  lv_label_set_text(n.sub_lbl, "");
  lv_obj_align(n.sub_lbl, LV_ALIGN_CENTER, 0, 12);
  lv_obj_add_flag(n.sub_lbl, LV_OBJ_FLAG_HIDDEN);

  // Name label placed below the ring
  if (name) {
    n.name_lbl = lv_label_create(parent);
    lv_obj_set_style_text_color(n.name_lbl, lv_color_hex(0xAAAAAA), LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(n.name_lbl, font, LV_PART_MAIN);
    lv_label_set_text(n.name_lbl, name);
    lv_obj_align_to(n.name_lbl, n.ring, LV_ALIGN_OUT_BOTTOM_MID, 0, 3);
  }

  return n;
}

// ── Create a direction dot ────────────────────────────────────────────────────

inline lv_obj_t *solar_flow_make_dot(lv_obj_t *parent, int x, int y, uint32_t color) {
  lv_obj_t *dot = lv_obj_create(parent);
  lv_obj_set_size(dot, 8, 8);
  lv_obj_set_pos(dot, x - 4, y - 4);
  lv_obj_set_style_radius(dot, LV_PCT(50), LV_PART_MAIN);
  lv_obj_set_style_bg_color(dot, lv_color_hex(color), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
  lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
  return dot;
}

// ── Create a connecting line (thin rectangle, H or V only) ───────────────────
// x1,y1 = start; x2,y2 = end. Draws a 2px-wide rectangle along the axis.

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
  lv_obj_set_style_bg_opa(line, LV_OPA_50, LV_PART_MAIN);
  lv_obj_set_style_border_width(line, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(line, 0, LV_PART_MAIN);
  lv_obj_set_style_pad_all(line, 0, LV_PART_MAIN);
  lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
  return line;
}

// ── Initialize all flow widgets for a given tile size ─────────────────────────

inline void solar_flow_init_widgets(SolarCardCtx *ctx, bool layout_2x2,
                                    const lv_font_t *font) {
  lv_obj_t *btn    = ctx->btn;
  lv_coord_t W     = lv_obj_get_width(btn);
  lv_coord_t H     = lv_obj_get_height(btn);
  SolarFlowWidgets *fw = ctx->flow_widgets;

  // Node size: 20% larger than original, shifted left by 5% of width
  int offset_x = -(int)(W * 0.05f);
  int node_sz = layout_2x2 ? (int)(W * 0.264f) : (int)(W * 0.432f);
  if (node_sz < 44) node_sz = 44;
  if (node_sz > 90) node_sz = 90;

  // Center of tile (offset left)
  int cx = W / 2 + offset_x, cy = H / 2;

  // Compass positions for 2x2; vertical stack for 1x1
  int solar_x   = cx,                       solar_y   = (int)(H * 0.18f);
  int home_x    = (int)(W * 0.82f) + offset_x, home_y = cy;
  int battery_x = cx,                       battery_y = (int)(H * 0.82f);
  int grid_x    = (int)(W * 0.18f) + offset_x, grid_y = cy;

  if (!layout_2x2) {
    solar_y = (int)(H * 0.22f);
    home_x  = cx;
    home_y  = (int)(H * 0.75f);
  }

  int r = node_sz / 2;

  // Split lines: each arm goes from node edge to center separately
  // so we can hide individual arms (e.g. battery arm when no battery)
  if (layout_2x2) {
    fw->line_top  = solar_flow_make_line(btn, cx, solar_y + r,    cx,      cy,            FLOW_COLOR_SOLAR);
    fw->line_bot  = solar_flow_make_line(btn, cx, cy,             cx,      battery_y - r, FLOW_COLOR_BATTERY);
    fw->line_left = solar_flow_make_line(btn, grid_x + r, cy,     cx,      cy,            FLOW_COLOR_GRID);
    fw->line_right= solar_flow_make_line(btn, cx,         cy,     home_x - r, cy,         FLOW_COLOR_SOLAR);
    fw->center    = solar_flow_make_dot(btn, cx, cy, 0x555555);
  } else {
    fw->line_top  = solar_flow_make_line(btn, cx, solar_y + r, cx, home_y - r, FLOW_COLOR_SOLAR);
  }

  // Nodes (created after lines so they render on top)
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

// ── Format a sensor value for display in a flow node ──────────────────────────

inline std::string solar_flow_fmt(const SolarField &f, bool invert) {
  if (!f.available || f.value.empty()) return "--";
  char *ep = nullptr;
  double v = std::strtod(f.value.c_str(), &ep);
  if (ep == f.value.c_str()) return f.value;
  if (invert) v = -v;
  std::string unit_out;
  return solar_format_qty(v, f.unit, unit_out) + " " + unit_out;
}

// ── Update a node's primary value label ──────────────────────────────────────

inline void solar_flow_update_node(SolarFlowNode &n, const std::string &val) {
  if (!n.ring || !n.val_lbl) return;
  lv_label_set_text(n.val_lbl, val.c_str());
  lv_obj_align(n.val_lbl, LV_ALIGN_CENTER, 0, 0);
}

// ── Main flow card face update ────────────────────────────────────────────────

inline void solar_flow_apply_card_face(SolarCardCtx *ctx) {
  if (!ctx || !ctx->btn) return;

  // Hide the standard hero widgets created by setup_todo_card
  if (ctx->value_lbl)  lv_obj_add_flag(ctx->value_lbl,  LV_OBJ_FLAG_HIDDEN);
  if (ctx->unit_lbl)   lv_obj_add_flag(ctx->unit_lbl,   LV_OBJ_FLAG_HIDDEN);
  if (ctx->label_lbl)  lv_obj_add_flag(ctx->label_lbl,  LV_OBJ_FLAG_HIDDEN);
  if (ctx->icon_lbl)   lv_obj_add_flag(ctx->icon_lbl,   LV_OBJ_FLAG_HIDDEN);
  if (ctx->corner_lbl) lv_obj_add_flag(ctx->corner_lbl, LV_OBJ_FLAG_HIDDEN);

  // Determine layout from physical tile dimensions.
  // Skip if the button hasn't been laid out yet (LVGL defers layout until
  // lv_timer_handler runs; creating flow widgets with W=H=0 produces negative
  // rectangle sizes that corrupt LVGL's internal state).
  lv_coord_t W = lv_obj_get_width(ctx->btn);
  lv_coord_t H = lv_obj_get_height(ctx->btn);
  if (W < 20 || H < 20) return;
  bool layout_2x2 = (W >= 180 && H >= 180);

  bool has_battery = !ctx->battery.entity_id.empty();
  bool has_grid    = !ctx->from_grid.entity_id.empty() || !ctx->to_grid.entity_id.empty();
  bool need_4node  = layout_2x2 && (has_battery || has_grid);

  // Lazy-initialize; re-initialize if layout type changed
  if (!ctx->flow_widgets) ctx->flow_widgets = new SolarFlowWidgets();
  SolarFlowWidgets *fw = ctx->flow_widgets;

  bool want_2x2 = layout_2x2 && need_4node;
  if (!fw->initialized || fw->layout_2x2 != want_2x2) {
    // Delete only the specific flow widgets we created previously.
    // Never iterate all btn children — sensor_container (not in ctx fields)
    // contains ctx->value_lbl as a grandchild; deleting it makes value_lbl
    // a dangling pointer and crashes lv_obj_add_flag on the next render.
    auto del_if = [](lv_obj_t *&o) { if (o) { lv_obj_del(o); o = nullptr; } };
    auto del_node = [&](SolarFlowNode &n) {
      // ring deletion recursively removes val_lbl/sub_lbl children
      del_if(n.ring); n.val_lbl = nullptr; n.sub_lbl = nullptr;
      del_if(n.name_lbl);
    };
    del_node(fw->solar_node);
    del_node(fw->home_node);
    del_node(fw->battery_node);
    del_node(fw->grid_node);
    del_if(fw->line_top);
    del_if(fw->line_bot);
    del_if(fw->line_left);
    del_if(fw->line_right);
    del_if(fw->center);
    *fw = SolarFlowWidgets{};
    solar_flow_init_widgets(ctx, want_2x2, ctx->label_font);
  }

  // Solid off-color background (no gradient)
  lv_style_selector_t sel = static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT;
  lv_obj_set_style_bg_color(ctx->btn, lv_color_hex(ctx->off_color), sel);
  lv_obj_set_style_bg_grad_dir(ctx->btn, LV_GRAD_DIR_NONE, sel);

  // Update Solar and Home nodes (always present)
  solar_flow_update_node(fw->solar_node, solar_flow_fmt(ctx->production, false));
  solar_flow_update_node(fw->home_node,  solar_flow_fmt(ctx->consumption, false));

  if (fw->layout_2x2) {
    // Helper: show or hide an obj
    auto show = [](lv_obj_t *o, bool visible) {
      if (!o) return;
      if (visible) lv_obj_clear_flag(o, LV_OBJ_FLAG_HIDDEN);
      else         lv_obj_add_flag(o,   LV_OBJ_FLAG_HIDDEN);
    };

    // Battery node + bottom arm
    if (has_battery && fw->battery_node.ring) {
      solar_flow_update_node(fw->battery_node, solar_flow_fmt(ctx->battery, ctx->invert_production));
    }
    show(fw->battery_node.ring,  has_battery);
    show(fw->battery_node.name_lbl, has_battery);
    show(fw->line_bot, has_battery);

    // Grid node + left arm
    if (has_grid && fw->grid_node.ring) {
      std::string to_str   = ctx->to_grid.available   && !ctx->to_grid.value.empty()
        ? solar_flow_fmt(ctx->to_grid,   false) : "--";
      std::string from_str = ctx->from_grid.available && !ctx->from_grid.value.empty()
        ? solar_flow_fmt(ctx->from_grid, false) : "--";
      if (fw->grid_node.val_lbl) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "+%s", to_str.c_str());
        lv_label_set_text(fw->grid_node.val_lbl, buf);
        lv_obj_set_style_text_color(fw->grid_node.val_lbl, lv_color_hex(FLOW_COLOR_EXPORT), LV_PART_MAIN);
        lv_obj_align(fw->grid_node.val_lbl, LV_ALIGN_CENTER, 0, -6);
      }
      if (fw->grid_node.sub_lbl) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "-%s", from_str.c_str());
        lv_label_set_text(fw->grid_node.sub_lbl, buf);
        lv_obj_set_style_text_color(fw->grid_node.sub_lbl, lv_color_hex(FLOW_COLOR_IMPORT), LV_PART_MAIN);
        lv_obj_clear_flag(fw->grid_node.sub_lbl, LV_OBJ_FLAG_HIDDEN);
      }
    }
    show(fw->grid_node.ring,     has_grid);
    show(fw->grid_node.name_lbl, has_grid);
    show(fw->line_left, has_grid);
  }
}
