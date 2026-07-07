#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.

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

// ── Local action controls ─────────────────────────────────────────────
//
// Devices register named one-shot callbacks here at boot. The button type
// "local" dispatches to these by key, so device-specific addons (e.g. BLE
// keyboard) can be triggered from the grid without going through HA.

struct LocalActionControl {
  std::string key;
  std::string label;
  std::function<void()> action;
};

inline std::vector<LocalActionControl> &local_action_registry() {
  static std::vector<LocalActionControl> actions;
  return actions;
}

inline void register_local_action(
    const std::string &key, const std::string &label,
    std::function<void()> action) {
  if (key.empty()) return;
  LocalActionControl a;
  a.key = key;
  a.label = label;
  a.action = action;
  auto &reg = local_action_registry();
  for (auto &existing : reg) {
    if (existing.key == key) {
      existing = a;
      return;
    }
  }
  reg.push_back(a);
}

inline void send_local_action(const std::string &key) {
  for (auto &a : local_action_registry()) {
    if (a.key == key) {
      if (a.action) a.action();
      return;
    }
  }
  ESP_LOGW("espcontrol", "Local action '%s' not registered", key.c_str());
}

// ── Local sensor controls ─────────────────────────────────────────────
//
// Displays a live value from any ESPHome sensor/text_sensor on the device.
// The device auto-subscribes to sensor callbacks; send_local_sensor_update()
// is available as a fallback for computed/non-entity values.

struct LocalSensorControl {
  std::string key;
  bool is_text;
  int precision;
  lv_obj_t *sensor_lbl;
  lv_obj_t *text_lbl;
};

inline std::vector<LocalSensorControl> &local_sensor_registry() {
  static std::vector<LocalSensorControl> sensors;
  return sensors;
}

#ifdef USE_WEBSERVER
inline std::string local_endpoint_json_escape(const std::string &s) {
  std::string out;
  out.reserve(s.size() + 4);
  for (char c : s) {
    if (c == '"') out += "\\\"";
    else if (c == '\\') out += "\\\\";
    else out += c;
  }
  return out;
}

class LocalActionHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    esphome::StringRef url = request->url_to(url_buf);
    return strncmp(url.c_str(), "/local_actions", 14) == 0;
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    std::string json;
    json.reserve(256);
    json = "[";
    bool first = true;
    for (auto &a : local_action_registry()) {
      if (!first) json += ",";
      first = false;
      json += "{\"key\":\"" + local_endpoint_json_escape(a.key) +
              "\",\"label\":\"" + local_endpoint_json_escape(a.label) + "\"}";
    }
    json += "]";
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK) ESP_LOGE("espcontrol", "httpd_resp_send failed: %d", err);
  }
};

inline void register_local_action_endpoint() {
  static bool registered = false;
  if (registered) return;
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (!server) {
    ESP_LOGW("espcontrol", "register_local_action_endpoint: server not ready");
    return;
  }
  server->addHandler(new LocalActionHandler());
  registered = true;
  ESP_LOGI("espcontrol", "Local action endpoint registered");
}

class LocalSensorHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:

  static std::string build_json() {
    std::string json;
    json.reserve(512);
    json = "[";
    bool first = true;
    auto append = [&](const std::string &key, const std::string &name,
                      const std::string &unit, const char *type, bool internal) {
      if (!first) json += ",";
      first = false;
      json += "{\"key\":\"" + local_endpoint_json_escape(key) + "\",\"name\":\"" + local_endpoint_json_escape(name) +
              "\",\"unit\":\"" + local_endpoint_json_escape(unit) + "\",\"type\":\"" + type + "\"";
      if (internal) json += ",\"internal\":true";
      json += "}";
    };
    char oid_buf[128];
#ifdef USE_SENSOR
    for (auto *s : esphome::App.get_sensors()) {
      bool internal = (int) s->get_entity_category() != 0;
      append(std::string(s->get_object_id_to(oid_buf).c_str()), std::string(s->get_name()),
             std::string(s->get_unit_of_measurement_ref()), "numeric", internal);
    }
#endif
#ifdef USE_TEXT_SENSOR
    for (auto *ts : esphome::App.get_text_sensors()) {
      bool internal = (int) ts->get_entity_category() != 0;
      append(std::string(ts->get_object_id_to(oid_buf).c_str()), std::string(ts->get_name()),
             "", "text", internal);
    }
#endif
    json += "]";
    return json;
  }

  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    esphome::StringRef url = request->url_to(url_buf);
    return strncmp(url.c_str(), "/local_sensors", 14) == 0;
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    std::string json = build_json();
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    esp_err_t err = httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
    if (err != ESP_OK) ESP_LOGE("sensors", "httpd_resp_send failed: %d", err);
  }
};

inline void register_local_sensor_endpoint() {
  static bool registered = false;
  if (registered) return;
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (!server) {
    ESP_LOGW("sensors", "register_local_sensor_endpoint: server not ready");
    return;
  }
  server->addHandler(new LocalSensorHandler());
  registered = true;
  ESP_LOGI("sensors", "Local sensor endpoint registered");
}
#endif  // USE_WEBSERVER

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
  return card_runtime_internal_push_mode(p.sensor);
}

inline bool internal_relay_state(const std::string &key) {
  InternalRelayControl *relay = find_internal_relay(key);
  return relay && relay->is_on ? relay->is_on() : false;
}

inline std::string internal_relay_label(const ParsedCfg &p) {
  if (!p.label.empty()) return p.label;
  InternalRelayControl *relay = find_internal_relay(p.entity);
  if (relay && !relay->label.empty()) return relay->label;
  return p.entity.empty() ? espcontrol_i18n(std::string("Relay")) : sentence_cap_text(p.entity);
}

inline const char *internal_relay_icon(const ParsedCfg &p, bool push_mode) {
  if (!p.icon.empty() && p.icon != "Auto") return find_icon(p.icon.c_str());
  return find_icon(push_mode ? "Gesture Tap" : "Power Plug");
}

inline void apply_internal_relay_state(lv_obj_t *btn, lv_obj_t *icon_lbl,
                                       bool on, bool has_icon_on,
                                       const char *icon_off, const char *icon_on) {
  if (btn) {
    set_card_checked_state(btn, on);
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
    set_card_checked_state(w.parent_btn, true);
    if (w.parent_has_alt_icon && w.parent_icon)
      lv_label_set_text(w.parent_icon, w.parent_on_glyph);
  } else {
    set_card_checked_state(w.parent_btn, false);
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
