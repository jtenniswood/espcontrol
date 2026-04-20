// =============================================================================
// BUTTON GRID - LVGL button grid layout, parsing, and HA entity binding
// =============================================================================
// Shared C++ utilities included by each device's sensors.yaml lambda. Handles:
//   - Parsing semicolon-delimited button config strings into structured fields
//   - Grid layout with double-height (d), double-wide (w), and 2×2 big (b) cells
//   - LVGL visual setup for toggle buttons, sensor cards, and slider widgets
//   - Home Assistant state subscriptions and action dispatch
//   - Subpage creation (nested grid screens with back button)
// =============================================================================
#pragma once
#include <string>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdint>
#include <vector>
#include <functional>
#include "icons.h"
#include "backlight.h"

constexpr uint32_t DEFAULT_SLIDER_COLOR = 0xFF8C00;

// LVGL widget handles for one button slot on the main grid
struct BtnSlot {
  esphome::text::Text *config;       // persisted config string (entity;label;icon;...)
  lv_obj_t *btn;                     // button container
  lv_obj_t *icon_lbl;               // icon label (MDI glyph)
  lv_obj_t *text_lbl;               // entity name / custom label
  lv_obj_t *sensor_container;       // flex row shown when sensor overlay is active
  lv_obj_t *sensor_lbl;             // numeric sensor value
  lv_obj_t *unit_lbl;               // unit suffix (°C, %, etc.)
};

// Extract the Nth semicolon-delimited field from a config string
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

// Structured view of a button config string: entity;label;icon;icon_on;sensor;unit;type;precision
struct ParsedCfg {
  std::string entity;      // 0  HA entity_id (e.g. light.kitchen)
  std::string label;       // 1  display name (blank = use HA friendly_name)
  std::string icon;        // 2  icon name for off/default state
  std::string icon_on;     // 3  icon name for on state (blank = no swap)
  std::string sensor;      // 4  sensor entity for toggle overlay; "h" for horizontal slider
  std::string unit;        // 5  unit suffix for sensor display
  std::string type;        // 6  button type: "" (toggle), sensor, slider, cover, push, subpage
  std::string precision;   // 7  decimal places for sensors; "text" = text sensor mode
};

inline ParsedCfg parse_cfg(const std::string &cfg) {
  ParsedCfg p;
  p.entity    = cfg_field(cfg, 0);
  p.label     = cfg_field(cfg, 1);
  p.icon      = cfg_field(cfg, 2);
  p.icon_on   = cfg_field(cfg, 3);
  p.sensor    = cfg_field(cfg, 4);
  p.unit      = cfg_field(cfg, 5);
  p.type      = cfg_field(cfg, 6);
  p.precision = cfg_field(cfg, 7);
  return p;
}

inline int parse_precision(const std::string &s) {
  if (s.empty()) return 0;
  int v = atoi(s.c_str());
  return (v < 0) ? 0 : (v > 3) ? 3 : v;
}

inline bool is_text_sensor_card(const std::string &type, const std::string &precision) {
  return (type == "sensor" && precision == "text") || type == "text_sensor";
}

inline bool is_text_sensor_card(const ParsedCfg &p) {
  return is_text_sensor_card(p.type, p.precision);
}

inline std::string sentence_cap_text(const std::string &state) {
  std::string out;
  out.reserve(state.size());
  bool cap_next = true;
  bool last_space = false;
  for (char ch : state) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (ch == '_' || ch == '-' || std::isspace(c)) {
      if (!out.empty() && !last_space) {
        out.push_back(' ');
        last_space = true;
      }
      continue;
    }
    if (std::isalpha(c)) {
      out.push_back(static_cast<char>(cap_next ? std::toupper(c) : std::tolower(c)));
      cap_next = false;
    } else {
      out.push_back(ch);
    }
    last_space = false;
  }
  if (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

inline const char* weather_icon_for_state(const std::string &state) {
  if (state == "sunny") return find_icon("Weather Sunny");
  if (state == "clear-night") return find_icon("Weather Night");
  if (state == "partlycloudy") return find_icon("Weather Partly Cloudy");
  if (state == "cloudy") return find_icon("Weather Cloudy");
  if (state == "fog") return find_icon("Weather Fog");
  if (state == "hail") return find_icon("Weather Hail");
  if (state == "lightning") return find_icon("Weather Lightning");
  if (state == "lightning-rainy") return find_icon("Weather Lightning Rainy");
  if (state == "pouring") return find_icon("Weather Pouring");
  if (state == "rainy") return find_icon("Weather Rainy");
  if (state == "snowy") return find_icon("Weather Snowy");
  if (state == "snowy-rainy") return find_icon("Weather Snowy Rainy");
  if (state == "windy") return find_icon("Weather Windy");
  if (state == "windy-variant") return find_icon("Weather Windy Variant");
  if (state == "unavailable" || state.empty()) return find_icon("Weather Sunny Off");
  return find_icon("Weather Cloudy Alert");
}

inline std::string weather_label_for_state(const std::string &state) {
  if (state == "sunny") return "Sunny";
  if (state == "clear-night") return "Clear night";
  if (state == "partlycloudy") return "Partly cloudy";
  if (state == "cloudy") return "Cloudy";
  if (state == "fog") return "Fog";
  if (state == "hail") return "Hail";
  if (state == "lightning") return "Lightning";
  if (state == "lightning-rainy") return "Lightning and rain";
  if (state == "pouring") return "Pouring";
  if (state == "rainy") return "Rainy";
  if (state == "snowy") return "Snowy";
  if (state == "snowy-rainy") return "Snowy and rain";
  if (state == "windy") return "Windy";
  if (state == "windy-variant") return "Windy and cloudy";
  if (state == "exceptional") return "Exceptional";
  if (state == "unknown") return "Unknown";
  if (state == "unavailable" || state.empty()) return "Unavailable";

  std::string label = state;
  for (size_t i = 0; i < label.length(); i++) {
    if (label[i] == '-' || label[i] == '_') label[i] = ' ';
  }
  if (!label.empty()) {
    label[0] = static_cast<char>(toupper(static_cast<unsigned char>(label[0])));
  }
  return label;
}

// Parse a 6-char hex color string (no # prefix) into a uint32_t RGB value
inline uint32_t parse_hex_color(const std::string &hex, bool &valid) {
  valid = hex.length() == 6;
  if (!valid) return 0;
  return strtoul(hex.c_str(), nullptr, 16);
}

// Reduce green channel to 80% to compensate for display panel color shift
inline uint32_t correct_color(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (uint8_t)(((rgb >> 8) & 0xFF) * 80 / 100);
  uint8_t b = rgb & 0xFF;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

// ── Grid layout parsing ───────────────────────────────────────────────

// Result of parsing a button_order CSV string into grid cell positions
struct OrderResult {
  int positions[25] = {};    // slot number at each grid position (1-based, 0=empty)
  bool is_double[25] = {};   // slot uses double height (suffix "d" or "b")
  bool is_wide[25] = {};     // slot uses double width (suffix "w" or "b")
};

// Parse "1,2d,3w,4b,..." into positions + double/wide flags
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

// Zero out grid cells that are covered by a neighbouring double/wide/big button
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

// ── Button visuals ────────────────────────────────────────────────────

// Apply on/off background colors to a button's checked/pressed/default states
inline void apply_button_colors(lv_obj_t *btn, bool has_on, uint32_t on_val,
                                bool has_off, uint32_t off_val) {
  if (has_on) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
    lv_obj_set_style_bg_color(btn, lv_color_hex(on_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  }
  if (has_off) {
    lv_obj_set_style_bg_color(btn, lv_color_hex(off_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
}

// Configure a button as a read-only sensor card (non-clickable, shows value + unit)
inline void setup_sensor_card(BtnSlot &s, const ParsedCfg &p,
                              bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  if (!p.unit.empty()) {
    lv_label_set_text(s.unit_lbl, p.unit.c_str());
  }
  if (!p.label.empty()) {
    lv_label_set_text(s.text_lbl, p.label.c_str());
  }
}

inline void setup_weather_card(BtnSlot &s, bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(s.icon_lbl, find_icon("Weather Cloudy"));
  lv_label_set_text(s.text_lbl, "Weather");
}

// Set icon and label on a toggle/push button based on its config
inline void setup_toggle_visual(BtnSlot &s, const ParsedCfg &p) {
  if (!p.entity.empty()) {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      std::string domain = p.entity.substr(0, p.entity.find('.'));
      icon_cp = domain_default_icon(domain);
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    lv_label_set_text(s.icon_lbl, icon_cp);

    if (!p.sensor.empty()) {
      if (!p.unit.empty()) {
        lv_label_set_text(s.unit_lbl, p.unit.c_str());
      }
    }
  } else {
    if (!p.label.empty()) {
      lv_label_set_text(s.text_lbl, p.label.c_str());
    }
    if (!p.icon.empty() && p.icon != "Auto") {
      lv_label_set_text(s.icon_lbl, find_icon(p.icon.c_str()));
    } else if (p.type == "push") {
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

inline void setup_text_sensor_card(BtnSlot &s, const ParsedCfg &p,
                                   bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  setup_toggle_visual(s, p);
  lv_obj_clear_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_label_set_text(s.text_lbl, "--");
}

// ── Home Assistant subscriptions ──────────────────────────────────────

// Subscribe to a HA sensor entity and update an LVGL label with its value
inline void subscribe_sensor_value(lv_obj_t *sensor_lbl, const std::string &sensor_id, int precision = 0) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(const std::string &)>([sensor_lbl, precision](const std::string &state) {
      char *end;
      float val = strtof(state.c_str(), &end);
      if (end != state.c_str()) {
        char fmt[8];
        snprintf(fmt, sizeof(fmt), "%%.%df", precision);
        char buf[16];
        snprintf(buf, sizeof(buf), fmt, val);
        lv_label_set_text(sensor_lbl, buf);
      } else {
        lv_label_set_text(sensor_lbl, state.c_str());
      }
    })
  );
}

inline void subscribe_text_sensor_value(lv_obj_t *text_lbl, const std::string &sensor_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    sensor_id, {},
    std::function<void(const std::string &)>([text_lbl](const std::string &state) {
      std::string text = sentence_cap_text(state);
      lv_label_set_text(text_lbl, text.c_str());
    })
  );
}

inline void subscribe_weather_state(lv_obj_t *icon_lbl, lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>([icon_lbl, text_lbl](const std::string &state) {
      lv_label_set_text(icon_lbl, weather_icon_for_state(state));
      lv_label_set_text(text_lbl, weather_label_for_state(state).c_str());
    })
  );
}

// Subscribe to an entity's friendly_name attribute and use it as the button label
inline void subscribe_friendly_name(lv_obj_t *text_lbl, const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(const std::string &)>([text_lbl](const std::string &name) {
      lv_label_set_text(text_lbl, name.c_str());
    })
  );
}

// Subscribe to a toggle entity's state; updates checked visual, icon swap, sensor overlay
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

// ── Home Assistant actions ────────────────────────────────────────────

// Send homeassistant.toggle for a given entity_id
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

// Send HA action for a slider change: toggle (value<0), brightness, or cover position
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

// ── Button click dispatch ─────────────────────────────────────────────

// Handle a main-grid button press: dispatch push event, subpage nav,
// slider toggle, or entity toggle based on the config string.
inline void handle_button_click(const std::string &cfg, int slot_num,
                                lv_obj_t *btn_obj) {
  ParsedCfg p = parse_cfg(cfg);
  if (p.type == "sensor" || p.type == "text_sensor") return;
  if (p.type == "push") {
    std::string label = p.label;
    if (label.empty()) {
      char buf[16];
      snprintf(buf, sizeof(buf), "Push %d", slot_num);
      label = buf;
    }
    esphome::api::HomeassistantActionRequest req;
    req.service = decltype(req.service)("esphome.push_button_pressed");
    req.is_event = true;
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("label");
    kv1.value = decltype(kv1.value)(label.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)("slot");
    char slot_buf[8];
    snprintf(slot_buf, sizeof(slot_buf), "%d", slot_num);
    kv2.value = decltype(kv2.value)(slot_buf);
    esphome::api::global_api_server->send_homeassistant_action(req);
  } else if (p.type == "subpage") {
    lv_obj_t *sub_scr = (lv_obj_t *)lv_obj_get_user_data(btn_obj);
    if (sub_scr)
      lv_scr_load_anim(sub_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  } else if (p.type == "slider" || p.type == "cover") {
    if (!p.entity.empty()) send_slider_action(p.entity, -1);
  } else {
    if (!p.entity.empty()) send_toggle_action(p.entity);
  }
}

// ── Slider widgets ───────────────────────────────────────────────────

// Context attached to each LVGL slider via user_data
struct SliderCtx {
  std::string entity_id;
  lv_obj_t *fill;
  bool horizontal;
  bool inverted;
  lv_coord_t radius;
};

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

// Full slider button setup: visual + event handlers + HA action on release
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);

  bool horizontal = p.sensor == "h";
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
  ctx->inverted = is_cover_entity(p.entity);
  ctx->radius = lv_obj_get_style_radius(s.btn, LV_PART_MAIN);
  lv_obj_set_user_data(slider, (void *)ctx);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (!c) return;
    int val = lv_slider_get_value(sl);
    int fill_val = c->inverted ? 100 - val : val;
    slider_update_fill(c->fill, lv_obj_get_parent(sl), fill_val, c->horizontal, c->inverted, c->radius);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty()) {
      int val = lv_slider_get_value(sl);
      send_slider_action(c->entity_id, val);
    }
  }, LV_EVENT_RELEASED, nullptr);
}

// Subscribe to HA state for a slider entity (light brightness or cover position)
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
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on, is_cover](const std::string &state) {
        bool on = is_entity_on(state);
        if (!on) {
          lv_slider_set_value(slider, 0, LV_ANIM_OFF);
          if (fill) slider_update_fill(fill, btn_ptr, inv ? 100 : 0, horiz, inv, rad);
        }
        if (has_icon_on && !is_cover) {
          lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
        }
      })
  );
  if (is_cover) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("current_position"),
      std::function<void(const std::string &)>(
        [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](const std::string &val) {
          char *end;
          float pos = strtof(val.c_str(), &end);
          if (end != val.c_str()) {
            int pct = (int)(pos + 0.5f);
            if (pct < 0) pct = 0;
            if (pct > 100) pct = 100;
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
            if (has_icon_on) {
              lv_label_set_text(icon_lbl, pct < 100 ? icon_on : icon_off);
            }
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
            lv_slider_set_value(slider, pct, LV_ANIM_OFF);
            int fill_pct = inv ? 100 - pct : pct;
            if (fill) slider_update_fill(fill, btn_ptr, fill_pct, horiz, inv, rad);
          }
        })
    );
  }
}

// ── Subpage helpers ───────────────────────────────────────────────────

// Button definition parsed from a subpage config (pipe+colon delimited)
struct SubpageBtn {
  std::string entity;
  std::string label;
  std::string icon;
  std::string icon_on;
  std::string sensor;     // sensor entity for toggle; orientation "h"|"" for slider/cover
  std::string unit;
  std::string type;
  std::string precision;  // decimal places for sensor display; "text" = text sensor mode
};

// Create a slider button inside a subpage screen (reuses main grid slider logic)
inline lv_obj_t *setup_subpage_slider(lv_obj_t *btn, lv_obj_t *icon_lbl, lv_obj_t *text_lbl,
                                       const SubpageBtn &sb, uint32_t on_color, lv_coord_t radius) {
  if (!sb.label.empty()) lv_label_set_text(text_lbl, sb.label.c_str());
  else subscribe_friendly_name(text_lbl, sb.entity);

  bool horiz = sb.sensor == "h";
  lv_obj_t *sl = setup_slider_widget(btn, on_color, horiz);
  lv_coord_t pad = radius + 4;
  lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);

  lv_obj_t *fill = lv_obj_get_child(btn, 0);
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = sb.entity;
  ctx->fill = fill;
  ctx->horizontal = horiz;
  ctx->inverted = is_cover_entity(sb.entity);
  ctx->radius = radius;
  lv_obj_set_user_data(sl, (void *)ctx);

  lv_obj_add_event_cb(sl, [](lv_event_t *e) {
    lv_obj_t *s = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(s);
    if (!c) return;
    int val = lv_slider_get_value(s);
    int fv = c->inverted ? 100 - val : val;
    slider_update_fill(c->fill, lv_obj_get_parent(s), fv, c->horizontal, c->inverted, c->radius);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(sl, [](lv_event_t *e) {
    lv_obj_t *s = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(s);
    if (c && !c->entity_id.empty())
      send_slider_action(c->entity_id, lv_slider_get_value(s));
  }, LV_EVENT_RELEASED, nullptr);

  bool has_icon_on = !sb.icon_on.empty() && sb.icon_on != "Auto";
  const char *sl_icon_on = has_icon_on ? find_icon(sb.icon_on.c_str()) : nullptr;
  const char *sl_icon_off = nullptr;
  if (has_icon_on) {
    sl_icon_off = (sb.icon.empty() || sb.icon == "Auto")
      ? domain_default_icon(sb.entity.substr(0, sb.entity.find('.')))
      : find_icon(sb.icon.c_str());
  }
  subscribe_slider_state(btn, icon_lbl, sl, has_icon_on, sl_icon_off, sl_icon_on, sb.entity);

  // Intentionally leaked -- lives for the lifetime of the display
  std::string *eid = new std::string(sb.entity);
  lv_obj_add_event_cb(btn, [](lv_event_t *e) {
    std::string *en = (std::string *)lv_event_get_user_data(e);
    if (en && !en->empty()) send_slider_action(*en, -1);
  }, LV_EVENT_CLICKED, eid);

  return sl;
}

// Parse "order|entity:label:icon:...|entity:label:..." into a vector of SubpageBtns
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
    std::string pr = flds.size() > 7 ? flds[7] : "";
    btns.push_back({e, l, ic, io, sn, un, tp, pr});
  }
  return btns;
}

// Extract the order string (everything before the first pipe) from subpage config
inline std::string get_subpage_order(const std::string &sp_cfg) {
  if (sp_cfg.empty()) return "";
  size_t pe = sp_cfg.find('|');
  if (pe == std::string::npos) return sp_cfg;
  return sp_cfg.substr(0, pe);
}

// Subpage grid layout with support for a back button token ("B")
struct SubpageOrder {
  int positions[25] = {};
  bool is_double[25] = {};
  bool is_wide[25] = {};
  int back_pos = 0;
  bool back_dbl = false;
  bool back_wide = false;
  bool has_back_token = false;
};

// Parse subpage order CSV; "B"/"Bd"/"Bw"/"Bb" tokens mark the back button position
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

// =============================================================================
// GRID BOOT PHASES - Consolidated on_boot logic for all devices
// =============================================================================
// Each sensors.yaml builds id()-based arrays and calls these three functions.
// Device-specific behavior is controlled by GridConfig fields.
// =============================================================================

struct GridConfig {
  int num_slots;
  int cols;
  bool color_correction;
  bool wrap_tall_labels;
  const lv_font_t *sp_sensor_font;
};

// ── Phase 1: Visual setup ────────────────────────────────────────────

inline void grid_phase1(
    BtnSlot *slots, const GridConfig &cfg,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex) {
  ESP_LOGI("sensors", "Phase 1: visual setup start (%lu ms)", esphome::millis());
  int NS = cfg.num_slots;
  int COLS = cfg.cols;

  if (!order_str.empty()) {
    bool all_empty = true;
    for (int i = 0; i < NS; i++) {
      if (!slots[i].config->state.empty()) { all_empty = false; break; }
    }
    if (all_empty) {
      ESP_LOGW("sensors", "Button order is set but all configs are empty. "
        "If upgrading from the old per-field format, export your config "
        "from the old firmware's web UI and import it after upgrading.");
    }
  }

  OrderResult parsed, order;
  parse_order_string(order_str, NS, parsed);
  clear_spanned_cells(parsed, NS, COLS, order);

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  for (int i = 0; i < NS; i++)
    lv_obj_add_flag(slots[i].btn, LV_OBJ_FLAG_HIDDEN);

  for (int pos = 0; pos < NS; pos++) {
    int idx = order.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;
    lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_HIDDEN);
    int col = pos % COLS, row = pos / COLS;
    int row_span = order.is_double[idx - 1] ? 2 : 1;
    int col_span = order.is_wide[idx - 1] ? 2 : 1;
    lv_obj_set_grid_cell(s.btn,
      LV_GRID_ALIGN_STRETCH, col, col_span,
      LV_GRID_ALIGN_STRETCH, row, row_span);

    if (cfg.wrap_tall_labels && row_span > 1) {
      lv_label_set_long_mode(s.text_lbl, LV_LABEL_LONG_WRAP);
      lv_obj_set_width(s.text_lbl, lv_pct(100));
    }

    apply_button_colors(s.btn, has_on, on_val, has_off, off_val);

    ParsedCfg p = parse_cfg(scfg);
    if (is_text_sensor_card(p)) {
      setup_text_sensor_card(s, p, has_sensor_color, sensor_val);
      continue;
    }
    if (p.type == "sensor") {
      if (p.sensor.empty()) continue;
      setup_sensor_card(s, p, has_sensor_color, sensor_val);
      continue;
    }
    if (p.type == "weather") {
      setup_weather_card(s, has_sensor_color, sensor_val);
      continue;
    }
    if (p.type == "slider" || p.type == "cover") {
      setup_slider_visual(s, p, has_on ? on_val : DEFAULT_SLIDER_COLOR);
    } else {
      setup_toggle_visual(s, p);
    }
  }
  ESP_LOGI("sensors", "Phase 1: done (%lu ms)", esphome::millis());
}

// ── Phase 2: HA subscriptions + subpage creation ─────────────────────

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  ESP_LOGI("sensors", "Phase 2: subscriptions + subpages start (%lu ms)", esphome::millis());
  int NS = cfg.num_slots;
  int COLS = cfg.cols;
  int ROWS = (NS + COLS - 1) / COLS;

  static bool has_sensor[25] = {};
  static bool has_icon_on[25] = {};
  static const char* icon_off_cp[25] = {};
  static const char* icon_on_cp[25] = {};

  static bool sp_child_was_on[25 * 25] = {};
  static std::string sp_entity_ids[25 * 25];
  static int sp_alloc_idx = 0;
  sp_alloc_idx = 0;

  bool has_on, has_off, has_sensor_color;
  uint32_t on_val = parse_hex_color(on_hex, has_on);
  uint32_t off_val = parse_hex_color(off_hex, has_off);
  uint32_t sensor_val = parse_hex_color(sensor_hex, has_sensor_color);

  if (cfg.color_correction) {
    if (has_on) on_val = correct_color(on_val);
    if (has_off) off_val = correct_color(off_val);
    if (has_sensor_color) sensor_val = correct_color(sensor_val);
  }

  OrderResult parsed;
  parse_order_string(order_str, NS, parsed);

  for (int pos = 0; pos < NS; pos++) {
    int idx = parsed.positions[pos];
    if (idx < 1 || idx > NS) continue;
    auto &s = slots[idx - 1];
    std::string scfg = s.config->state;

    ParsedCfg p = parse_cfg(scfg);
    if (is_text_sensor_card(p)) {
      if (!p.sensor.empty())
        subscribe_text_sensor_value(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "sensor") {
      if (p.sensor.empty()) continue;
      subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision));
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.sensor);
      continue;
    }
    if (p.type == "weather") {
      if (!p.entity.empty())
        subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity);
      continue;
    }

    if (p.entity.empty()) continue;

    if (p.type == "slider" || p.type == "cover") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      bool sl_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
      const char *sl_icon_on_cp = sl_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
      const char *sl_icon_off_cp = nullptr;
      if (sl_has_icon_on) {
        sl_icon_off_cp = (p.icon.empty() || p.icon == "Auto")
          ? domain_default_icon(p.entity.substr(0, p.entity.find('.')))
          : find_icon(p.icon.c_str());
      }
      subscribe_slider_state(s.btn, s.icon_lbl, slider,
        sl_has_icon_on, sl_icon_off_cp, sl_icon_on_cp, p.entity);
      if (p.label.empty())
        subscribe_friendly_name(s.text_lbl, p.entity);
      continue;
    }

    has_sensor[idx - 1] = !p.sensor.empty();

    has_icon_on[idx - 1] = !p.icon_on.empty() && p.icon_on != "Auto";
    if (has_icon_on[idx - 1])
      icon_on_cp[idx - 1] = find_icon(p.icon_on.c_str());

    const char* icon_cp = "\U000F0493";
    if (p.icon.empty() || p.icon == "Auto") {
      icon_cp = domain_default_icon(p.entity.substr(0, p.entity.find('.')));
    } else {
      icon_cp = find_icon(p.icon.c_str());
    }
    icon_off_cp[idx - 1] = icon_cp;

    if (p.label.empty())
      subscribe_friendly_name(s.text_lbl, p.entity);

    subscribe_toggle_state(s.btn, s.icon_lbl, s.sensor_container,
      &has_sensor[idx - 1], &has_icon_on[idx - 1],
      &icon_off_cp[idx - 1], &icon_on_cp[idx - 1], p.entity);

    if (has_sensor[idx - 1])
      subscribe_sensor_value(s.sensor_lbl, p.sensor, parse_precision(p.precision));
  }

  // --- Subpage creation ---
  // Heap-allocated grid descriptors (never freed -- display lifetime)
  lv_coord_t *sp_col_dsc = new lv_coord_t[COLS + 1];
  for (int i = 0; i < COLS; i++) sp_col_dsc[i] = LV_GRID_FR(1);
  sp_col_dsc[COLS] = LV_GRID_TEMPLATE_LAST;
  lv_coord_t *sp_row_dsc = new lv_coord_t[ROWS + 1];
  for (int i = 0; i < ROWS; i++) sp_row_dsc[i] = LV_GRID_FR(1);
  sp_row_dsc[ROWS] = LV_GRID_TEMPLATE_LAST;

  const lv_font_t *sp_icon_fnt = lv_obj_get_style_text_font(slots[0].icon_lbl, LV_PART_MAIN);

  lv_obj_t *ref_btn = slots[0].btn;
  for (int i = 0; i < NS; i++) {
    ParsedCfg pc = parse_cfg(slots[i].config->state);
    if (pc.type != "slider" && pc.type != "cover") {
      ref_btn = slots[i].btn;
      break;
    }
  }
  lv_coord_t sp_radius = lv_obj_get_style_radius(ref_btn, LV_PART_MAIN);
  lv_coord_t sp_pad = lv_obj_get_style_pad_top(ref_btn, LV_PART_MAIN);
  const lv_font_t *sp_btn_fnt = lv_obj_get_style_text_font(ref_btn, LV_PART_MAIN);
  lv_color_t sp_txt_color = lv_obj_get_style_text_color(ref_btn, LV_PART_MAIN);

  lv_coord_t mp_pad_top = lv_obj_get_style_pad_top(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_bottom = lv_obj_get_style_pad_bottom(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_left = lv_obj_get_style_pad_left(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_right = lv_obj_get_style_pad_right(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_row = lv_obj_get_style_pad_row(main_page_obj, LV_PART_MAIN);
  lv_coord_t mp_pad_col = lv_obj_get_style_pad_column(main_page_obj, LV_PART_MAIN);

  static int sp_on_count[25] = {};

  for (int si = 0; si < NS; si++) {
    ParsedCfg p = parse_cfg(slots[si].config->state);
    if (p.type != "subpage") continue;
    bool sp_indicator = p.sensor == "indicator";

    bool sp_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
    const char* sp_icon_on_glyph = sp_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
    const char* sp_icon_off_glyph = nullptr;
    if (sp_has_icon_on) {
      sp_icon_off_glyph = (p.icon.empty() || p.icon == "Auto")
        ? "\U000F024B" : find_icon(p.icon.c_str());
    }

    std::string sp_cfg = sp_configs[si]->state + sp_ext_configs[si]->state;
    if (sp_cfg.empty()) continue;

    auto sp_btns = parse_subpage_config(sp_cfg);
    if (sp_btns.empty()) continue;
    std::string sp_order_str = get_subpage_order(sp_cfg);

    SubpageOrder sp_ord;
    parse_subpage_order(sp_order_str, NS, sp_btns.size(), sp_ord);

    lv_obj_t *sub_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(sub_scr, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sub_scr, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_layout(sub_scr, LV_LAYOUT_GRID);
    lv_obj_set_grid_dsc_array(sub_scr, sp_col_dsc, sp_row_dsc);
    lv_obj_set_style_pad_top(sub_scr, mp_pad_top, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(sub_scr, mp_pad_bottom, LV_PART_MAIN);
    lv_obj_set_style_pad_left(sub_scr, mp_pad_left, LV_PART_MAIN);
    lv_obj_set_style_pad_right(sub_scr, mp_pad_right, LV_PART_MAIN);
    lv_obj_set_style_pad_row(sub_scr, mp_pad_row, LV_PART_MAIN);
    lv_obj_set_style_pad_column(sub_scr, mp_pad_col, LV_PART_MAIN);
    lv_obj_clear_flag(sub_scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *back_btn = lv_btn_create(sub_scr);
    lv_obj_set_style_radius(back_btn, sp_radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(back_btn, sp_pad, LV_PART_MAIN);
    lv_obj_set_style_text_font(back_btn, sp_btn_fnt, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_btn, sp_txt_color, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(back_btn, 0, LV_PART_MAIN);
    if (has_off) lv_obj_set_style_bg_color(back_btn, lv_color_hex(off_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
    lv_obj_set_grid_cell(back_btn, LV_GRID_ALIGN_STRETCH, sp_ord.back_pos % COLS, sp_ord.back_wide ? 2 : 1,
      LV_GRID_ALIGN_STRETCH, sp_ord.back_pos / COLS, sp_ord.back_dbl ? 2 : 1);
    lv_obj_t *bi = lv_label_create(back_btn);
    lv_obj_set_style_text_font(bi, sp_icon_fnt, LV_PART_MAIN);
    lv_label_set_text(bi, "\U000F004D");
    lv_obj_align(bi, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_t *bl = lv_label_create(back_btn);
    lv_label_set_text(bl, "Back");
    lv_obj_align(bl, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
      lv_scr_load_anim((lv_obj_t *)lv_event_get_user_data(e), LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, main_page_obj);

    for (int gp = 0; gp < NS; gp++) {
      int bn = sp_ord.positions[gp];
      if (bn < 1 || bn > (int)sp_btns.size()) continue;
      auto &sb = sp_btns[bn - 1];
      int col, row;
      if (sp_ord.has_back_token) { col = gp % COLS; row = gp / COLS; }
      else { int op = gp + 1; col = op % COLS; row = op / COLS; }
      int rs = sp_ord.is_double[bn - 1] ? 2 : 1;

      lv_obj_t *sb_btn = lv_btn_create(sub_scr);
      lv_obj_set_style_radius(sb_btn, sp_radius, LV_PART_MAIN);
      lv_obj_set_style_pad_all(sb_btn, sp_pad, LV_PART_MAIN);
      lv_obj_set_style_text_font(sb_btn, sp_btn_fnt, LV_PART_MAIN);
      lv_obj_set_style_text_color(sb_btn, sp_txt_color, LV_PART_MAIN);
      lv_obj_set_style_shadow_width(sb_btn, 0, LV_PART_MAIN);
      if (has_off) lv_obj_set_style_bg_color(sb_btn, lv_color_hex(off_val),
        static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
      if (has_on) lv_obj_set_style_bg_color(sb_btn, lv_color_hex(on_val),
        static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_CHECKED));
      int cs = sp_ord.is_wide[bn - 1] ? 2 : 1;
      lv_obj_set_grid_cell(sb_btn, LV_GRID_ALIGN_STRETCH, col, cs, LV_GRID_ALIGN_STRETCH, row, rs);

      lv_obj_t *sil = lv_label_create(sb_btn);
      lv_obj_set_style_text_font(sil, sp_icon_fnt, LV_PART_MAIN);
      const char *sic = find_icon(sb.icon.c_str());
      if ((sb.icon.empty() || sb.icon == "Auto") && !sb.entity.empty()) {
        sic = domain_default_icon(sb.entity.substr(0, sb.entity.find('.')));
      }
      lv_label_set_text(sil, sic);
      lv_obj_align(sil, LV_ALIGN_TOP_LEFT, 0, 0);

      lv_obj_t *stl = lv_label_create(sb_btn);
      lv_obj_align(stl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
      if (cfg.wrap_tall_labels && rs > 1) {
        lv_label_set_long_mode(stl, LV_LABEL_LONG_WRAP);
        lv_obj_set_width(stl, lv_pct(100));
      }

      if (is_text_sensor_card(sb.type, sb.precision)) {
        if (has_sensor_color)
          lv_obj_set_style_bg_color(sb_btn, lv_color_hex(sensor_val),
            static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
        lv_obj_clear_flag(sil, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sb_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(stl, "--");
        if (!sb.sensor.empty())
          subscribe_text_sensor_value(stl, sb.sensor);

      } else if (sb.type == "sensor") {
        if (sb.sensor.empty()) continue;
        if (has_sensor_color)
          lv_obj_set_style_bg_color(sb_btn, lv_color_hex(sensor_val),
            static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
        lv_obj_clear_flag(sb_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(sil, LV_OBJ_FLAG_HIDDEN);

        lv_obj_t *sc = lv_obj_create(sb_btn);
        lv_obj_set_align(sc, LV_ALIGN_TOP_LEFT);
        lv_obj_set_size(sc, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
        lv_obj_clear_flag(sc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(sc, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_opa(sc, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(sc, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(sc, 0, LV_PART_MAIN);
        lv_obj_set_layout(sc, LV_LAYOUT_FLEX);
        lv_obj_set_style_flex_flow(sc, LV_FLEX_FLOW_ROW, LV_PART_MAIN);
        lv_obj_set_style_flex_cross_place(sc, LV_FLEX_ALIGN_END, LV_PART_MAIN);

        lv_obj_t *svl = lv_label_create(sc);
        lv_obj_set_style_text_font(svl, cfg.sp_sensor_font, LV_PART_MAIN);
        lv_obj_set_style_text_color(svl, sp_txt_color, LV_PART_MAIN);
        lv_label_set_text(svl, "--");

        lv_obj_t *sul = lv_label_create(sc);
        lv_obj_set_style_text_font(sul, sp_btn_fnt, LV_PART_MAIN);
        lv_obj_set_style_text_color(sul, sp_txt_color, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(sul, 6, LV_PART_MAIN);
        if (!sb.unit.empty())
          lv_label_set_text(sul, sb.unit.c_str());

        subscribe_sensor_value(svl, sb.sensor, parse_precision(sb.precision));
        if (!sb.label.empty()) {
          lv_label_set_text(stl, sb.label.c_str());
        } else {
          subscribe_friendly_name(stl, sb.sensor);
        }

      } else if (sb.type == "weather") {
        if (has_sensor_color)
          lv_obj_set_style_bg_color(sb_btn, lv_color_hex(sensor_val),
            static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
        lv_obj_clear_flag(sb_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(sil, find_icon("Weather Cloudy"));
        lv_label_set_text(stl, "Weather");
        if (!sb.entity.empty())
          subscribe_weather_state(sil, stl, sb.entity);

      } else if (sb.type == "push") {
        if (!sb.label.empty()) {
          lv_label_set_text(stl, sb.label.c_str());
        } else {
          lv_label_set_text(stl, "Push");
        }
        std::string push_label = sb.label.empty() ? "Push" : sb.label;
        // Intentionally leaked -- lives for the lifetime of the display.
        std::string *label = new std::string(push_label);
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          std::string *label = (std::string *)lv_event_get_user_data(e);
          esphome::api::HomeassistantActionRequest req;
          req.service = decltype(req.service)("esphome.push_button_pressed");
          req.is_event = true;
          req.data.init(1);
          auto &kv = req.data.emplace_back();
          kv.key = decltype(kv.key)("label");
          kv.value = decltype(kv.value)(label->c_str());
          esphome::api::global_api_server->send_homeassistant_action(req);
        }, LV_EVENT_CLICKED, label);

      } else if ((sb.type == "slider" || sb.type == "cover") && !sb.entity.empty()) {
        lv_obj_t *sl = setup_subpage_slider(sb_btn, sil, stl, sb, has_on ? on_val : DEFAULT_SLIDER_COLOR, sp_radius);

        if (sp_indicator) {
          lv_obj_t *parent_btn = slots[si].btn;
          lv_obj_t *parent_icon = slots[si].icon_lbl;
          int parent_idx = si;
          int cwi = sp_alloc_idx++;
          sp_child_was_on[cwi] = false;
          bool *child_was_on = &sp_child_was_on[cwi];
          bool has_alt_icon = sp_has_icon_on;
          const char* off_glyph = sp_icon_off_glyph;
          const char* on_glyph = sp_icon_on_glyph;
          esphome::api::global_api_server->subscribe_home_assistant_state(
            sb.entity, {},
            std::function<void(const std::string &)>([parent_btn, parent_icon, parent_idx, child_was_on, has_alt_icon, off_glyph, on_glyph](const std::string &state) {
              bool is_on = is_entity_on(state);
              if (is_on && !*child_was_on) {
                sp_on_count[parent_idx]++;
                *child_was_on = true;
              } else if (!is_on && *child_was_on) {
                sp_on_count[parent_idx]--;
                *child_was_on = false;
              }
              if (sp_on_count[parent_idx] > 0) {
                lv_obj_add_state(parent_btn, LV_STATE_CHECKED);
                if (has_alt_icon) lv_label_set_text(parent_icon, on_glyph);
              } else {
                lv_obj_clear_state(parent_btn, LV_STATE_CHECKED);
                if (has_alt_icon) lv_label_set_text(parent_icon, off_glyph);
              }
            })
          );
        }

      } else if (!sb.entity.empty()) {
        if (!sb.label.empty()) {
          lv_label_set_text(stl, sb.label.c_str());
        } else {
          subscribe_friendly_name(stl, sb.entity);
        }

        lv_obj_t *bp = sb_btn;
        esphome::api::global_api_server->subscribe_home_assistant_state(
          sb.entity, {},
          std::function<void(const std::string &)>([bp](const std::string &state) {
            bool on = is_entity_on(state);
            if (on) lv_obj_add_state(bp, LV_STATE_CHECKED);
            else lv_obj_clear_state(bp, LV_STATE_CHECKED);
          })
        );

        if (sp_indicator) {
          lv_obj_t *parent_btn = slots[si].btn;
          lv_obj_t *parent_icon = slots[si].icon_lbl;
          int parent_idx = si;
          int cwi = sp_alloc_idx++;
          sp_child_was_on[cwi] = false;
          bool *child_was_on = &sp_child_was_on[cwi];
          bool has_alt_icon = sp_has_icon_on;
          const char* off_glyph = sp_icon_off_glyph;
          const char* on_glyph = sp_icon_on_glyph;
          esphome::api::global_api_server->subscribe_home_assistant_state(
            sb.entity, {},
            std::function<void(const std::string &)>([parent_btn, parent_icon, parent_idx, child_was_on, has_alt_icon, off_glyph, on_glyph](const std::string &state) {
              bool is_on = is_entity_on(state);
              if (is_on && !*child_was_on) {
                sp_on_count[parent_idx]++;
                *child_was_on = true;
              } else if (!is_on && *child_was_on) {
                sp_on_count[parent_idx]--;
                *child_was_on = false;
              }
              if (sp_on_count[parent_idx] > 0) {
                lv_obj_add_state(parent_btn, LV_STATE_CHECKED);
                if (has_alt_icon) lv_label_set_text(parent_icon, on_glyph);
              } else {
                lv_obj_clear_state(parent_btn, LV_STATE_CHECKED);
                if (has_alt_icon) lv_label_set_text(parent_icon, off_glyph);
              }
            })
          );
        }

        int eid_idx = sp_alloc_idx++;
        sp_entity_ids[eid_idx] = sb.entity;
        lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
          std::string *en = (std::string *)lv_event_get_user_data(e);
          if (en && !en->empty()) send_toggle_action(*en);
        }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
      } else {
        lv_label_set_text(stl, sb.label.empty() ? "Configure" : sb.label.c_str());
      }
    }

    lv_obj_set_user_data(slots[si].btn, (void *)sub_scr);
  }
  ESP_LOGI("sensors", "Phase 2: done (%lu ms)", esphome::millis());
}

// ── Phase 3: Temperature + presence subscriptions ────────────────────

inline void grid_phase3(
    bool indoor_on, bool outdoor_on,
    const std::string &indoor_entity, const std::string &outdoor_entity,
    float *indoor_temp_ptr, float *outdoor_temp_ptr,
    lv_obj_t *temp_label,
    const std::string &presence_entity,
    bool *presence_detected_ptr,
    std::function<void()> wake_callback,
    std::function<void()> sleep_callback) {
  ESP_LOGI("sensors", "Phase 3: temp/presence subscriptions start (%lu ms)", esphome::millis());

  if (indoor_on && outdoor_on) {
    lv_label_set_text(temp_label, "-\u00B0 / -\u00B0");
  } else if (indoor_on || outdoor_on) {
    lv_label_set_text(temp_label, "-\u00B0");
  }

  if (indoor_on && !indoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      indoor_entity, {},
      std::function<void(const std::string &)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](const std::string &state) {
          char *end;
          float val = strtof(state.c_str(), &end);
          if (end != state.c_str()) {
            *indoor_temp_ptr = val;
            float outdoor = *outdoor_temp_ptr;
            char buf[16];
            if (std::isnan(outdoor)) {
              snprintf(buf, sizeof(buf), "%.0f\u00B0", val);
            } else {
              snprintf(buf, sizeof(buf), "%.0f\u00B0 / %.0f\u00B0", outdoor, val);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (outdoor_on && !outdoor_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      outdoor_entity, {},
      std::function<void(const std::string &)>(
        [indoor_temp_ptr, outdoor_temp_ptr, temp_label](const std::string &state) {
          char *end;
          float val = strtof(state.c_str(), &end);
          if (end != state.c_str()) {
            *outdoor_temp_ptr = val;
            float indoor = *indoor_temp_ptr;
            char buf[16];
            if (std::isnan(indoor)) {
              snprintf(buf, sizeof(buf), "%.0f\u00B0", val);
            } else {
              snprintf(buf, sizeof(buf), "%.0f\u00B0 / %.0f\u00B0", val, indoor);
            }
            lv_label_set_text(temp_label, buf);
          }
        })
    );
  }

  if (!presence_entity.empty()) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      presence_entity, {},
      std::function<void(const std::string &)>(
        [presence_detected_ptr, wake_callback, sleep_callback](const std::string &state) {
          if (state == "on") {
            *presence_detected_ptr = true;
            lv_disp_trig_activity(NULL);
            wake_callback();
          } else if (state == "off") {
            *presence_detected_ptr = false;
            sleep_callback();
          }
        })
    );
  }
  ESP_LOGI("sensors", "Phase 3: done (%lu ms)", esphome::millis());
}
