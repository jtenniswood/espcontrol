#pragma once
#include <string>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <vector>
#include <functional>
#include "icons.h"

struct BtnSlot {
  esphome::text::Text *config;
  lv_obj_t *btn;
  lv_obj_t *icon_lbl;
  lv_obj_t *text_lbl;
  lv_obj_t *sensor_container;
  lv_obj_t *sensor_lbl;
  lv_obj_t *unit_lbl;
};

inline std::string cfg_field(const std::string &cfg, int idx) {
  size_t start = 0;
  for (int i = 0; i < idx; i++) {
    size_t pos = cfg.find(';', start);
    if (pos == std::string::npos) return "";
    start = pos + 1;
  }
  size_t end = cfg.find(';', start);
  return (end == std::string::npos) ? cfg.substr(start) : cfg.substr(start, end - start);
}

inline uint32_t parse_hex_color(const std::string &hex, bool &valid) {
  valid = hex.length() == 6;
  if (!valid) return 0;
  return strtoul(hex.c_str(), nullptr, 16);
}

inline uint32_t correct_color(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (uint8_t)(((rgb >> 8) & 0xFF) * 80 / 100);
  uint8_t b = rgb & 0xFF;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

struct OrderResult {
  int positions[25] = {};
  bool is_double[25] = {};
  bool is_wide[25] = {};
};

inline void parse_order_string(const std::string &order_str, int num_slots, OrderResult &result) {
  memset(result.positions, 0, sizeof(result.positions));
  memset(result.is_double, 0, sizeof(result.is_double));
  memset(result.is_wide, 0, sizeof(result.is_wide));
  if (order_str.empty()) return;
  size_t gpos = 0, start = 0;
  while (start <= order_str.length() && gpos < (size_t)num_slots) {
    size_t comma = order_str.find(',', start);
    if (comma == std::string::npos) comma = order_str.length();
    if (comma > start) {
      std::string token = order_str.substr(start, comma - start);
      bool dbl = !token.empty() && token.back() == 'd';
      bool wide = !token.empty() && token.back() == 'w';
      bool big = !token.empty() && token.back() == 'b';
      if (big) { dbl = true; wide = true; }
      if (dbl || wide) token.pop_back();
      int v = atoi(token.c_str());
      if (v >= 1 && v <= num_slots) {
        result.positions[gpos] = v;
        result.is_double[v - 1] = dbl;
        result.is_wide[v - 1] = wide;
      }
    }
    gpos++;
    start = comma + 1;
  }
}

inline void clear_spanned_cells(const OrderResult &order, int num_slots, int cols, OrderResult &result) {
  for (int p = 0; p < num_slots; p++) {
    result.positions[p] = order.positions[p];
    result.is_double[p] = order.is_double[p];
    result.is_wide[p] = order.is_wide[p];
  }
  for (int p = 0; p < num_slots; p++) {
    if (result.positions[p] <= 0) continue;
    int idx = result.positions[p] - 1;
    if (result.is_double[idx] && p + cols < num_slots) {
      result.positions[p + cols] = 0;
    }
    if (result.is_wide[idx] && (p + 1) % cols != 0 && p + 1 < num_slots) {
      result.positions[p + 1] = 0;
    }
    if (result.is_double[idx] && result.is_wide[idx] && (p + 1) % cols != 0 && p + cols + 1 < num_slots) {
      result.positions[p + cols + 1] = 0;
    }
  }
}

inline void apply_button_colors(lv_obj_t *btn, bool has_on, uint32_t on_val,
                                bool has_off, uint32_t off_val) {
  if (has_on) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
  }
  if (has_off) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(off_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
}

inline void setup_sensor_card(BtnSlot &s, const std::string &cfg,
                              bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  std::string unit_str = cfg_field(cfg, 5);
  if (!unit_str.empty()) {
    lv_label_set_text(s.unit_lbl, unit_str.c_str());
  }
  std::string custom_label = cfg_field(cfg, 1);
  if (!custom_label.empty()) {
    lv_label_set_text(s.text_lbl, custom_label.c_str());
  }
}

inline void setup_toggle_visual(BtnSlot &s, const std::string &cfg) {
  std::string entity_id = cfg_field(cfg, 0);
  if (!entity_id.empty()) {
    std::string custom_label = cfg_field(cfg, 1);
    if (!custom_label.empty()) {
      lv_label_set_text(s.text_lbl, custom_label.c_str());
    }
    std::string icon_name = cfg_field(cfg, 2);
    const char* icon_cp = "\U000F0493";
    if (icon_name.empty() || icon_name == "Auto") {
      std::string domain = entity_id.substr(0, entity_id.find('.'));
      icon_cp = domain_default_icon(domain);
    } else {
      icon_cp = find_icon(icon_name.c_str());
    }
    lv_label_set_text(s.icon_lbl, icon_cp);

    std::string sensor_id = cfg_field(cfg, 4);
    if (!sensor_id.empty()) {
      std::string unit_str = cfg_field(cfg, 5);
      if (!unit_str.empty()) {
        lv_label_set_text(s.unit_lbl, unit_str.c_str());
      }
    }
  } else {
    std::string custom_label = cfg_field(cfg, 1);
    if (!custom_label.empty()) {
      lv_label_set_text(s.text_lbl, custom_label.c_str());
    }
    std::string icon_name = cfg_field(cfg, 2);
    if (!icon_name.empty() && icon_name != "Auto") {
      lv_label_set_text(s.icon_lbl, find_icon(icon_name.c_str()));
    } else if (cfg_field(cfg, 6) == "push") {
      lv_label_set_text(s.icon_lbl, "\U000F0741");
      static const lv_style_prop_t push_props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
      static lv_style_transition_dsc_t push_trans;
      static bool push_trans_inited = false;
      if (!push_trans_inited) {
        lv_style_transition_dsc_init(&push_trans, push_props, lv_anim_path_ease_out, 400, 0, NULL);
        push_trans_inited = true;
      }
      lv_obj_set_style_transition(s.btn, &push_trans,
        static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
    }
  }
}

inline void subscribe_sensor_value(lv_obj_t *sensor_lbl, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(const std::string &)>([sensor_lbl](const std::string &state) {
      char *end;
      float val = strtof(state.c_str(), &end);
      if (end != state.c_str()) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", val);
        lv_label_set_text(sensor_lbl, buf);
      } else {
        lv_label_set_text(sensor_lbl, state.c_str());
      }
    })
  );
}

inline void subscribe_friendly_name(lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(const std::string &)>([text_lbl](const std::string &name) {
      lv_label_set_text(text_lbl, name.c_str());
    })
  );
}

inline void subscribe_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   lv_obj_t *sensor_ctr,
                                   bool *slot_has_sensor, bool *slot_has_icon_on,
                                   const char **slot_icon_off, const char **slot_icon_on,
                                   const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>(
      [btn_ptr, icon_lbl, sensor_ctr, slot_has_sensor, slot_has_icon_on,
       slot_icon_off, slot_icon_on](const std::string &state) {
        bool on = is_entity_on(state);
        if (on) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        if (*slot_has_icon_on) {
          lv_label_set_text(icon_lbl, on ? *slot_icon_on : *slot_icon_off);
        } else if (*slot_has_sensor) {
          if (on) {
            lv_obj_add_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          } else {
            lv_obj_clear_flag(icon_lbl, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(sensor_ctr, LV_OBJ_FLAG_HIDDEN);
          }
        }
      })
  );
}

inline void send_toggle_action(const std::string &entity_id) {
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)("homeassistant.toggle");
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(entity_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

// ── Slider card helpers ────────────────────────────────────────────────

inline bool is_cover_entity(const std::string &entity_id) {
  return entity_id.size() > 6 && entity_id.compare(0, 6, "cover.") == 0;
}

inline void send_slider_action(const std::string &entity_id, int value) {
  esphome::api::HomeassistantActionRequest req;
  req.is_event = false;
  if (value < 0) {
    req.service = decltype(req.service)("homeassistant.toggle");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)("entity_id");
    kv.value = decltype(kv.value)(entity_id.c_str());
  } else if (is_cover_entity(entity_id)) {
    req.service = decltype(req.service)("cover.set_cover_position");
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("entity_id");
    kv1.value = decltype(kv1.value)(entity_id.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)("position");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    kv2.value = decltype(kv2.value)(buf);
  } else if (value == 0) {
    req.service = decltype(req.service)("light.turn_off");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)("entity_id");
    kv.value = decltype(kv.value)(entity_id.c_str());
  } else {
    req.service = decltype(req.service)("light.turn_on");
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("entity_id");
    kv1.value = decltype(kv1.value)(entity_id.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)("brightness_pct");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    kv2.value = decltype(kv2.value)(buf);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

struct SliderCtx {
  std::string entity_id;
  lv_obj_t *fill;
  bool horizontal;
  bool inverted;
  lv_coord_t radius;
};

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
  lv_obj_clear_flag(fill, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *slider = lv_slider_create(btn);
  lv_slider_set_range(slider, 0, 100);
  lv_slider_set_value(slider, 0, LV_ANIM_OFF);
  lv_obj_update_layout(btn);
  lv_coord_t bw = lv_obj_get_width(btn);
  lv_coord_t bh = lv_obj_get_height(btn);
  if (horizontal) {
    lv_obj_set_size(slider, bw, bh >= bw ? bw - 1 : bh);
  } else {
    lv_obj_set_size(slider, bw >= bh ? bh - 1 : bw, bh);
  }
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 0);

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

inline void setup_slider_visual(BtnSlot &s, const std::string &cfg, uint32_t on_color) {
  setup_toggle_visual(s, cfg);

  bool horizontal = cfg_field(cfg, 4) == "h";
  lv_obj_t *slider = setup_slider_widget(s.btn, on_color, horizontal);
  lv_coord_t pad = lv_obj_get_style_radius(s.btn, LV_PART_MAIN) + 4;
  lv_obj_align(s.icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(s.text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  lv_obj_set_user_data(s.sensor_container, (void *)slider);

  lv_obj_t *fill = lv_obj_get_child(s.btn, 0);
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = cfg_field(cfg, 0);
  ctx->fill = fill;
  ctx->horizontal = horizontal;
  ctx->inverted = cfg_field(cfg, 7) == "1";
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  lv_obj_set_user_data(slider, (void *)ctx);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    slider_update_fill(c->fill, lv_obj_get_parent(sl), val, c->horizontal, c->inverted, c->radius);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = lv_event_get_target(e);
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty()) {
      int val = lv_slider_get_value(sl);
      send_slider_action(c->entity_id, c->inverted ? 100 - val : val);
    }
  }, LV_EVENT_RELEASED, nullptr);
}

inline void subscribe_slider_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                  lv_obj_t *slider,
                                  bool has_icon_on,
                                  const char *icon_off, const char *icon_on,
                                  const std::string &entity_id) {
  SliderCtx *sctx = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *fill = sctx ? sctx->fill : nullptr;
  bool horiz = sctx ? sctx->horizontal : false;
  bool inv = sctx ? sctx->inverted : false;
  lv_coord_t rad = sctx ? sctx->radius : 0;
  bool is_cover = is_cover_entity(entity_id);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>(
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](const std::string &state) {
        bool on = is_entity_on(state);
        if (!on) {
          int z = inv ? 100 : 0;
          lv_slider_set_value(slider, z, LV_ANIM_OFF);
          if (fill) slider_update_fill(fill, btn_ptr, z, horiz, inv, rad);
        }
        if (has_icon_on) {
          lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
        }
      })
  );
  if (is_cover) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("current_position"),
      std::function<void(const std::string &)>(
        [slider, btn_ptr, fill, horiz, inv, rad](const std::string &val) {
          char *end;
          float pos = strtof(val.c_str(), &end);
          if (end != val.c_str()) {
            int pct = (int)(pos + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            if (inv) pct = 100 - pct;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            if (fill) slider_update_fill(fill, btn_ptr, pct, horiz, inv, rad);
          }
        })
    );
  } else {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("brightness"),
      std::function<void(const std::string &)>(
        [slider, btn_ptr, fill, horiz, inv, rad](const std::string &val) {
          char *end;
          float bri = strtof(val.c_str(), &end);
          if (end != val.c_str()) {
            int pct = (int)((bri * 100.0f + 127.0f) / 255.0f);
            if (pct < 1) pct = 1;
            if (pct > 100) pct = 100;
            if (inv) pct = 100 - pct;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            if (fill) slider_update_fill(fill, btn_ptr, pct, horiz, inv, rad);
          }
        })
    );
  }
}

struct SubpageBtn {
  std::string entity;
  std::string label;
  std::string icon;
  std::string icon_on;
  std::string sensor;
  std::string unit;
  std::string type;
  std::string invert;
  std::string off_color;
  std::string on_color;
};

inline std::vector<SubpageBtn> parse_subpage_config(const std::string &sp_cfg) {
  std::vector<SubpageBtn> btns;
  if (sp_cfg.empty()) return btns;

  std::vector<std::string> pipes;
  size_t ps = 0;
  while (ps <= sp_cfg.length()) {
    size_t pe = sp_cfg.find('|', ps);
    if (pe == std::string::npos) pe = sp_cfg.length();
    pipes.push_back(sp_cfg.substr(ps, pe - ps));
    ps = pe + 1;
  }
  if (pipes.size() < 2) return btns;

  for (size_t pi = 1; pi < pipes.size(); pi++) {
    std::vector<std::string> flds;
    size_t fs = 0;
    while (fs <= pipes[pi].length()) {
      size_t fe = pipes[pi].find(':', fs);
      if (fe == std::string::npos) fe = pipes[pi].length();
      flds.push_back(pipes[pi].substr(fs, fe - fs));
      fs = fe + 1;
    }
    std::string e = flds.size() > 0 ? flds[0] : "";
    std::string l = flds.size() > 1 ? flds[1] : "";
    std::string ic = flds.size() > 2 ? flds[2] : "Auto";
    if (ic.empty()) ic = "Auto";
    std::string io = flds.size() > 3 ? flds[3] : "Auto";
    if (io.empty()) io = "Auto";
    std::string sn = flds.size() > 4 ? flds[4] : "";
    std::string un = flds.size() > 5 ? flds[5] : "";
    std::string tp = flds.size() > 6 ? flds[6] : "";
    std::string inv = flds.size() > 7 ? flds[7] : "";
    std::string off = flds.size() > 8 ? flds[8] : "";
    std::string on = flds.size() > 9 ? flds[9] : "";
    btns.push_back({e, l, ic, io, sn, un, tp, inv, off, on});
  }
  return btns;
}

inline std::string get_subpage_order(const std::string &sp_cfg) {
  if (sp_cfg.empty()) return "";
  size_t pe = sp_cfg.find('|');
  if (pe == std::string::npos) return sp_cfg;
  return sp_cfg.substr(0, pe);
}

struct SubpageOrder {
  int positions[25] = {};
  bool is_double[25] = {};
  bool is_wide[25] = {};
  int back_pos = 0;
  bool back_dbl = false;
  bool back_wide = false;
  bool has_back_token = false;
};

inline void parse_subpage_order(const std::string &order_str, int num_slots, int num_btns,
                                SubpageOrder &result) {
  if (order_str.empty()) return;
  size_t gp2 = 0, st2 = 0;
  while (st2 <= order_str.length() && gp2 < (size_t)num_slots) {
    size_t cm = order_str.find(',', st2);
    if (cm == std::string::npos) cm = order_str.length();
    if (cm > st2) {
      std::string tk = order_str.substr(st2, cm - st2);
      if (tk == "B" || tk == "Bd" || tk == "Bw" || tk == "Bb") {
        result.back_pos = gp2;
        result.back_dbl = (tk == "Bd" || tk == "Bb");
        result.back_wide = (tk == "Bw" || tk == "Bb");
        result.has_back_token = true;
      } else {
        bool d = !tk.empty() && tk.back() == 'd';
        bool w = !tk.empty() && tk.back() == 'w';
        bool bg = !tk.empty() && tk.back() == 'b';
        if (bg) { d = true; w = true; }
        if (d || w) tk.pop_back();
        int v = atoi(tk.c_str());
        if (v >= 1 && v <= num_btns) {
          result.positions[gp2] = v;
          result.is_double[v - 1] = d;
          result.is_wide[v - 1] = w;
        }
      }
    }
    gp2++;
    st2 = cm + 1;
  }
}
