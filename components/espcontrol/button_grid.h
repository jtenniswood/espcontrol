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
constexpr int MAX_GRID_SLOTS = 25;
constexpr int MAX_SUBPAGE_ITEMS = MAX_GRID_SLOTS * MAX_GRID_SLOTS;

inline int bounded_grid_slots(int num_slots) {
  if (num_slots < 0) return 0;
  return num_slots > MAX_GRID_SLOTS ? MAX_GRID_SLOTS : num_slots;
}

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

inline std::vector<std::string> split_config_fields(const std::string &value, char delim) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= value.length()) {
    size_t end = value.find(delim, start);
    if (end == std::string::npos) end = value.length();
    out.push_back(value.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

inline int hex_digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

inline std::string decode_compact_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (size_t i = 0; i < value.size(); i++) {
    if (value[i] == '%' && i + 2 < value.size()) {
      int hi = hex_digit(value[i + 1]);
      int lo = hex_digit(value[i + 2]);
      if (hi >= 0 && lo >= 0) {
        out.push_back(static_cast<char>((hi << 4) | lo));
        i += 2;
        continue;
      }
    }
    out.push_back(value[i]);
  }
  return out;
}

// Structured view of a button config string: entity;label;icon;icon_on;sensor;unit;type;precision
struct ParsedCfg {
  std::string entity;      // 0  HA entity_id, or internal relay key for internal cards
  std::string label;       // 1  display name (blank = use HA friendly_name)
  std::string icon;        // 2  icon name for off/default state
  std::string icon_on;     // 3  icon name for on state (blank = no swap)
  std::string sensor;      // 4  sensor entity; "h" for horizontal slider; "push" for internal relay; "toggle"/"tilt" for cover modes
  std::string unit;        // 5  unit suffix for sensor display
  std::string type;        // 6  button type: "" (toggle), sensor, slider, cover, garage, push, internal, subpage
  std::string precision;   // 7  decimal places for sensors; "text" = text sensor mode
};

inline ParsedCfg parse_cfg(const std::string &cfg) {
  ParsedCfg p;
  if (!cfg.empty() && cfg[0] == '~') {
    std::vector<std::string> f = split_config_fields(cfg.substr(1), ',');
    p.entity    = f.size() > 0 ? decode_compact_field(f[0]) : "";
    p.label     = f.size() > 1 ? decode_compact_field(f[1]) : "";
    p.icon      = f.size() > 2 ? decode_compact_field(f[2]) : "";
    p.icon_on   = f.size() > 3 ? decode_compact_field(f[3]) : "";
    p.sensor    = f.size() > 4 ? decode_compact_field(f[4]) : "";
    p.unit      = f.size() > 5 ? decode_compact_field(f[5]) : "";
    p.type      = f.size() > 6 ? decode_compact_field(f[6]) : "";
    p.precision = f.size() > 7 ? decode_compact_field(f[7]) : "";
    return p;
  }
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

inline const char* garage_closed_icon(const std::string &icon) {
  return (icon.empty() || icon == "Auto") ? find_icon("Garage") : find_icon(icon.c_str());
}

inline const char* garage_open_icon(const std::string &icon_on) {
  return (icon_on.empty() || icon_on == "Auto") ? find_icon("Garage Open") : find_icon(icon_on.c_str());
}

// ── Internal relay controls ───────────────────────────────────────────
//
// Only devices that actually have relays register entries here. The shared
// grid code can then control those relays locally without referencing device-
// specific ids, so non-relay devices still compile and simply have no relays.

struct InternalRelayControl {
  std::string key;
  std::string label;
  std::function<void(bool)> set_state;
  std::function<void()> pulse;
  std::function<bool()> is_on;
};

struct InternalRelayWatcher {
  std::string key;
  lv_obj_t *btn;
  lv_obj_t *icon_lbl;
  bool has_icon_on;
  const char *icon_off;
  const char *icon_on;
  bool *child_was_on;
  lv_obj_t *parent_btn;
  lv_obj_t *parent_icon;
  int parent_idx;
  bool parent_has_alt_icon;
  const char *parent_off_glyph;
  const char *parent_on_glyph;
  int *sp_on_count;
};

struct InternalRelayClickCtx {
  std::string key;
  bool push_mode;
};

inline std::vector<InternalRelayControl> &internal_relay_registry() {
  static std::vector<InternalRelayControl> relays;
  return relays;
}

inline std::vector<InternalRelayWatcher> &internal_relay_watchers() {
  static std::vector<InternalRelayWatcher> watchers;
  return watchers;
}

inline void clear_internal_relay_watchers() {
  internal_relay_watchers().clear();
}

inline void register_internal_relay(
    const std::string &key, const std::string &label,
    std::function<void(bool)> set_state,
    std::function<void()> pulse,
    std::function<bool()> is_on) {
  if (key.empty()) return;
  InternalRelayControl r;
  r.key = key;
  r.label = label;
  r.set_state = set_state;
  r.pulse = pulse;
  r.is_on = is_on;

  auto &relays = internal_relay_registry();
  for (auto &existing : relays) {
    if (existing.key == key) {
      existing = r;
      return;
    }
  }
  relays.push_back(r);
}

inline InternalRelayControl *find_internal_relay(const std::string &key) {
  auto &relays = internal_relay_registry();
  for (auto &relay : relays) {
    if (relay.key == key) return &relay;
  }
  return nullptr;
}

inline bool internal_relay_push_mode(const ParsedCfg &p) {
  return p.sensor == "push";
}

inline bool internal_relay_state(const std::string &key) {
  InternalRelayControl *relay = find_internal_relay(key);
  return relay && relay->is_on ? relay->is_on() : false;
}

inline std::string internal_relay_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label;
  InternalRelayControl *relay = find_internal_relay(p.entity);
  if (relay && !relay->label.empty()) return relay->label;
  return p.entity.empty() ? std::string("Relay") : sentence_cap_text(p.entity);
}

inline const char *internal_relay_icon(const ParsedCfg &p, bool push_mode) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(push_mode ? "Gesture Tap" : "Power Plug");
}

inline void apply_internal_relay_state(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                       bool on, bool has_icon_on,
                                       const char *icon_off, const char *icon_on) {
  if (btn) {
    if (on) lv_obj_add_state(btn, LV_STATE_CHECKED);
    else lv_obj_clear_state(btn, LV_STATE_CHECKED);
  }
  if (icon_lbl && has_icon_on)
    lv_label_set_text(icon_lbl, on ? icon_on : icon_off);
}

inline void apply_internal_relay_parent_indicator(InternalRelayWatcher &w, bool on) {
  if (!w.child_was_on || !w.parent_btn || !w.sp_on_count) return;
  if (on && !*w.child_was_on) {
    w.sp_on_count[w.parent_idx]++;
    *w.child_was_on = true;
  } else if (!on && *w.child_was_on) {
    w.sp_on_count[w.parent_idx]--;
    *w.child_was_on = false;
  }
  if (w.sp_on_count[w.parent_idx] > 0) {
    lv_obj_add_state(w.parent_btn, LV_STATE_CHECKED);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_on_glyph);
  } else {
    lv_obj_clear_state(w.parent_btn, LV_STATE_CHECKED);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_off_glyph);
  }
}

inline void notify_internal_relay_changed(const std::string &key, bool on) {
  auto &watchers = internal_relay_watchers();
  for (auto &w : watchers) {
    if (w.key != key) continue;
    apply_internal_relay_state(w.btn, w.icon_lbl, on, w.has_icon_on, w.icon_off, w.icon_on);
    apply_internal_relay_parent_indicator(w, on);
  }
}

inline void watch_internal_relay_state(
    const std::string &key, lv_obj_t *btn, lv_obj_t *icon_lbl,
    bool has_icon_on, const char *icon_off, const char *icon_on,
    bool *child_was_on = nullptr, lv_obj_t *parent_btn = nullptr,
    lv_obj_t *parent_icon = nullptr, int parent_idx = 0,
    bool parent_has_alt_icon = false, const char *parent_off_glyph = nullptr,
    const char *parent_on_glyph = nullptr, int *sp_on_count = nullptr) {
  if (key.empty()) return;
  InternalRelayWatcher w;
  w.key = key;
  w.btn = btn;
  w.icon_lbl = icon_lbl;
  w.has_icon_on = has_icon_on;
  w.icon_off = icon_off;
  w.icon_on = icon_on;
  w.child_was_on = child_was_on;
  w.parent_btn = parent_btn;
  w.parent_icon = parent_icon;
  w.parent_idx = parent_idx;
  w.parent_has_alt_icon = parent_has_alt_icon;
  w.parent_off_glyph = parent_off_glyph;
  w.parent_on_glyph = parent_on_glyph;
  w.sp_on_count = sp_on_count;
  internal_relay_watchers().push_back(w);

  bool on = internal_relay_state(key);
  apply_internal_relay_state(btn, icon_lbl, on, has_icon_on, icon_off, icon_on);
  InternalRelayWatcher &stored = internal_relay_watchers().back();
  apply_internal_relay_parent_indicator(stored, on);
}

inline void send_internal_relay_action(const std::string &key, bool push_mode) {
  InternalRelayControl *relay = find_internal_relay(key);
  if (!relay) return;
  if (push_mode) {
    if (relay->pulse) relay->pulse();
    return;
  }
  bool next = !internal_relay_state(key);
  if (relay->set_state) relay->set_state(next);
  notify_internal_relay_changed(key, next);
}

inline void send_internal_relay_action(const ParsedCfg &p) {
  send_internal_relay_action(p.entity, internal_relay_push_mode(p));
}

inline std::string garage_state_label(const std::string &state) {
  if (state.empty()) return "--";
  return sentence_cap_text(state);
}

inline bool garage_state_is_active(const std::string &state) {
  return state == "open" || state == "opening" || state == "closing";
}

inline bool cover_toggle_state_is_active(const std::string &state) {
  return state == "closed" || state == "closing";
}

inline bool garage_state_uses_open_icon(const std::string &state) {
  return state == "open" || state == "opening";
}

inline bool garage_state_releases_label(const std::string &state) {
  return state == "open" || state == "closed";
}

// Reusable label helper: show changed status, then optionally return to steady text.
static const uint32_t STATUS_LABEL_STABLE_MS = 3000;

struct TransientStatusLabel {
  lv_obj_t *label = nullptr;
  std::string steady_text;
  std::string last_status_text;
  bool has_status = false;
  bool showing_status = false;
  lv_timer_t *revert_timer = nullptr;
};

inline void transient_status_label_revert_cb(lv_timer_t *timer) {
  TransientStatusLabel *ctx = static_cast<TransientStatusLabel *>(lv_timer_get_user_data(timer));
  if (!ctx) return;
  ctx->showing_status = false;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  lv_timer_pause(timer);
}

inline TransientStatusLabel *create_transient_status_label(
    lv_obj_t *label, const std::string &steady_text,
    uint32_t stable_ms = STATUS_LABEL_STABLE_MS) {
  // Intentionally leaked -- lives for the lifetime of the display.
  TransientStatusLabel *ctx = new TransientStatusLabel();
  ctx->label = label;
  ctx->steady_text = steady_text;
  if (ctx->label) lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  ctx->revert_timer = lv_timer_create(transient_status_label_revert_cb, stable_ms, ctx);
  if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
  return ctx;
}

inline void transient_status_label_set_steady(TransientStatusLabel *ctx,
                                              const std::string &steady_text) {
  if (!ctx) return;
  ctx->steady_text = steady_text;
  if (!ctx->showing_status && ctx->label) {
    lv_label_set_text(ctx->label, ctx->steady_text.c_str());
  }
}

inline void transient_status_label_show_if_changed(TransientStatusLabel *ctx,
                                                   const std::string &status_text,
                                                   bool release_to_steady = true) {
  if (!ctx) return;
  if (!ctx->has_status) {
    ctx->last_status_text = status_text;
    ctx->has_status = true;
    if (!release_to_steady) {
      ctx->showing_status = true;
      if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
      if (ctx->revert_timer) lv_timer_pause(ctx->revert_timer);
    }
    return;
  }
  if (ctx->last_status_text == status_text) return;
  ctx->last_status_text = status_text;
  ctx->showing_status = true;
  if (ctx->label) lv_label_set_text(ctx->label, status_text.c_str());
  if (ctx->revert_timer) {
    if (release_to_steady) {
      lv_timer_reset(ctx->revert_timer);
      lv_timer_resume(ctx->revert_timer);
    } else {
      lv_timer_pause(ctx->revert_timer);
    }
  }
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
  int positions[MAX_GRID_SLOTS] = {};    // slot number at each grid position (1-based, 0=empty)
  bool is_double[MAX_GRID_SLOTS] = {};   // slot uses double height (suffix "d" or "b")
  bool is_wide[MAX_GRID_SLOTS] = {};     // slot uses double width (suffix "w" or "b")
};

// Parse "1,2d,3w,4b,..." into positions + double/wide flags
inline void parse_order_string(const std::string &order_str, int num_slots, OrderResult &result) {
  memset(result.positions, 0, sizeof(result.positions));
  memset(result.is_double, 0, sizeof(result.is_double));
  memset(result.is_wide, 0, sizeof(result.is_wide));
  int slot_limit = bounded_grid_slots(num_slots);
  if (order_str.empty()) return;
  size_t gpos = 0, start = 0;
  while (start <= order_str.length() && gpos < (size_t)slot_limit) {
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
      if (v >= 1 && v <= slot_limit) {
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
  int slot_limit = bounded_grid_slots(num_slots);
  for (int p = 0; p < slot_limit; p++) {
    result.positions[p] = order.positions[p];
    result.is_double[p] = order.is_double[p];
    result.is_wide[p] = order.is_wide[p];
  }
  for (int p = 0; p < slot_limit; p++) {
    if (result.positions[p] <= 0) continue;
    int idx = result.positions[p] - 1;
    if (result.is_double[idx] && p + cols < slot_limit) {
      result.positions[p + cols] = 0;
    }
    if (result.is_wide[idx] && (p + 1) % cols != 0 && p + 1 < slot_limit) {
      result.positions[p + 1] = 0;
    }
    if (result.is_double[idx] && result.is_wide[idx] && (p + 1) % cols != 0 && p + cols + 1 < slot_limit) {
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

// Match the main-page button widget label behavior so longer titles wrap
// instead of running off the edge of the tile.
inline void configure_button_label_wrap(lv_obj_t *label) {
  if (!label) return;
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
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

struct CalendarCardRef {
  lv_obj_t *value_lbl;
  lv_obj_t *unit_lbl;
  lv_obj_t *label_lbl;
  bool show_time;
};

inline CalendarCardRef *calendar_card_refs() {
  static CalendarCardRef refs[MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS];
  return refs;
}

inline int &calendar_card_count() {
  static int count = 0;
  return count;
}

struct CalendarDateState {
  bool date_valid;
  int day;
  int month;
  bool time_valid;
  int hour;
  int minute;
  bool use_12h;
};

inline CalendarDateState &calendar_date_state() {
  static CalendarDateState state = {false, 0, 0, false, 0, 0, false};
  return state;
}

inline bool calendar_date_valid() {
  return calendar_date_state().date_valid;
}

inline bool calendar_card_shows_time(const ParsedCfg &p) {
  return p.precision == "datetime";
}

inline void apply_calendar_card_text(const CalendarCardRef &ref,
                                     const CalendarDateState &state);

inline void reset_calendar_cards() {
  calendar_card_count() = 0;
}

inline void register_calendar_card(lv_obj_t *value_lbl, lv_obj_t *unit_lbl,
                                   lv_obj_t *label_lbl, bool show_time) {
  int &count = calendar_card_count();
  if (count >= MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS) {
    ESP_LOGW("calendar", "Too many calendar cards; skipping date updates");
    return;
  }
  calendar_card_refs()[count++] = {value_lbl, unit_lbl, label_lbl, show_time};
  CalendarDateState &state = calendar_date_state();
  apply_calendar_card_text(calendar_card_refs()[count - 1], state);
}

inline const char *calendar_month_name(int month) {
  static const char *months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
  };
  if (month < 1 || month > 12) return "Date";
  return months[month - 1];
}

inline void apply_calendar_card_text(const CalendarCardRef &ref,
                                     const CalendarDateState &state) {
  char value_buf[8];
  char label_buf[32];
  const char *value_text = "--";
  const char *unit_text = "";
  const char *label_text = "Date";

  if (ref.show_time) {
    if (state.time_valid &&
        state.day >= 1 && state.day <= 31 &&
        state.month >= 1 && state.month <= 12 &&
        state.hour >= 0 && state.hour <= 23 &&
        state.minute >= 0 && state.minute <= 59) {
      if (state.use_12h) {
        int hour12 = state.hour % 12;
        if (hour12 == 0) hour12 = 12;
        snprintf(value_buf, sizeof(value_buf), "%d:%02d", hour12, state.minute);
        unit_text = state.hour < 12 ? "am" : "pm";
      } else {
        snprintf(value_buf, sizeof(value_buf), "%02d:%02d", state.hour, state.minute);
      }
      value_text = value_buf;
      snprintf(label_buf, sizeof(label_buf), "%d %s", state.day, calendar_month_name(state.month));
      label_text = label_buf;
    }
  } else if (state.date_valid &&
             state.day >= 1 && state.day <= 31 &&
             state.month >= 1 && state.month <= 12) {
    snprintf(value_buf, sizeof(value_buf), "%d", state.day);
    value_text = value_buf;
    label_text = calendar_month_name(state.month);
  }
  if (ref.value_lbl) lv_label_set_text(ref.value_lbl, value_text);
  if (ref.unit_lbl) lv_label_set_text(ref.unit_lbl, unit_text);
  if (ref.label_lbl) lv_label_set_text(ref.label_lbl, label_text);
}

inline void update_calendar_cards(bool valid, int day, int month) {
  CalendarDateState &state = calendar_date_state();
  state.date_valid = valid && day >= 1 && day <= 31 && month >= 1 && month <= 12;
  state.day = state.date_valid ? day : 0;
  state.month = state.date_valid ? month : 0;
  if (!state.date_valid && !state.time_valid) {
    state.hour = 0;
    state.minute = 0;
    state.use_12h = false;
  }

  CalendarCardRef *refs = calendar_card_refs();
  int count = calendar_card_count();
  for (int i = 0; i < count; i++) {
    apply_calendar_card_text(refs[i], state);
  }
}

inline void update_calendar_cards_time(bool valid, int day, int month,
                                       int hour, int minute, bool use_12h) {
  CalendarDateState &state = calendar_date_state();
  state.time_valid = valid &&
                     day >= 1 && day <= 31 &&
                     month >= 1 && month <= 12 &&
                     hour >= 0 && hour <= 23 &&
                     minute >= 0 && minute <= 59;
  state.hour = state.time_valid ? hour : 0;
  state.minute = state.time_valid ? minute : 0;
  state.use_12h = state.time_valid ? use_12h : false;
  if (state.time_valid) {
    state.date_valid = true;
    state.day = day;
    state.month = month;
  }

  CalendarCardRef *refs = calendar_card_refs();
  int count = calendar_card_count();
  for (int i = 0; i < count; i++) {
    apply_calendar_card_text(refs[i], state);
  }
}

inline bool parse_calendar_date_text(const std::string &value, int &day, int &month) {
  if (value.length() < 10) return false;
  if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
      !std::isdigit(static_cast<unsigned char>(value[1])) ||
      !std::isdigit(static_cast<unsigned char>(value[2])) ||
      !std::isdigit(static_cast<unsigned char>(value[3])) ||
      value[4] != '-' ||
      !std::isdigit(static_cast<unsigned char>(value[5])) ||
      !std::isdigit(static_cast<unsigned char>(value[6])) ||
      value[7] != '-' ||
      !std::isdigit(static_cast<unsigned char>(value[8])) ||
      !std::isdigit(static_cast<unsigned char>(value[9]))) {
    return false;
  }
  int parsed_month = (value[5] - '0') * 10 + (value[6] - '0');
  int parsed_day = (value[8] - '0') * 10 + (value[9] - '0');
  if (parsed_day < 1 || parsed_day > 31 || parsed_month < 1 || parsed_month > 12) return false;
  day = parsed_day;
  month = parsed_month;
  return true;
}

inline bool parse_calendar_date_text_dmy(const std::string &value, int &day, int &month) {
  if (value.length() < 10) return false;
  if (!std::isdigit(static_cast<unsigned char>(value[0])) ||
      !std::isdigit(static_cast<unsigned char>(value[1])) ||
      (value[2] != '/' && value[2] != '-') ||
      !std::isdigit(static_cast<unsigned char>(value[3])) ||
      !std::isdigit(static_cast<unsigned char>(value[4])) ||
      (value[5] != '/' && value[5] != '-') ||
      !std::isdigit(static_cast<unsigned char>(value[6])) ||
      !std::isdigit(static_cast<unsigned char>(value[7])) ||
      !std::isdigit(static_cast<unsigned char>(value[8])) ||
      !std::isdigit(static_cast<unsigned char>(value[9]))) {
    return false;
  }
  int parsed_day = (value[0] - '0') * 10 + (value[1] - '0');
  int parsed_month = (value[3] - '0') * 10 + (value[4] - '0');
  if (parsed_day < 1 || parsed_day > 31 || parsed_month < 1 || parsed_month > 12) return false;
  day = parsed_day;
  month = parsed_month;
  return true;
}

inline bool update_calendar_cards_from_date_text(const std::string &value) {
  int day = 0;
  int month = 0;
  bool valid = parse_calendar_date_text(value, day, month) ||
               parse_calendar_date_text_dmy(value, day, month);
  if (valid) update_calendar_cards(true, day, month);
  return valid;
}

inline std::string calendar_date_entity_or_default(const std::string &entity_id) {
  return entity_id.empty() ? std::string("sensor.date") : entity_id;
}

inline void subscribe_calendar_date_source(const std::string &entity_id) {
  std::string source = calendar_date_entity_or_default(entity_id);
  static std::vector<std::string> subscribed;
  for (const auto &existing : subscribed) {
    if (existing == source) return;
  }
  subscribed.push_back(source);
  esphome::api::global_api_server->subscribe_home_assistant_state(
    source, {},
    std::function<void(const std::string &)>([](const std::string &state) {
      update_calendar_cards_from_date_text(state);
    })
  );
}

inline void setup_calendar_card(BtnSlot &s, const ParsedCfg &p,
                                bool has_sensor_color, uint32_t sensor_val) {
  if (has_sensor_color) {
    lv_obj_set_style_bg_color(s.btn, lv_color_hex(sensor_val),
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
  }
  lv_obj_clear_flag(s.btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(s.icon_lbl, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(s.sensor_container, LV_OBJ_FLAG_HIDDEN);
  lv_label_set_text(s.sensor_lbl, "--");
  lv_label_set_text(s.unit_lbl, "");
  lv_label_set_text(s.text_lbl, "Date");
  register_calendar_card(s.sensor_lbl, s.unit_lbl, s.text_lbl, calendar_card_shows_time(p));
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

inline void setup_garage_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, garage_closed_icon(p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Garage Door" : p.label.c_str());
}

inline void apply_push_button_transition(lv_obj_t *btn) {
  static const lv_style_prop_t push_props[] = {LV_STYLE_BG_COLOR, LV_STYLE_PROP_INV};
  static lv_style_transition_dsc_t push_trans;
  static bool push_trans_inited = false;
  if (!push_trans_inited) {
    lv_style_transition_dsc_init(&push_trans, push_props, lv_anim_path_ease_out, 400, 0, NULL);
    push_trans_inited = true;
  }
  lv_obj_set_style_transition(btn, &push_trans,
    static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_DEFAULT);
}

inline void setup_internal_relay_card(BtnSlot &s, const ParsedCfg &p) {
  bool push_mode = internal_relay_push_mode(p);
  std::string label = internal_relay_label(p);
  lv_label_set_text(s.text_lbl, label.c_str());
  const char *icon_off = internal_relay_icon(p, push_mode);
  lv_label_set_text(s.icon_lbl, icon_off);
  if (push_mode) {
    apply_push_button_transition(s.btn);
    return;
  }
  bool has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
  const char *icon_on = has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
  apply_internal_relay_state(s.btn, s.icon_lbl, internal_relay_state(p.entity),
    has_icon_on, icon_off, icon_on);
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
      apply_push_button_transition(s.btn);
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

inline void subscribe_garage_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                   TransientStatusLabel *status_label,
                                   const char *closed_icon, const char *open_icon,
                                   const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](const std::string &state) {
        bool active = garage_state_is_active(state);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state), garage_state_releases_label(state));
      })
  );
}

inline void subscribe_cover_toggle_state(lv_obj_t *btn_ptr, lv_obj_t *icon_lbl,
                                         TransientStatusLabel *status_label,
                                         const char *closed_icon, const char *open_icon,
                                         const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>(
      [btn_ptr, icon_lbl, status_label, closed_icon, open_icon](const std::string &state) {
        bool active = cover_toggle_state_is_active(state);
        if (active) lv_obj_add_state(btn_ptr, LV_STATE_CHECKED);
        else lv_obj_clear_state(btn_ptr, LV_STATE_CHECKED);
        lv_label_set_text(icon_lbl, garage_state_uses_open_icon(state) ? open_icon : closed_icon);
        transient_status_label_show_if_changed(
          status_label, garage_state_label(state), garage_state_releases_label(state));
      })
  );
}

// Subscribe to an entity's friendly_name attribute and use it as the button label
inline void subscribe_friendly_name(TransientStatusLabel *status_label,
                                    const std::string &entity_id) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, std::string("friendly_name"),
    std::function<void(const std::string &)>([status_label](const std::string &name) {
      transient_status_label_set_steady(status_label, name);
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

inline bool is_button_entity(const std::string &entity_id) {
  return entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0;
}

// Press HA button entities; toggle other bound entities.
inline void send_toggle_action(const std::string &entity_id) {
  esphome::api::HomeassistantActionRequest req;
  req.service = is_button_entity(entity_id)
    ? decltype(req.service)("button.press")
    : decltype(req.service)("homeassistant.toggle");
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

inline bool is_fan_entity(const std::string &entity_id) {
  return entity_id.size() > 4 && entity_id.compare(0, 4, "fan.") == 0;
}

inline bool cover_toggle_mode(const std::string &sensor) {
  return sensor == "toggle";
}

inline bool cover_tilt_mode(const std::string &sensor) {
  return sensor == "tilt";
}

// Send HA action for a slider change: toggle (value<0), brightness, or cover position/tilt
inline void send_slider_action(const std::string &entity_id, int value, bool cover_tilt = false) {
  esphome::api::HomeassistantActionRequest req;
  req.is_event = false;
  if (value < 0) {
    req.service = decltype(req.service)("homeassistant.toggle");
    req.data.init(1);
    auto &kv = req.data.emplace_back();
    kv.key = decltype(kv.key)("entity_id");
    kv.value = decltype(kv.value)(entity_id.c_str());
  } else if (is_cover_entity(entity_id)) {
    req.service = decltype(req.service)(
      cover_tilt ? "cover.set_cover_tilt_position" : "cover.set_cover_position");
    req.data.init(2);
    auto &kv1 = req.data.emplace_back();
    kv1.key = decltype(kv1.key)("entity_id");
    kv1.value = decltype(kv1.value)(entity_id.c_str());
    auto &kv2 = req.data.emplace_back();
    kv2.key = decltype(kv2.key)(cover_tilt ? "tilt_position" : "position");
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    kv2.value = decltype(kv2.value)(buf);
  } else if (is_fan_entity(entity_id)) {
    if (value == 0) {
      req.service = decltype(req.service)("fan.turn_off");
      req.data.init(1);
      auto &kv = req.data.emplace_back();
      kv.key = decltype(kv.key)("entity_id");
      kv.value = decltype(kv.value)(entity_id.c_str());
    } else {
      req.service = decltype(req.service)("fan.turn_on");
      req.data.init(2);
      auto &kv1 = req.data.emplace_back();
      kv1.key = decltype(kv1.key)("entity_id");
      kv1.value = decltype(kv1.value)(entity_id.c_str());
      auto &kv2 = req.data.emplace_back();
      kv2.key = decltype(kv2.key)("percentage");
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", value);
      kv2.value = decltype(kv2.value)(buf);
    }
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
  if (p.type == "sensor" || p.type == "text_sensor" || p.type == "calendar") return;
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
  } else if (p.type == "garage") {
    if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
    if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "internal") {
    if (!p.entity.empty()) send_internal_relay_action(p);
  } else if (p.type == "slider" || p.type == "cover") {
    if (!p.entity.empty()) send_slider_action(p.entity, -1, cover_tilt_mode(p.sensor));
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
  bool cover_tilt;
  bool inverted;
  lv_coord_t radius;
};

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

inline void slider_refresh_geometry(lv_obj_t *slider) {
  if (!slider) return;
  SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(slider);
  lv_obj_t *btn = lv_obj_get_parent(slider);
  if (!c || !btn) return;

  slider_fit_to_button(slider, btn, c->horizontal);
  int val = lv_slider_get_value(slider);
  int fill_val = c->inverted ? 100 - val : val;
  if (c->fill)
    slider_update_fill(c->fill, btn, fill_val, c->horizontal, c->inverted, c->radius);
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
  return type == "cover" || (!icon_on.empty() && icon_on != "Auto");
}

inline const char *slider_icon_off(const std::string &type, const std::string &entity_id,
                                   const std::string &icon) {
  if (type == "cover" && (icon.empty() || icon == "Auto"))
    return find_icon("Blinds");
  if (icon.empty() || icon == "Auto")
    return domain_default_icon(entity_id.substr(0, entity_id.find('.')));
  return find_icon(icon.c_str());
}

inline const char *slider_icon_on(const std::string &type, const std::string &icon_on) {
  if (type == "cover" && (icon_on.empty() || icon_on == "Auto"))
    return find_icon("Blinds Open");
  return find_icon(icon_on.c_str());
}

inline void setup_cover_toggle_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));
  lv_label_set_text(s.text_lbl, p.label.empty() ? "Cover" : p.label.c_str());
}

// Full slider button setup: visual + event handlers + HA action on release
inline void setup_slider_visual(BtnSlot &s, const ParsedCfg &p, uint32_t on_color) {
  setup_toggle_visual(s, p);
  if (p.type == "cover")
    lv_label_set_text(s.icon_lbl, slider_icon_off(p.type, p.entity, p.icon));

  bool horizontal = p.type == "slider" && p.sensor == "h";
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
    slider_update_fill(c->fill, lv_obj_get_parent(sl), fill_val, c->horizontal, c->inverted, c->radius);
  }, LV_EVENT_VALUE_CHANGED, nullptr);

  lv_obj_add_event_cb(slider, [](lv_event_t *e) {
    lv_obj_t *sl = static_cast<lv_obj_t *>(lv_event_get_target(e));
    SliderCtx *c = (SliderCtx *)lv_obj_get_user_data(sl);
    if (c && !c->entity_id.empty()) {
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
    std::function<void(const std::string &)>(
      [slider, btn_ptr, fill, horiz, inv, rad, icon_lbl, has_icon_on, icon_off, icon_on](const std::string &state) {
        bool on = is_entity_on(state);
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
              lv_label_set_text(icon_lbl, pct > 0 ? icon_on : icon_off);
            }
          }
        })
    );
  } else if (is_fan) {
    esphome::api::global_api_server->subscribe_home_assistant_state(
      entity_id, std::string("percentage"),
      std::function<void(const std::string &)>(
        [slider, btn_ptr, fill, horiz, inv, rad](const std::string &val) {
          char *end;
          float pct_f = strtof(val.c_str(), &end);
          if (end != val.c_str()) {
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
  std::string sensor;     // sensor entity for toggle; orientation "h"|"" for slider; "toggle"/"tilt" for cover modes
  std::string unit;
  std::string type;       // button type: "" (toggle), sensor, slider, cover, garage, push, internal, subpage
  std::string precision;  // decimal places for sensor display; "text" = text sensor mode
};

inline std::vector<std::string> split_subpage_fields(const std::string &value, char delim) {
  std::vector<std::string> out;
  size_t start = 0;
  while (start <= value.length()) {
    size_t end = value.find(delim, start);
    if (end == std::string::npos) end = value.length();
    out.push_back(value.substr(start, end - start));
    start = end + 1;
  }
  return out;
}

inline std::string compact_subpage_type(const std::string &code) {
  if (code == "D") return "calendar";
  if (code == "S") return "sensor";
  if (code == "W") return "weather";
  if (code == "L") return "slider";
  if (code == "C") return "cover";
  if (code == "R") return "garage";
  if (code == "P") return "push";
  if (code == "I") return "internal";
  if (code == "G") return "subpage";
  return code;
}

inline std::string decode_compact_subpage_field(const std::string &value) {
  return decode_compact_field(value);
}

// Create a slider button inside a subpage screen (reuses main grid slider logic)
inline lv_obj_t *setup_subpage_slider(lv_obj_t *btn, lv_obj_t *icon_lbl, lv_obj_t *text_lbl,
                                       const SubpageBtn &sb, uint32_t on_color, lv_coord_t radius) {
  if (!sb.label.empty()) lv_label_set_text(text_lbl, sb.label.c_str());
  else subscribe_friendly_name(text_lbl, sb.entity);

  bool horiz = sb.type == "slider" && sb.sensor == "h";
  lv_obj_t *sl = setup_slider_widget(btn, on_color, horiz);
  lv_coord_t pad = radius + 4;
  lv_obj_align(icon_lbl, LV_ALIGN_TOP_LEFT, pad, pad);
  lv_obj_align(text_lbl, LV_ALIGN_BOTTOM_LEFT, pad, -pad);
  if (sb.type == "cover")
    lv_label_set_text(icon_lbl, slider_icon_off(sb.type, sb.entity, sb.icon));

  lv_obj_t *fill = lv_obj_get_child(btn, 0);
  // Intentionally leaked -- lives for the lifetime of the display
  SliderCtx *ctx = new SliderCtx();
  ctx->entity_id = sb.entity;
  ctx->fill = fill;
  ctx->horizontal = horiz;
  ctx->cover_tilt = sb.type == "cover" && cover_tilt_mode(sb.sensor);
  ctx->inverted = is_cover_entity(sb.entity);
  ctx->radius = radius;
  lv_obj_set_user_data(sl, (void *)ctx);
  slider_bind_geometry_refresh(btn, sl);

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
      send_slider_action(c->entity_id, lv_slider_get_value(s), c->cover_tilt);
  }, LV_EVENT_RELEASED, nullptr);

  bool has_icon_on = slider_has_alt_icon(sb.type, sb.icon_on);
  const char *sl_icon_on = has_icon_on ? slider_icon_on(sb.type, sb.icon_on) : nullptr;
  const char *sl_icon_off = has_icon_on ? slider_icon_off(sb.type, sb.entity, sb.icon) : nullptr;
  subscribe_slider_state(btn, icon_lbl, sl, has_icon_on, sl_icon_off, sl_icon_on,
    sb.entity, ctx->cover_tilt);

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

  bool compact = sp_cfg[0] == '~';
  std::vector<std::string> pipes = split_subpage_fields(compact ? sp_cfg.substr(1) : sp_cfg, '|');
  if (pipes.size() < 2) return btns;

  for (size_t pi = 1; pi < pipes.size(); pi++) {
    if (compact) {
      std::vector<std::string> flds = split_subpage_fields(pipes[pi], ',');
      std::string tp = flds.size() > 0 ? compact_subpage_type(flds[0]) : "";
      std::string e = flds.size() > 1 ? decode_compact_subpage_field(flds[1]) : "";
      std::string l = flds.size() > 2 ? decode_compact_subpage_field(flds[2]) : "";
      std::string ic = flds.size() > 3 ? decode_compact_subpage_field(flds[3]) : "Auto";
      if (ic.empty()) ic = "Auto";
      std::string io = flds.size() > 4 ? decode_compact_subpage_field(flds[4]) : "Auto";
      if (io.empty()) io = "Auto";
      std::string sn = flds.size() > 5 ? decode_compact_subpage_field(flds[5]) : "";
      std::string un = flds.size() > 6 ? decode_compact_subpage_field(flds[6]) : "";
      std::string pr = flds.size() > 7 ? decode_compact_subpage_field(flds[7]) : "";
      btns.push_back({e, l, ic, io, sn, un, tp, pr});
      continue;
    }
    std::vector<std::string> flds = split_subpage_fields(pipes[pi], ':');
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
  size_t start = sp_cfg[0] == '~' ? 1 : 0;
  size_t pe = sp_cfg.find('|', start);
  if (pe == std::string::npos) return sp_cfg.substr(start);
  return sp_cfg.substr(start, pe - start);
}

// Subpage grid layout with support for a back button token ("B")
struct SubpageOrder {
  int positions[MAX_GRID_SLOTS] = {};
  bool is_double[MAX_GRID_SLOTS] = {};
  bool is_wide[MAX_GRID_SLOTS] = {};
  int back_pos = 0;
  bool back_dbl = false;
  bool back_wide = false;
  bool has_back_token = false;
};

inline void subscribe_subpage_parent_indicator(
    const std::string &entity_id,
    lv_obj_t *parent_btn, lv_obj_t *parent_icon,
    int parent_idx, bool *child_was_on,
    bool has_alt_icon, const char *off_glyph, const char *on_glyph,
    int *sp_on_count) {
  esphome::api::global_api_server->subscribe_home_assistant_state(
    entity_id, {},
    std::function<void(const std::string &)>(
      [parent_btn, parent_icon, parent_idx, child_was_on,
       has_alt_icon, off_glyph, on_glyph, sp_on_count](const std::string &state) {
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

// Parse subpage order CSV; "B"/"Bd"/"Bw"/"Bb" tokens mark the back button position
inline void parse_subpage_order(const std::string &order_str, int num_slots, int num_btns,
                                SubpageOrder &result) {
  int slot_limit = bounded_grid_slots(num_slots);
  int btn_limit = bounded_grid_slots(num_btns);
  if (order_str.empty()) return;
  size_t gp2 = 0, st2 = 0;
  while (st2 <= order_str.length() && gp2 < (size_t)slot_limit) {
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
        if (v >= 1 && v <= btn_limit) {
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
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }

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

  reset_calendar_cards();

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
    if (p.type == "calendar") {
      setup_calendar_card(s, p, has_sensor_color, sensor_val);
      continue;
    }
    if (p.type == "weather") {
      setup_weather_card(s, has_sensor_color, sensor_val);
      continue;
    }
    if (p.type == "garage") {
      setup_garage_card(s, p);
      continue;
    }
    if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
      setup_cover_toggle_card(s, p);
      continue;
    }
    if (p.type == "internal") {
      setup_internal_relay_card(s, p);
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

inline std::string optional_text_state(esphome::text::Text **configs, int index) {
  return (configs != nullptr && configs[index] != nullptr) ? configs[index]->state : "";
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    esphome::text::Text **sp_ext2_configs,
    esphome::text::Text **sp_ext3_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  ESP_LOGI("sensors", "Phase 2: subscriptions + subpages start (%lu ms)", esphome::millis());
  int NS = bounded_grid_slots(cfg.num_slots);
  int COLS = cfg.cols > 0 ? cfg.cols : 1;
  if (NS != cfg.num_slots) {
    ESP_LOGW("sensors", "Grid slot count %d exceeds max %d; ignoring extra slots",
      cfg.num_slots, MAX_GRID_SLOTS);
  }
  int ROWS = (NS + COLS - 1) / COLS;

  static bool has_sensor[MAX_GRID_SLOTS] = {};
  static bool has_icon_on[MAX_GRID_SLOTS] = {};
  static const char* icon_off_cp[MAX_GRID_SLOTS] = {};
  static const char* icon_on_cp[MAX_GRID_SLOTS] = {};

  static bool sp_child_was_on[MAX_SUBPAGE_ITEMS] = {};
  static std::string sp_entity_ids[MAX_SUBPAGE_ITEMS];
  static int sp_child_alloc_idx = 0;
  static int sp_entity_alloc_idx = 0;
  sp_child_alloc_idx = 0;
  sp_entity_alloc_idx = 0;
  memset(has_sensor, 0, sizeof(has_sensor));
  memset(has_icon_on, 0, sizeof(has_icon_on));
  clear_internal_relay_watchers();

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
    if (p.type == "calendar") {
      subscribe_calendar_date_source(p.entity);
      continue;
    }
    if (p.type == "weather") {
      if (!p.entity.empty())
        subscribe_weather_state(s.icon_lbl, s.text_lbl, p.entity);
      continue;
    }
    if (p.type == "garage") {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Garage Door" : p.label);
        subscribe_garage_state(s.btn, s.icon_lbl, status_label,
          garage_closed_icon(p.icon), garage_open_icon(p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
      if (!p.entity.empty()) {
        TransientStatusLabel *status_label = create_transient_status_label(
          s.text_lbl, p.label.empty() ? "Cover" : p.label);
        subscribe_cover_toggle_state(s.btn, s.icon_lbl, status_label,
          slider_icon_off(p.type, p.entity, p.icon), slider_icon_on(p.type, p.icon_on), p.entity);
        if (p.label.empty())
          subscribe_friendly_name(status_label, p.entity);
      }
      continue;
    }
    if (p.type == "internal") {
      if (!p.entity.empty() && !internal_relay_push_mode(p)) {
        bool internal_has_icon_on = !p.icon_on.empty() && p.icon_on != "Auto";
        const char *internal_icon_on = internal_has_icon_on ? find_icon(p.icon_on.c_str()) : nullptr;
        watch_internal_relay_state(p.entity, s.btn, s.icon_lbl,
          internal_has_icon_on, internal_relay_icon(p, false), internal_icon_on);
      }
      continue;
    }

    if (p.entity.empty()) continue;

    if (p.type == "slider" || p.type == "cover") {
      lv_obj_t *slider = (lv_obj_t *)lv_obj_get_user_data(s.sensor_container);
      bool sl_has_icon_on = slider_has_alt_icon(p.type, p.icon_on);
      const char *sl_icon_on_cp = sl_has_icon_on ? slider_icon_on(p.type, p.icon_on) : nullptr;
      const char *sl_icon_off_cp = sl_has_icon_on ? slider_icon_off(p.type, p.entity, p.icon) : nullptr;
      subscribe_slider_state(s.btn, s.icon_lbl, slider,
        sl_has_icon_on, sl_icon_off_cp, sl_icon_on_cp, p.entity,
        p.type == "cover" && cover_tilt_mode(p.sensor));
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

  static int sp_on_count[MAX_GRID_SLOTS] = {};
  memset(sp_on_count, 0, sizeof(sp_on_count));

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

    std::string sp_cfg = optional_text_state(sp_configs, si) +
      optional_text_state(sp_ext_configs, si) +
      optional_text_state(sp_ext2_configs, si) +
      optional_text_state(sp_ext3_configs, si);
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
    configure_button_label_wrap(bl);

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
      configure_button_label_wrap(stl);

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

      } else if (sb.type == "calendar") {
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
        lv_label_set_text(sul, "");

        lv_label_set_text(stl, "Date");
        register_calendar_card(svl, sul, stl, sb.precision == "datetime");
        subscribe_calendar_date_source(sb.entity);

      } else if (sb.type == "weather") {
        if (has_sensor_color)
          lv_obj_set_style_bg_color(sb_btn, lv_color_hex(sensor_val),
            static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_DEFAULT));
        lv_obj_clear_flag(sb_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_label_set_text(sil, find_icon("Weather Cloudy"));
        lv_label_set_text(stl, "Weather");
        if (!sb.entity.empty())
          subscribe_weather_state(sil, stl, sb.entity);

      } else if (sb.type == "cover" && cover_toggle_mode(sb.sensor)) {
        lv_label_set_text(sil, slider_icon_off(sb.type, sb.entity, sb.icon));
        lv_label_set_text(stl, sb.label.empty() ? "Cover" : sb.label.c_str());
        if (!sb.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            stl, sb.label.empty() ? "Cover" : sb.label);
          subscribe_cover_toggle_state(sb_btn, sil, status_label,
            slider_icon_off(sb.type, sb.entity, sb.icon), slider_icon_on(sb.type, sb.icon_on), sb.entity);
          if (sb.label.empty())
            subscribe_friendly_name(status_label, sb.entity);

          if (sp_indicator) {
            lv_obj_t *parent_btn = slots[si].btn;
            lv_obj_t *parent_icon = slots[si].icon_lbl;
            int parent_idx = si;
            int cwi = sp_child_alloc_idx++;
            if (cwi >= MAX_SUBPAGE_ITEMS) {
              ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb.entity.c_str());
            } else {
              sp_child_was_on[cwi] = false;
              subscribe_subpage_parent_indicator(
                sb.entity, parent_btn, parent_icon, parent_idx,
                &sp_child_was_on[cwi], sp_has_icon_on,
                sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
            }
          }

          int eid_idx = sp_entity_alloc_idx++;
          if (eid_idx >= MAX_SUBPAGE_ITEMS) {
            ESP_LOGW("sensors", "Too many subpage click handlers; skipping %s", sb.entity.c_str());
          } else {
            sp_entity_ids[eid_idx] = sb.entity;
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
              lv_obj_add_state(target, LV_STATE_CHECKED);
              std::string *en = (std::string *)lv_event_get_user_data(e);
              if (en && !en->empty()) send_toggle_action(*en);
            }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
          }
        }

      } else if (sb.type == "garage") {
        lv_label_set_text(sil, garage_closed_icon(sb.icon));
        lv_label_set_text(stl, sb.label.empty() ? "Garage Door" : sb.label.c_str());
        if (!sb.entity.empty()) {
          TransientStatusLabel *status_label = create_transient_status_label(
            stl, sb.label.empty() ? "Garage Door" : sb.label);
          subscribe_garage_state(sb_btn, sil, status_label,
            garage_closed_icon(sb.icon), garage_open_icon(sb.icon_on), sb.entity);
          if (sb.label.empty())
            subscribe_friendly_name(status_label, sb.entity);

          if (sp_indicator) {
            lv_obj_t *parent_btn = slots[si].btn;
            lv_obj_t *parent_icon = slots[si].icon_lbl;
            int parent_idx = si;
            int cwi = sp_child_alloc_idx++;
            if (cwi >= MAX_SUBPAGE_ITEMS) {
              ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb.entity.c_str());
            } else {
              sp_child_was_on[cwi] = false;
              subscribe_subpage_parent_indicator(
                sb.entity, parent_btn, parent_icon, parent_idx,
                &sp_child_was_on[cwi], sp_has_icon_on,
                sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
            }
          }

          int eid_idx = sp_entity_alloc_idx++;
          if (eid_idx >= MAX_SUBPAGE_ITEMS) {
            ESP_LOGW("sensors", "Too many subpage click handlers; skipping %s", sb.entity.c_str());
          } else {
            sp_entity_ids[eid_idx] = sb.entity;
            lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
              lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
              lv_obj_add_state(target, LV_STATE_CHECKED);
              std::string *en = (std::string *)lv_event_get_user_data(e);
              if (en && !en->empty()) send_toggle_action(*en);
            }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
          }
        }

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

      } else if (sb.type == "internal") {
        ParsedCfg ip;
        ip.entity = sb.entity;
        ip.label = sb.label;
        ip.icon = sb.icon;
        ip.icon_on = sb.icon_on;
        ip.sensor = sb.sensor;
        ip.type = sb.type;

        bool push_mode = internal_relay_push_mode(ip);
        const char *internal_icon_off = internal_relay_icon(ip, push_mode);
        lv_label_set_text(sil, internal_icon_off);
        std::string internal_label = internal_relay_label(ip);
        lv_label_set_text(stl, internal_label.c_str());

        if (push_mode) {
          apply_push_button_transition(sb_btn);
        } else if (!sb.entity.empty()) {
          bool internal_has_icon_on = !sb.icon_on.empty() && sb.icon_on != "Auto";
          const char *internal_icon_on = internal_has_icon_on ? find_icon(sb.icon_on.c_str()) : nullptr;

          bool *child_was_on = nullptr;
          lv_obj_t *parent_btn = nullptr;
          lv_obj_t *parent_icon = nullptr;
          if (sp_indicator) {
            int cwi = sp_child_alloc_idx++;
            if (cwi >= MAX_SUBPAGE_ITEMS) {
              ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb.entity.c_str());
            } else {
              sp_child_was_on[cwi] = false;
              child_was_on = &sp_child_was_on[cwi];
              parent_btn = slots[si].btn;
              parent_icon = slots[si].icon_lbl;
            }
          }

          watch_internal_relay_state(
            sb.entity, sb_btn, sil,
            internal_has_icon_on, internal_icon_off, internal_icon_on,
            child_was_on, parent_btn, parent_icon, si,
            sp_has_icon_on, sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
        }

        if (!sb.entity.empty()) {
          InternalRelayClickCtx *ctx = new InternalRelayClickCtx();
          ctx->key = sb.entity;
          ctx->push_mode = push_mode;
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            InternalRelayClickCtx *c = (InternalRelayClickCtx *)lv_event_get_user_data(e);
            if (c && !c->key.empty()) send_internal_relay_action(c->key, c->push_mode);
          }, LV_EVENT_CLICKED, ctx);
        }

      } else if ((sb.type == "slider" || sb.type == "cover") && !sb.entity.empty()) {
        lv_obj_t *sl = setup_subpage_slider(sb_btn, sil, stl, sb, has_on ? on_val : DEFAULT_SLIDER_COLOR, sp_radius);

        if (sp_indicator) {
          lv_obj_t *parent_btn = slots[si].btn;
          lv_obj_t *parent_icon = slots[si].icon_lbl;
          int parent_idx = si;
          int cwi = sp_child_alloc_idx++;
          if (cwi >= MAX_SUBPAGE_ITEMS) {
            ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb.entity.c_str());
          } else {
            sp_child_was_on[cwi] = false;
            subscribe_subpage_parent_indicator(
              sb.entity, parent_btn, parent_icon, parent_idx,
              &sp_child_was_on[cwi], sp_has_icon_on,
              sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
          }
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
          int cwi = sp_child_alloc_idx++;
          if (cwi >= MAX_SUBPAGE_ITEMS) {
            ESP_LOGW("sensors", "Too many subpage state indicators; skipping %s", sb.entity.c_str());
          } else {
            sp_child_was_on[cwi] = false;
            subscribe_subpage_parent_indicator(
              sb.entity, parent_btn, parent_icon, parent_idx,
              &sp_child_was_on[cwi], sp_has_icon_on,
              sp_icon_off_glyph, sp_icon_on_glyph, sp_on_count);
          }
        }

        int eid_idx = sp_entity_alloc_idx++;
        if (eid_idx >= MAX_SUBPAGE_ITEMS) {
          ESP_LOGW("sensors", "Too many subpage click handlers; skipping %s", sb.entity.c_str());
        } else {
          sp_entity_ids[eid_idx] = sb.entity;
          lv_obj_add_event_cb(sb_btn, [](lv_event_t *e) {
            std::string *en = (std::string *)lv_event_get_user_data(e);
            if (en && !en->empty()) send_toggle_action(*en);
          }, LV_EVENT_CLICKED, &sp_entity_ids[eid_idx]);
        }
      } else {
        lv_label_set_text(stl, sb.label.empty() ? "Configure" : sb.label.c_str());
      }
    }

    lv_obj_set_user_data(slots[si].btn, (void *)sub_scr);
  }
  ESP_LOGI("sensors", "Phase 2: done (%lu ms)", esphome::millis());
}

inline void grid_phase2(
    BtnSlot *slots, const GridConfig &cfg,
    esphome::text::Text **sp_configs,
    esphome::text::Text **sp_ext_configs,
    const std::string &order_str,
    const std::string &on_hex, const std::string &off_hex,
    const std::string &sensor_hex,
    lv_obj_t *main_page_obj) {
  grid_phase2(slots, cfg, sp_configs, sp_ext_configs, nullptr, nullptr,
    order_str, on_hex, off_hex, sensor_hex, main_page_obj);
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
