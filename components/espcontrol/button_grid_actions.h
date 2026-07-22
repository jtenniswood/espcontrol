#pragma once

#include "media_volume_capability.h"

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

// ── Home Assistant actions ────────────────────────────────────────────

inline std::function<void(lv_obj_t *)> &button_grid_screen_load_callback() {
  static std::function<void(lv_obj_t *)> callback = nullptr;
  return callback;
}

inline void button_grid_load_screen(lv_obj_t *screen) {
  if (!screen) return;
  lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
  lv_obj_update_layout(screen);
  auto &callback = button_grid_screen_load_callback();
  if (callback) callback(screen);
}

inline bool is_button_entity(const std::string &entity_id) {
  return entity_id.size() > 7 && entity_id.compare(0, 7, "button.") == 0;
}

// Press HA button entities; toggle other bound entities.
inline void send_toggle_action(const std::string &entity_id) {
  ha_send_entity_action(entity_id,
    is_button_entity(entity_id) ? "button.press" : "homeassistant.toggle");
}

inline void send_turn_off_action(const std::string &entity_id) {
  ha_send_entity_action(entity_id, "homeassistant.turn_off");
}

inline void send_turn_on_action(const std::string &entity_id) {
  ha_send_entity_action(entity_id, "homeassistant.turn_on");
}

inline bool action_card_requires_value(const std::string &action) {
  return action == "input_number.set_value";
}

inline const char *action_card_value_key(const std::string &action) {
  if (action == "input_number.set_value") return "value";
  return nullptr;
}

inline std::string action_card_trim_text(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(start, end - start);
}

inline bool action_card_script_field_name_valid(const std::string &name) {
  if (name.empty() || name == "entity_id") return false;
  for (char ch : name) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (!(std::isalnum(c) || ch == '_')) return false;
  }
  return true;
}

inline bool action_card_parse_script_field(const std::string &line,
                                           std::string &key,
                                           std::string &value) {
  size_t sep = line.find(':');
  size_t equals = line.find('=');
  if (sep == std::string::npos || (equals != std::string::npos && equals < sep)) {
    sep = equals;
  }
  if (sep == std::string::npos) return false;
  key = action_card_trim_text(line.substr(0, sep));
  value = action_card_trim_text(line.substr(sep + 1));
  return action_card_script_field_name_valid(key) && !value.empty();
}

struct ActionCardScriptField {
  std::string key;
  std::string value;
};

inline std::vector<ActionCardScriptField> action_card_script_fields(const std::string &options) {
  std::vector<ActionCardScriptField> out;
  std::string fields = cfg_option_value(options, "script_fields");
  if (fields.empty()) return out;
  size_t start = 0;
  while (start <= fields.size()) {
    size_t end = fields.find('\n', start);
    if (end == std::string::npos) end = fields.size();
    std::string key;
    std::string value;
    if (action_card_parse_script_field(fields.substr(start, end - start), key, value)) {
      out.push_back({key, value});
    }
    start = end + 1;
  }
  return out;
}

inline std::string action_card_script_fields_template(const std::vector<ActionCardScriptField> &fields) {
  if (fields.empty()) return "";
  std::string out = "{{ dict(";
  for (size_t i = 0; i < fields.size(); i++) {
    if (i > 0) out += ", ";
    out += fields[i].key + "=" + fields[i].key;
  }
  out += ") }}";
  return out;
}

inline void action_card_add_script_field_variables(esphome::api::HomeassistantActionRequest &req,
                                                   const std::vector<ActionCardScriptField> &fields) {
  for (const auto &field : fields) {
    ha_action_add_variable(req, field.key.c_str(), field.value.c_str());
  }
}

inline bool action_card_action_allowed(const std::string &action) {
  return action == "scene.turn_on" ||
         action == "script.turn_on" ||
         action == "automation.trigger" ||
         action == "button.press" ||
         action == "vacuum.start" ||
         action == "vacuum.return_to_base" ||
         action == "input_button.press" ||
         action == "input_boolean.toggle" ||
         action == "input_boolean.turn_on" ||
         action == "input_boolean.turn_off" ||
         action_card_option_select_action(action) ||
         action_card_requires_value(action);
}

inline void send_action_card_action(const ParsedCfg &p) {
  if (action_card_local_action(p)) {
    if (!p.entity.empty()) send_local_action(p.entity);
    return;
  }
  if (p.entity.empty() || p.sensor.empty() || !action_card_action_allowed(p.sensor)) return;
  if (action_card_option_select(p)) return;
  const char *value_key = action_card_value_key(p.sensor);
  if (value_key && p.unit.empty()) return;
  std::vector<ActionCardScriptField> script_fields = p.sensor == "script.turn_on"
    ? action_card_script_fields(p.options)
    : std::vector<ActionCardScriptField>();

  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, p.sensor.c_str(), false, 1 + (value_key ? 1 : 0))) return;
  std::string script_fields_template;
  if (!script_fields.empty()) {
    script_fields_template = action_card_script_fields_template(script_fields);
    if (!script_fields_template.empty()) {
      req.data_template.init(1);
      req.variables.init(script_fields.size());
    }
  }
  ha_action_add_entity(req, p.entity);
  if (value_key) {
    ha_action_add_data(req, value_key, p.unit.c_str());
  }
  if (!script_fields_template.empty()) {
    ha_action_add_data_template(req, "variables", script_fields_template.c_str());
    action_card_add_script_field_variables(req, script_fields);
  }
  ha_action_send(req);
}

using WebhookHeaders = std::vector<esphome::http_request::Header>;
using WebhookSender = std::function<bool(const std::string &, const std::string &,
                                         const std::string &, const WebhookHeaders &)>;

inline WebhookSender &webhook_sender() {
  static WebhookSender sender;
  return sender;
}

inline void register_webhook_sender(WebhookSender sender) {
  webhook_sender() = sender;
}

inline std::string trim_webhook_text(const std::string &value) {
  size_t start = 0;
  while (start < value.size() &&
         std::isspace(static_cast<unsigned char>(value[start]))) {
    start++;
  }
  size_t end = value.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    end--;
  }
  return value.substr(start, end - start);
}

inline bool webhook_header_name_valid(const std::string &name) {
  if (name.empty() || name.size() > 64) return false;
  for (char ch : name) {
    unsigned char c = static_cast<unsigned char>(ch);
    if (c <= 32 || c >= 127 || ch == ':') return false;
  }
  return true;
}

inline void webhook_add_header(WebhookHeaders &headers,
                               const std::string &name,
                               const std::string &value) {
  std::string trimmed_name = trim_webhook_text(name);
  std::string trimmed_value = trim_webhook_text(value);
  if (!webhook_header_name_valid(trimmed_name) || trimmed_value.size() > 256) return;
  esphome::http_request::Header header;
  header.name = trimmed_name;
  header.value = trimmed_value;
  headers.push_back(header);
}

inline bool webhook_has_header(const WebhookHeaders &headers, const char *name) {
  if (!name) return false;
  std::string wanted = name;
  for (char &ch : wanted) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  for (const auto &header : headers) {
    std::string actual = header.name;
    for (char &ch : actual) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if (actual == wanted) return true;
  }
  return false;
}

inline bool webhook_body_looks_json(const std::string &body) {
  for (char ch : body) {
    if (std::isspace(static_cast<unsigned char>(ch))) continue;
    return ch == '{' || ch == '[';
  }
  return false;
}

inline WebhookHeaders parse_webhook_headers(const std::string &value,
                                            const std::string &body) {
  WebhookHeaders headers;
  size_t start = 0;
  while (start <= value.size() && headers.size() < 8) {
    size_t end = value.find(';', start);
    if (end == std::string::npos) end = value.size();
    std::string part = trim_webhook_text(value.substr(start, end - start));
    size_t colon = part.find(':');
    if (colon != std::string::npos) {
      webhook_add_header(headers, part.substr(0, colon), part.substr(colon + 1));
    }
    start = end + 1;
  }
  if (!body.empty() && !webhook_has_header(headers, "Content-Type")) {
    webhook_add_header(headers, "Content-Type",
                       webhook_body_looks_json(body) ? "application/json" : "text/plain");
  }
  return headers;
}

inline void send_webhook_action(const ParsedCfg &p) {
  std::string url = trim_webhook_text(p.entity);
  if (url.empty()) {
    ESP_LOGW("webhook", "Webhook card has no URL");
    return;
  }
  std::string method = normalize_webhook_method(p.sensor);
  std::string body = (method == "GET" || method == "DELETE") ? "" : p.unit;
  WebhookHeaders headers = parse_webhook_headers(webhook_card_headers(p), body);
  WebhookSender &sender = webhook_sender();
  if (!sender) {
    ESP_LOGW("webhook", "Webhook sender is not registered");
    return;
  }
  ESP_LOGI("webhook", "Calling webhook with method %s", method.c_str());
  sender(url, method, body, headers);
}

inline void send_lock_action(const std::string &entity_id, const std::string &state) {
  ha_send_entity_action(entity_id, card_runtime_lock_toggle_service(state));
}

inline void send_lock_action(LockCardCtx *ctx) {
  if (!ctx) return;
  send_lock_action(ctx->entity_id, ctx->state);
}

inline void send_lock_command_action(const ParsedCfg &p) {
  if (p.entity.empty()) return;
  const char *service = card_runtime_lock_command_service(p.sensor);
  if (service == nullptr) return;

  ha_send_entity_action(p.entity, service);
}

// ── Slider card helpers ────────────────────────────────────────────────

inline bool is_cover_entity(const std::string &entity_id) {
  return entity_id.size() > 6 && entity_id.compare(0, 6, "cover.") == 0;
}

inline bool is_fan_entity(const std::string &entity_id) {
  return entity_id.size() > 4 && entity_id.compare(0, 4, "fan.") == 0;
}

inline bool cover_toggle_mode(const std::string &sensor) {
  return card_runtime_cover_toggle_mode(sensor);
}

inline bool cover_tilt_mode(const std::string &sensor) {
  return card_runtime_cover_tilt_mode(sensor);
}

inline bool cover_modal_mode(const std::string &sensor) {
  return card_runtime_cover_modal_mode(sensor);
}

inline bool cover_command_mode(const std::string &sensor) {
  return card_runtime_cover_command_mode(sensor);
}

inline const char *cover_command_service(const std::string &sensor) {
  return card_runtime_cover_command_service(sensor);
}

constexpr int COVER_SUPPORT_OPEN = 1;
constexpr int COVER_SUPPORT_CLOSE = 2;
constexpr int COVER_SUPPORT_SET_POSITION = 4;
constexpr int COVER_SUPPORT_STOP = 8;
constexpr int COVER_SUPPORT_OPEN_TILT = 16;
constexpr int COVER_SUPPORT_CLOSE_TILT = 32;
constexpr int COVER_SUPPORT_STOP_TILT = 64;
constexpr int COVER_SUPPORT_SET_TILT_POSITION = 128;

inline bool cover_parse_supported_features(esphome::StringRef val, int &features) {
  std::string value = normalized_state_text(val);
  if (value.empty() || value == "none" || value == "null" ||
      value == "unknown" || value == "unavailable") {
    return false;
  }
  char *end = nullptr;
  long parsed = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) return false;
  features = static_cast<int>(parsed);
  return true;
}

inline const char *cover_tilt_command_service(const std::string &sensor) {
  if (sensor == "open") return "cover.open_cover_tilt";
  if (sensor == "close") return "cover.close_cover_tilt";
  if (sensor == "stop") return "cover.stop_cover_tilt";
  if (sensor == "set_position") return "cover.set_cover_tilt_position";
  return nullptr;
}

inline int cover_command_normal_feature(const std::string &sensor) {
  if (sensor == "open") return COVER_SUPPORT_OPEN;
  if (sensor == "close") return COVER_SUPPORT_CLOSE;
  if (sensor == "stop") return COVER_SUPPORT_STOP;
  if (sensor == "set_position") return COVER_SUPPORT_SET_POSITION;
  return 0;
}

inline int cover_command_tilt_feature(const std::string &sensor) {
  if (sensor == "open") return COVER_SUPPORT_OPEN_TILT;
  if (sensor == "close") return COVER_SUPPORT_CLOSE_TILT;
  if (sensor == "stop") return COVER_SUPPORT_STOP_TILT;
  if (sensor == "set_position") return COVER_SUPPORT_SET_TILT_POSITION;
  return 0;
}

struct CoverCommandCtx {
  ParsedCfg config;
  bool supported_features_known = false;
  int supported_features = 0;
};

inline bool cover_command_use_tilt(const std::string &sensor,
                                   bool supported_features_known,
                                   int supported_features) {
  if (!supported_features_known) return false;
  int normal_feature = cover_command_normal_feature(sensor);
  int tilt_feature = cover_command_tilt_feature(sensor);
  return normal_feature != 0 && tilt_feature != 0 &&
         (supported_features & normal_feature) == 0 &&
         (supported_features & tilt_feature) != 0;
}

inline bool cover_command_supported(const std::string &sensor,
                                    bool supported_features_known,
                                    int supported_features) {
  if (!supported_features_known) return true;
  int normal_feature = cover_command_normal_feature(sensor);
  int tilt_feature = cover_command_tilt_feature(sensor);
  return (normal_feature != 0 && (supported_features & normal_feature) != 0) ||
         (tilt_feature != 0 && (supported_features & tilt_feature) != 0);
}

inline void cover_command_apply_supported_features(CoverCommandCtx *ctx, int features) {
  if (!ctx) return;
  ctx->supported_features_known = true;
  ctx->supported_features = features;
}

inline CoverCommandCtx *create_cover_command_context(const ParsedCfg &p) {
  CoverCommandCtx *ctx = new CoverCommandCtx();
  ctx->config = p;
  return ctx;
}

inline void subscribe_cover_command_features(CoverCommandCtx *ctx) {
  if (!ctx || ctx->config.entity.empty()) return;
  ha_subscribe_attribute(
    ctx->config.entity, std::string("supported_features"),
    std::function<void(esphome::StringRef)>(
      [ctx](esphome::StringRef val) {
        int features = 0;
        if (cover_parse_supported_features(val, features)) {
          cover_command_apply_supported_features(ctx, features);
        } else {
          ctx->supported_features_known = false;
          ctx->supported_features = 0;
        }
      })
  );
}

inline int cover_position_value(const std::string &value) {
  char *end = nullptr;
  long pos = std::strtol(value.c_str(), &end, 10);
  if (end == value.c_str()) pos = 50;
  if (pos < 0) pos = 0;
  if (pos > 100) pos = 100;
  return static_cast<int>(pos);
}

inline uint32_t next_cover_stop_call_id() {
  static uint32_t call_id = 200000;
  return call_id++;
}

inline uint32_t &cover_stop_pending_call_id() {
  static uint32_t call_id = 0;
  return call_id;
}

inline void cover_stop_clear_pending(uint32_t call_id) {
  if (cover_stop_pending_call_id() == call_id) cover_stop_pending_call_id() = 0;
}

inline bool cover_stop_track_pending(uint32_t call_id) {
  if (call_id == 0 || cover_stop_pending_call_id() != 0) return false;
  cover_stop_pending_call_id() = call_id;
  return true;
}

inline void cover_stop_cancel_pending_request() {
  uint32_t call_id = cover_stop_pending_call_id();
  if (call_id == 0) return;
  cover_stop_pending_call_id() = 0;
  ha_cancel_action_response_callback(call_id, "api disconnected");
}

inline void send_cover_command_action(const ParsedCfg &p, bool tilt_command = false) {
  const char *service = tilt_command
    ? cover_tilt_command_service(p.sensor)
    : cover_command_service(p.sensor);
  if (p.entity.empty() || service == nullptr) return;

  bool has_position = p.sensor == "set_position";
  bool wants_stop_response = p.sensor == "stop" && !tilt_command;
  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = wants_stop_response ? next_cover_stop_call_id() : 0;
  if (!ha_action_begin(req, service, false, has_position ? 2 : 1, call_id)) return;
  if (wants_stop_response) {
    req.call_id = call_id;
  }
  ha_action_add_entity(req, p.entity);
  if (has_position) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", cover_position_value(p.unit));
    ha_action_add_data(req, tilt_command ? "tilt_position" : "position", buf);
  }
  bool cover_stop_tracked = false;
  if (wants_stop_response) {
    std::string entity_id = p.entity;
    cover_stop_tracked = cover_stop_track_pending(req.call_id);
    if (cover_stop_tracked) {
      if (!ha_register_action_response_callback(
        req.call_id,
        [entity_id, call_id = req.call_id](const esphome::api::ActionResponse &response) {
          cover_stop_clear_pending(call_id);
          if (response.is_success()) return;
          ESP_LOGW("cover", "cover.stop_cover failed for %s: %s; falling back to cover toggle",
                   entity_id.c_str(), response.get_error_message().c_str());
          send_toggle_action(entity_id);
        })) {
        cover_stop_clear_pending(req.call_id);
        cover_stop_tracked = false;
      }
    }
  }
  if (!ha_action_send(req) && cover_stop_tracked) {
    cover_stop_clear_pending(req.call_id);
    ha_cancel_action_response_callback(req.call_id, "send failed");
  }
}

inline void send_cover_command_action(const std::string &entity_id,
                                      const std::string &mode,
                                      bool tilt_command = false) {
  ParsedCfg p;
  p.entity = entity_id;
  p.sensor = mode;
  send_cover_command_action(p, tilt_command);
}

inline void send_cover_command_action(const CoverCommandCtx &ctx) {
  const ParsedCfg &p = ctx.config;
  if (!cover_command_supported(p.sensor, ctx.supported_features_known, ctx.supported_features)) {
    ESP_LOGW("cover", "Cover command %s is not supported for %s",
             p.sensor.c_str(), p.entity.c_str());
    return;
  }
  send_cover_command_action(
    p,
    cover_command_use_tilt(p.sensor, ctx.supported_features_known, ctx.supported_features));
}

// Send HA action for a slider change: toggle (value<0), brightness, or cover position/tilt
inline void send_slider_action(const std::string &entity_id, int value, bool cover_tilt = false) {
  esphome::api::HomeassistantActionRequest req;
  if (value < 0) {
    if (!ha_action_begin(req, "homeassistant.toggle", false, 1)) return;
    ha_action_add_entity(req, entity_id);
  } else if (is_cover_entity(entity_id)) {
    if (!ha_action_begin(req,
      cover_tilt ? "cover.set_cover_tilt_position" : "cover.set_cover_position",
      false, 2)) return;
    ha_action_add_entity(req, entity_id);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ha_action_add_data(req, cover_tilt ? "tilt_position" : "position", buf);
  } else if (is_fan_entity(entity_id)) {
    if (value == 0) {
      if (!ha_action_begin(req, "fan.turn_off", false, 1)) return;
      ha_action_add_entity(req, entity_id);
    } else {
      if (!ha_action_begin(req, "fan.turn_on", false, 2)) return;
      ha_action_add_entity(req, entity_id);
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", value);
      ha_action_add_data(req, "percentage", buf);
    }
  } else if (value == 0) {
    if (!ha_action_begin(req, "light.turn_off", false, 1)) return;
    ha_action_add_entity(req, entity_id);
  } else {
    if (!ha_action_begin(req, "light.turn_on", false, 2)) return;
    ha_action_add_entity(req, entity_id);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", value);
    ha_action_add_data(req, "brightness_pct", buf);
  }
  ha_action_send(req);
}

// Parse "min-max" kelvin range from the unit config field (e.g. "2000-6500").
inline void parse_kelvin_range(const std::string &unit, int &min_k, int &max_k) {
  min_k = 2000; max_k = 6500;
  if (unit.empty()) return;
  auto dash = unit.find('-');
  if (dash == std::string::npos || dash == 0) return;
  int a = atoi(unit.substr(0, dash).c_str());
  int b = atoi(unit.substr(dash + 1).c_str());
  if (a >= 1000 && b > a) { min_k = a; max_k = b; }
}

// Map a kelvin value to an lv_color_t by lerping between warm amber (low K) and
// cool blue-white (high K). Used when "Show light colour on card" is enabled.
//
// The interpolation is anchored to an absolute reference range
// (KELVIN_REF_MIN..KELVIN_REF_MAX) rather than the user's configured slider
// range. That way a narrow range (e.g. 5000-6000K) shows the actual subtle
// shift between two cool-white shades — instead of stretching the full
// amber-to-blue gradient across two values that should both look cool.
inline lv_color_t kelvin_to_fill_color(int k, int /*min_k*/, int /*max_k*/) {
  constexpr int KELVIN_REF_MIN = 2000;
  constexpr int KELVIN_REF_MAX = 6500;
  float t = (float)(k - KELVIN_REF_MIN) /
            (float)(KELVIN_REF_MAX - KELVIN_REF_MIN);
  if (t < 0.0f) t = 0.0f;
  if (t > 1.0f) t = 1.0f;
  // warm = 0xFF8012, cool = 0xB8CCFF
  uint8_t r = (uint8_t)(0xFF + t * (float)(0xB8 - 0xFF) + 0.5f);
  uint8_t g = (uint8_t)(0x80 + t * (float)(0xCC - 0x80) + 0.5f);
  uint8_t b = (uint8_t)(0x12 + t * (float)(0xFF - 0x12) + 0.5f);
  return lv_color_make(r, g, b);
}

// Send light.turn_on with color_temp_kelvin mapped from 0-100 pct over [min_k, max_k].
inline void send_light_temp_action(const std::string &entity_id, int pct, int min_k, int max_k) {
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, "light.turn_on", false, 2)) return;
  if (entity_id.empty()) return;
  ha_action_add_entity(req, entity_id);
  int kelvin = min_k + (pct * (max_k - min_k)) / 100;
  if (kelvin < min_k) kelvin = min_k;
  if (kelvin > max_k) kelvin = max_k;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", kelvin);
  ha_action_add_data(req, "color_temp_kelvin", buf);
  ha_action_send(req);
}

inline void send_light_color_name_action(const std::string &entity_id, const char *color_name) {
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, "light.turn_on", false, 2)) return;
  if (entity_id.empty() || !color_name || color_name[0] == '\0') return;
  ha_action_add_entity(req, entity_id);
  ha_action_add_data(req, "color_name", color_name);
  ha_action_send(req);
}

inline void send_light_rgb_action(const std::string &entity_id, uint32_t color) {
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, "light.turn_on", false, 1)) return;
  if (entity_id.empty()) return;
  req.data_template.init(1);
  req.variables.init(3);
  ha_action_add_entity(req, entity_id);
  ha_action_add_data_template(req, "rgb_color", "{{ [red | int, green | int, blue | int] }}");
  char red[4];
  char green[4];
  char blue[4];
  snprintf(red, sizeof(red), "%u", static_cast<unsigned>((color >> 16) & 0xFF));
  snprintf(green, sizeof(green), "%u", static_cast<unsigned>((color >> 8) & 0xFF));
  snprintf(blue, sizeof(blue), "%u", static_cast<unsigned>(color & 0xFF));
  ha_action_add_variable(req, "red", red);
  ha_action_add_variable(req, "green", green);
  ha_action_add_variable(req, "blue", blue);
  ha_action_send(req);
}

inline const char *light_temp_icon(const std::string &icon) {
  return (!icon.empty() && icon != "Auto") ? find_icon(icon.c_str()) : find_icon("Lightbulb");
}

inline std::string media_card_mode(const std::string &sensor) {
  return card_runtime_media_mode(sensor);
}

inline bool media_playback_button_mode(const std::string &mode) {
  return card_runtime_media_playback_button_mode(mode);
}

inline const char *media_service_for_mode(const std::string &mode) {
  return card_runtime_media_playback_service(mode);
}

inline void send_media_player_action(const std::string &entity_id,
                                     const char *service,
                                     const char *value_key = nullptr,
                                     const char *value = nullptr) {
  if (entity_id.empty() || service == nullptr || service[0] == '\0') return;
  esphome::api::HomeassistantActionRequest req;
  if (!ha_action_begin(req, service, false, value_key && value ? 2 : 1)) return;
  ha_action_add_entity(req, entity_id);
  if (value_key && value) {
    ha_action_add_data(req, value_key, value);
  }
  ha_action_send(req);
}

inline void send_media_volume_action(const std::string &entity_id, int value) {
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  char buf[12];
  snprintf(buf, sizeof(buf), "%d.%02d", value / 100, value % 100);
  send_media_player_action(entity_id, "media_player.volume_set", "volume_level", buf);
}

inline void send_media_volume_command(
    const std::string &entity_id,
    const espcontrol::media::VolumeCommand &command) {
  using espcontrol::media::VolumeCommandKind;
  if (command.kind == VolumeCommandKind::SET_ABSOLUTE) {
    send_media_volume_action(entity_id, command.value);
  } else if (command.kind == VolumeCommandKind::STEP_DOWN) {
    send_media_player_action(entity_id, "media_player.volume_down");
  } else if (command.kind == VolumeCommandKind::STEP_UP) {
    send_media_player_action(entity_id, "media_player.volume_up");
  }
}

inline void send_media_seek_action(const std::string &entity_id, int value, float duration) {
  if (duration <= 0.0f) return;
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  int seconds = (int)((duration * value / 100.0f) + 0.5f);
  char buf[16];
  snprintf(buf, sizeof(buf), "%d", seconds);
  send_media_player_action(entity_id, "media_player.media_seek", "seek_position", buf);
}

inline void send_media_source_action(const std::string &entity_id,
                                     const std::string &source) {
  if (entity_id.empty() || source.empty()) return;
  send_media_player_action(entity_id, "media_player.select_source", "source", source.c_str());
}

inline void send_media_playback_action(const std::string &entity_id,
                                       const std::string &mode) {
  if (entity_id.empty()) return;
  send_media_player_action(entity_id, media_service_for_mode(mode));
}

inline uint32_t next_media_playlist_call_id() {
  static uint32_t call_id = 400000;
  return call_id++;
}

inline void send_media_playlist_action(const ParsedCfg &p) {
  if (p.entity.empty()) return;
  const auto config = espcontrol::media::decode_config_v1(p);
  const std::string &content_id = config.playlist_content_id;
  if (content_id.empty()) {
    ESP_LOGW("media", "Playlist button for %s has no media content ID", p.entity.c_str());
    return;
  }
  const std::string &content_type = config.playlist_content_type;
  const std::string &player_source = config.playlist_player_source;
  ESP_LOGI("media", "Playlist button: entity=%s content_type=%s content_id=%s playback_device=%s",
           p.entity.c_str(), content_type.c_str(), content_id.c_str(),
           player_source.empty() ? "(none)" : player_source.c_str());
  if (!player_source.empty()) {
    esphome::api::HomeassistantActionRequest source_req;
    uint32_t source_call_id = next_media_playlist_call_id();
    if (ha_action_begin(source_req, "media_player.select_source", false, 2, source_call_id)) {
      ha_action_add_entity(source_req, p.entity);
      ha_action_add_data(source_req, "source", player_source.c_str());
      std::string entity_id = p.entity;
      std::string source = player_source;
      ha_register_action_response_callback(
        source_req.call_id,
        [entity_id, source](const esphome::api::ActionResponse &response) {
          if (response.is_success()) {
            ESP_LOGI("media", "media_player.select_source accepted for %s source=%s",
                     entity_id.c_str(), source.c_str());
            return;
          }
          ESP_LOGW("media", "media_player.select_source failed for %s source=%s: %s",
                   entity_id.c_str(), source.c_str(), response.get_error_message().c_str());
        });
      if (!ha_action_send(source_req)) {
        ha_cancel_action_response_callback(source_req.call_id, "send failed");
      }
    }
  }
  esphome::api::HomeassistantActionRequest req;
  uint32_t play_call_id = next_media_playlist_call_id();
  if (!ha_action_begin(req, "media_player.play_media", false, 3, play_call_id)) return;
  ha_action_add_entity(req, p.entity);
  ha_action_add_data(req, "media_content_id", content_id.c_str());
  ha_action_add_data(req, "media_content_type", content_type.c_str());
  std::string entity_id = p.entity;
  ha_register_action_response_callback(
    req.call_id,
    [entity_id, content_type, content_id](const esphome::api::ActionResponse &response) {
      if (response.is_success()) {
        ESP_LOGI("media", "media_player.play_media accepted for %s type=%s id=%s",
                 entity_id.c_str(), content_type.c_str(), content_id.c_str());
        return;
      }
      ESP_LOGW("media", "media_player.play_media failed for %s type=%s id=%s: %s",
               entity_id.c_str(), content_type.c_str(), content_id.c_str(),
               response.get_error_message().c_str());
    });
  if (!ha_action_send(req)) {
    ha_cancel_action_response_callback(req.call_id, "send failed");
  }
}

inline bool media_fast_press_mode(const std::string &mode) {
  return mode == "previous" || mode == "next";
}

inline bool *media_fast_press_slots() {
  static bool slots[MAX_GRID_SLOTS + 1] = {};
  return slots;
}

inline bool media_fast_press_consume(int slot_num) {
  if (slot_num <= 0 || slot_num > MAX_GRID_SLOTS) return false;
  bool *slots = media_fast_press_slots();
  bool sent = slots[slot_num];
  slots[slot_num] = false;
  return sent;
}

inline void handle_button_press(const std::string &cfg, int slot_num,
                                lv_obj_t *btn_obj) {
  (void) btn_obj;
  if (slot_num <= 0 || slot_num > MAX_GRID_SLOTS) return;
  ParsedCfg p = parse_cfg(cfg);
  if (p.type != "media") return;
  std::string mode = media_card_mode(p.sensor);
  if (!media_fast_press_mode(mode) || p.entity.empty()) return;
  media_fast_press_slots()[slot_num] = true;
  send_media_playback_action(p.entity, mode);
}

// ── Button click dispatch ─────────────────────────────────────────────

struct MediaVolumeCtx;
inline void media_volume_open_modal(MediaVolumeCtx *ctx);
struct MediaControlCtx;
inline void media_control_open_modal(MediaControlCtx *ctx);
inline MediaControlCtx *grid_media_control_runtime_for_owner(lv_obj_t *owner);
struct ClimateControlCtx;
inline void climate_control_open_modal(ClimateControlCtx *ctx);
struct ImageCardCtx;
inline void image_card_open_modal(ImageCardCtx *ctx);
inline void switch_confirmation_open_modal(const ParsedCfg &p, lv_obj_t *btn_obj, bool turn_on);
struct OptionSelectCtx;
inline void option_select_open_modal(OptionSelectCtx *ctx);
struct TodoCardCtx;
inline bool todo_card_context_valid(TodoCardCtx *ctx);
inline void todo_card_open_modal(TodoCardCtx *ctx);
struct AlarmCardCtx;
inline void alarm_card_open_page(AlarmCardCtx *ctx);
inline bool alarm_card_context_valid(AlarmCardCtx *ctx);
struct AlarmActionCtx;
inline void alarm_action_activate(AlarmActionCtx *action);
inline bool alarm_action_context_valid(AlarmActionCtx *action);
struct FanCardCtx;
inline bool fan_non_speed_card_type(const std::string &type);
inline void fan_card_handle_click(FanCardCtx *ctx);
inline void fan_control_open_modal(FanCardCtx *ctx);
struct CoverControlCtx;
inline void cover_control_open_modal(CoverControlCtx *ctx);
struct LightControlCtx;
inline void light_control_open_modal(LightControlCtx *ctx);

namespace espcontrol::cards {
inline bool basic_action_driver_handle_main_click(
    const Context &context, const ParsedCfg &config,
    int slot_number, lv_obj_t *button);
inline bool numeric_selectable_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool cleaning_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool access_cover_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool cover_modal_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool navigation_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool image_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool light_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool fan_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool climate_control_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool alarm_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool media_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button);
inline bool legacy_compatibility_driver_handle_main_click(
    const Context &context, lv_obj_t *button);
}

// Handle a main-grid button press: dispatch push event, subpage nav,
// slider toggle, or entity toggle based on the config string.
inline void handle_button_click(const std::string &cfg, int slot_num,
                                lv_obj_t *btn_obj) {
  (void) btn_obj;
  if (media_fast_press_consume(slot_num)) return;
  ParsedCfg p = parse_cfg(cfg);
  const auto context = card_runtime_context(p);
  ESP_LOGI("button", "Main button %d clicked: type=%s entity=%s mode=%s label=%s",
           slot_num, p.type.c_str(), p.entity.c_str(), p.sensor.c_str(), p.label.c_str());
  if (card_runtime_passive(context)) return;
  if (espcontrol::cards::basic_action_driver_handle_main_click(
        context, p, slot_num, btn_obj)) return;
  if (espcontrol::cards::numeric_selectable_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::cleaning_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::access_cover_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::cover_modal_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::navigation_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::image_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::light_control_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::fan_control_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::climate_control_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::alarm_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::media_driver_handle_main_click(
        context, p, btn_obj)) return;
  if (espcontrol::cards::legacy_compatibility_driver_handle_main_click(
        context, btn_obj)) return;
  ESP_LOGE("card_runtime", "Card has no main-grid action driver: type=%s",
           p.type.c_str());
}
