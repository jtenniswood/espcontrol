#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

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

inline void send_turn_off_action(const std::string &entity_id) {
  if (entity_id.empty()) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)("homeassistant.turn_off");
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(entity_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline bool action_card_requires_value(const std::string &action) {
  return action == "input_number.set_value" || action == "input_select.select_option";
}

inline const char *action_card_value_key(const std::string &action) {
  if (action == "input_number.set_value") return "value";
  if (action == "input_select.select_option") return "option";
  return nullptr;
}

inline bool action_card_action_allowed(const std::string &action) {
  return action == "scene.turn_on" ||
         action == "script.turn_on" ||
         action == "automation.trigger" ||
         action == "button.press" ||
         action == "input_button.press" ||
         action == "input_boolean.toggle" ||
         action == "input_boolean.turn_on" ||
         action == "input_boolean.turn_off" ||
         action_card_requires_value(action);
}

inline void send_action_card_action(const ParsedCfg &p) {
  if (p.entity.empty() || p.sensor.empty() || !action_card_action_allowed(p.sensor)) return;
  const char *value_key = action_card_value_key(p.sensor);
  if (value_key && p.unit.empty()) return;

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(p.sensor.c_str());
  req.is_event = false;
  req.data.init(value_key ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(p.entity.c_str());
  if (value_key) {
    auto &value_kv = req.data.emplace_back();
    value_kv.key = decltype(value_kv.key)(value_key);
    value_kv.value = decltype(value_kv.value)(p.unit.c_str());
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_lock_action(const std::string &entity_id, const std::string &state) {
  if (entity_id.empty()) return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(state == "locked" ? "lock.unlock" : "lock.lock");
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(entity_id.c_str());
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_lock_action(LockCardCtx *ctx) {
  if (!ctx) return;
  send_lock_action(ctx->entity_id, ctx->state);
}

inline void send_lock_command_action(const ParsedCfg &p) {
  if (p.entity.empty() || esphome::api::global_api_server == nullptr) return;
  const char *service = nullptr;
  if (p.sensor == "lock") service = "lock.lock";
  else if (p.sensor == "unlock") service = "lock.unlock";
  if (service == nullptr) return;

  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(1);
  auto &kv = req.data.emplace_back();
  kv.key = decltype(kv.key)("entity_id");
  kv.value = decltype(kv.value)(p.entity.c_str());
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

inline bool cover_command_mode(const std::string &sensor) {
  return sensor == "open" || sensor == "close" ||
         sensor == "stop" || sensor == "set_position";
}

inline const char *cover_command_service(const std::string &sensor) {
  if (sensor == "open") return "cover.open_cover";
  if (sensor == "close") return "cover.close_cover";
  if (sensor == "stop") return "cover.stop_cover";
  if (sensor == "set_position") return "cover.set_cover_position";
  return nullptr;
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

inline void send_cover_command_action(const ParsedCfg &p) {
  const char *service = cover_command_service(p.sensor);
  if (p.entity.empty() || service == nullptr || esphome::api::global_api_server == nullptr) return;

  bool has_position = p.sensor == "set_position";
  bool wants_stop_response = p.sensor == "stop";
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  if (wants_stop_response) {
    req.call_id = next_cover_stop_call_id();
    req.wants_response = true;
  }
  req.data.init(has_position ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(p.entity.c_str());
  if (has_position) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", cover_position_value(p.unit));
    auto &position_kv = req.data.emplace_back();
    position_kv.key = decltype(position_kv.key)("position");
    position_kv.value = decltype(position_kv.value)(buf);
  }
  if (wants_stop_response) {
    std::string entity_id = p.entity;
    esphome::api::global_api_server->register_action_response_callback(
      req.call_id,
      [entity_id](const esphome::api::ActionResponse &response) {
        if (response.is_success()) return;
        ESP_LOGW("cover", "cover.stop_cover failed for %s: %s; falling back to cover toggle",
                 entity_id.c_str(), response.get_error_message().c_str());
        send_toggle_action(entity_id);
      });
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
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
  if (entity_id.empty()) return;
  esphome::api::HomeassistantActionRequest req;
  req.is_event = false;
  req.service = decltype(req.service)("light.turn_on");
  req.data.init(2);
  auto &kv1 = req.data.emplace_back();
  kv1.key = decltype(kv1.key)("entity_id");
  kv1.value = decltype(kv1.value)(entity_id.c_str());
  auto &kv2 = req.data.emplace_back();
  kv2.key = decltype(kv2.key)("color_temp_kelvin");
  int kelvin = min_k + (pct * (max_k - min_k)) / 100;
  if (kelvin < min_k) kelvin = min_k;
  if (kelvin > max_k) kelvin = max_k;
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", kelvin);
  kv2.value = decltype(kv2.value)(buf);
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline const char *light_temp_icon(const std::string &icon) {
  return (!icon.empty() && icon != "Auto") ? find_icon(icon.c_str()) : find_icon("Lightbulb");
}

inline std::string media_card_mode(const std::string &sensor) {
  if (sensor == "volume" || sensor == "position" ||
      sensor == "now_playing" || sensor == "play_pause" ||
      sensor == "previous" || sensor == "next")
    return sensor;
  if (sensor == "controls") return "play_pause";
  return "play_pause";
}

inline bool media_playback_button_mode(const std::string &mode) {
  return mode == "play_pause" || mode == "previous" || mode == "next";
}

inline const char *media_service_for_mode(const std::string &mode) {
  if (mode == "previous") return "media_player.media_previous_track";
  if (mode == "next") return "media_player.media_next_track";
  return "media_player.media_play_pause";
}

inline void send_media_player_action(const std::string &entity_id,
                                     const char *service,
                                     const char *value_key = nullptr,
                                     const char *value = nullptr) {
  if (entity_id.empty() || service == nullptr || service[0] == '\0') return;
  esphome::api::HomeassistantActionRequest req;
  req.service = decltype(req.service)(service);
  req.is_event = false;
  req.data.init(value_key && value ? 2 : 1);
  auto &entity_kv = req.data.emplace_back();
  entity_kv.key = decltype(entity_kv.key)("entity_id");
  entity_kv.value = decltype(entity_kv.value)(entity_id.c_str());
  if (value_key && value) {
    auto &value_kv = req.data.emplace_back();
    value_kv.key = decltype(value_kv.key)(value_key);
    value_kv.value = decltype(value_kv.value)(value);
  }
  esphome::api::global_api_server->send_homeassistant_action(req);
}

inline void send_media_volume_action(const std::string &entity_id, int value) {
  if (value < 0) value = 0;
  if (value > 100) value = 100;
  char buf[12];
  snprintf(buf, sizeof(buf), "%d.%02d", value / 100, value % 100);
  send_media_player_action(entity_id, "media_player.volume_set", "volume_level", buf);
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

inline void send_media_playback_action(const std::string &entity_id,
                                       const std::string &mode) {
  if (entity_id.empty()) return;
  send_media_player_action(entity_id, media_service_for_mode(mode));
}

// ── Button click dispatch ─────────────────────────────────────────────

struct MediaVolumeCtx;
inline void media_volume_open_modal(MediaVolumeCtx *ctx);
struct ClimateControlCtx;
inline void climate_control_open_modal(ClimateControlCtx *ctx);
inline void switch_confirmation_open_modal(const ParsedCfg &p, lv_obj_t *btn_obj);

// Handle a main-grid button press: dispatch push event, subpage nav,
// slider toggle, or entity toggle based on the config string.
inline void handle_button_click(const std::string &cfg, int slot_num,
                                lv_obj_t *btn_obj) {
  if (btn_obj && lv_obj_has_state(btn_obj, LV_STATE_DISABLED)) return;
  ParsedCfg p = parse_cfg(cfg);
  if (p.type == "sensor" || p.type == "text_sensor" ||
      p.type == "calendar" || p.type == "timezone" ||
      p.type == "weather_forecast") return;
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
    if (garage_command_mode(p.sensor)) {
      send_cover_command_action(p);
    } else if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "lock") {
    if (lock_command_mode(p.sensor)) {
      send_lock_command_action(p);
    } else {
      LockCardCtx *ctx = (LockCardCtx *)lv_obj_get_user_data(btn_obj);
      if (ctx) send_lock_action(ctx);
      else send_lock_action(p.entity, "");
    }
  } else if (p.type == "cover" && cover_command_mode(p.sensor)) {
    send_cover_command_action(p);
  } else if (p.type == "cover" && cover_toggle_mode(p.sensor)) {
    if (!p.entity.empty()) {
      lv_obj_add_state(btn_obj, LV_STATE_CHECKED);
      send_toggle_action(p.entity);
    }
  } else if (p.type == "internal") {
    if (!p.entity.empty()) send_internal_relay_action(p);
  } else if (p.type == "action") {
    send_action_card_action(p);
  } else if (p.type == "media") {
    std::string mode = media_card_mode(p.sensor);
    if (mode == "volume") {
      MediaVolumeCtx *ctx = (MediaVolumeCtx *)lv_obj_get_user_data(btn_obj);
      if (ctx) media_volume_open_modal(ctx);
    } else if (mode == "now_playing" && p.precision == "play_pause") {
      send_media_playback_action(p.entity, "play_pause");
    } else if (media_playback_button_mode(mode)) {
      send_media_playback_action(p.entity, mode);
    }
  } else if (p.type == "climate") {
    ClimateControlCtx *ctx = (ClimateControlCtx *)lv_obj_get_user_data(btn_obj);
    if (ctx) climate_control_open_modal(ctx);
  } else if (p.type == "light_temperature") {
    // Tap does nothing; only dragging the slider sends commands.
  } else if (brightness_slider_type(p.type) || p.type == "cover") {
    if (!p.entity.empty()) send_slider_action(p.entity, -1, cover_tilt_mode(p.sensor));
  } else {
    if (!p.entity.empty()) {
      if (switch_confirmation_enabled(p) && btn_obj &&
          lv_obj_has_state(btn_obj, LV_STATE_CHECKED) &&
          !is_button_entity(p.entity)) {
        switch_confirmation_open_modal(p, btn_obj);
      } else {
        send_toggle_action(p.entity);
      }
    }
  }
}
