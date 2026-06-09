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
  lv_obj_t *ring    = nullptr;  // circle container (border = ring effect)
  lv_obj_t *val_lbl = nullptr;  // primary value text
  lv_obj_t *sub_lbl = nullptr;  // secondary value (grid node: second line)
};

struct SolarFlowWidgets {
  SolarFlowNode solar_node;
  SolarFlowNode home_node;
  SolarFlowNode battery_node;
  SolarFlowNode grid_node;

  lv_obj_t *line_v    = nullptr;  // vertical line   (solar <-> battery)
  lv_obj_t *line_h    = nullptr;  // horizontal line (grid <-> home)
  lv_obj_t *dot_solar = nullptr;  // midpoint dot on top arm (solar side)
  lv_obj_t *dot_bat   = nullptr;  // midpoint dot on bottom arm (battery)
  lv_obj_t *dot_grid  = nullptr;  // midpoint dot on left arm (grid)
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
                                          const lv_font_t *font) {
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

  // Node size scales with the tile dimensions
  int node_sz = layout_2x2 ? (int)(W * 0.22f) : (int)(W * 0.36f);
  if (node_sz < 38) node_sz = 38;
  if (node_sz > 78) node_sz = 78;

  int cx = W / 2, cy = H / 2;

  // Compass positions for 2x2; vertical stack for 1x1
  int solar_x   = cx,               solar_y   = (int)(H * 0.18f);
  int home_x    = (int)(W * 0.82f), home_y    = cy;
  int battery_x = cx,               battery_y = (int)(H * 0.82f);
  int grid_x    = (int)(W * 0.18f), grid_y    = cy;

  if (!layout_2x2) {
    solar_y = (int)(H * 0.22f);
    home_x  = cx;
    home_y  = (int)(H * 0.75f);
  }

  // Lines first (rendered behind nodes in LVGL child order)
  int r = node_sz / 2;
  if (layout_2x2) {
    fw->line_v = solar_flow_make_line(btn, cx, solar_y + r, cx, battery_y - r, FLOW_COLOR_SOLAR);
    fw->line_h = solar_flow_make_line(btn, grid_x + r, cy, home_x - r, cy, FLOW_COLOR_SOLAR);
    fw->center = solar_flow_make_dot(btn, cx, cy, 0x444444);
  } else {
    fw->line_v = solar_flow_make_line(btn, cx, solar_y + r, cx, home_y - r, FLOW_COLOR_SOLAR);
  }

  // Direction dots (midpoint of each arm)
  if (layout_2x2) {
    fw->dot_solar = solar_flow_make_dot(btn, cx, (solar_y + r + cy) / 2, FLOW_COLOR_SOLAR);
    fw->dot_bat   = solar_flow_make_dot(btn, cx, (battery_y - r + cy) / 2, FLOW_COLOR_BATTERY);
    fw->dot_grid  = solar_flow_make_dot(btn, (grid_x + r + cx) / 2, cy, FLOW_COLOR_GRID);
  } else {
    fw->dot_solar = solar_flow_make_dot(btn, cx, (solar_y + r + home_y - r) / 2, FLOW_COLOR_SOLAR);
  }

  // Nodes (rendered on top of lines)
  fw->solar_node = solar_flow_make_node(btn, solar_x, solar_y, node_sz, FLOW_COLOR_SOLAR, font);
  fw->home_node  = solar_flow_make_node(btn, home_x,  home_y,  node_sz, FLOW_COLOR_HOME,  font);
  if (layout_2x2) {
    fw->battery_node = solar_flow_make_node(btn, battery_x, battery_y, node_sz, FLOW_COLOR_BATTERY, font);
    fw->grid_node    = solar_flow_make_node(btn, grid_x,    grid_y,    node_sz, FLOW_COLOR_GRID,    font);
  }

  // Bring center dot above lines but below nodes
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
      // Deleting the ring recursively deletes its label children
      del_if(n.ring); n.val_lbl = nullptr; n.sub_lbl = nullptr;
    };
    del_node(fw->solar_node);
    del_node(fw->home_node);
    del_node(fw->battery_node);
    del_node(fw->grid_node);
    del_if(fw->line_v);
    del_if(fw->line_h);
    del_if(fw->dot_solar);
    del_if(fw->dot_bat);
    del_if(fw->dot_grid);
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
    // Battery node
    if (has_battery && fw->battery_node.ring) {
      solar_flow_update_node(fw->battery_node, solar_flow_fmt(ctx->battery, ctx->invert_production));
      lv_obj_clear_flag(fw->battery_node.ring, LV_OBJ_FLAG_HIDDEN);
    } else if (fw->battery_node.ring) {
      lv_obj_add_flag(fw->battery_node.ring, LV_OBJ_FLAG_HIDDEN);
    }

    // Grid node: +to_grid on first line, -from_grid on second line
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
      lv_obj_clear_flag(fw->grid_node.ring, LV_OBJ_FLAG_HIDDEN);
    } else if (fw->grid_node.ring) {
      lv_obj_add_flag(fw->grid_node.ring, LV_OBJ_FLAG_HIDDEN);
    }

    // Battery dot: amber = charging (solar→battery), green = discharging
    if (fw->dot_bat) {
      bool charging = true;
      if (ctx->battery.available && !ctx->battery.value.empty()) {
        char *ep = nullptr;
        double v = std::strtod(ctx->battery.value.c_str(), &ep);
        charging = (ep != ctx->battery.value.c_str()) ? (v >= 0.0) : true;
      }
      lv_obj_set_style_bg_color(fw->dot_bat,
        lv_color_hex(charging ? FLOW_COLOR_SOLAR : FLOW_COLOR_BATTERY), LV_PART_MAIN);
      if (has_battery) lv_obj_clear_flag(fw->dot_bat, LV_OBJ_FLAG_HIDDEN);
      else             lv_obj_add_flag(fw->dot_bat,   LV_OBJ_FLAG_HIDDEN);
    }

    // Grid dot: yellow = exporting, red = importing
    if (fw->dot_grid) {
      bool exporting = true;
      if (ctx->from_grid.available && !ctx->from_grid.value.empty()) {
        char *ep = nullptr;
        double v = std::strtod(ctx->from_grid.value.c_str(), &ep);
        exporting = (ep == ctx->from_grid.value.c_str()) || (v < 0.1);
      }
      lv_obj_set_style_bg_color(fw->dot_grid,
        lv_color_hex(exporting ? FLOW_COLOR_EXPORT : FLOW_COLOR_IMPORT), LV_PART_MAIN);
      if (has_grid) lv_obj_clear_flag(fw->dot_grid, LV_OBJ_FLAG_HIDDEN);
      else          lv_obj_add_flag(fw->dot_grid,   LV_OBJ_FLAG_HIDDEN);
    }
  }
}
