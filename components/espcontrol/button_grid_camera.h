#pragma once

// Internal implementation detail for button_grid.h. Include button_grid.h from device YAML.
//
// id() is only resolvable inside YAML-generated lambda bodies (main.cpp scope).
// This header is compiled as part of the ESPHome component system before those
// variables are declared, so it cannot call id() directly.
//
// Instead, camera_popup_yaml registers a capturing lambda at on_boot (where id()
// IS in scope). camera_popup_open_from_cfg calls it via the stored std::function.

inline std::function<void(const std::string &, uint32_t)> &camera_popup_open_fn_storage() {
  static std::function<void(const std::string &, uint32_t)> fn;
  return fn;
}

// Called once from the camera_popup.yaml on_boot lambda to bind id()-based
// globals and the open script into C++-callable storage.
inline void camera_popup_register_fn(std::function<void(const std::string &, uint32_t)> fn) {
  camera_popup_open_fn_storage() = std::move(fn);
}

inline void setup_camera_card(BtnSlot &s, const ParsedCfg &p) {
  lv_label_set_text(s.text_lbl,
    p.label.empty() ? (p.entity.empty() ? "Camera" : p.entity.c_str())
                    : p.label.c_str());
  const char *icon_cp =
    (p.icon.empty() || p.icon == "Auto") ? find_icon("Camera") : find_icon(p.icon.c_str());
  lv_label_set_text(s.icon_lbl, icon_cp);
  apply_push_button_transition(s.btn);
}

// Parse refresh interval (seconds) from the sensor field; default 5, clamp 2-120.
inline uint32_t camera_refresh_ms(const std::string &sensor_field) {
  if (sensor_field.empty()) return 5000u;
  int secs = atoi(sensor_field.c_str());
  if (secs < 2) secs = 2;
  if (secs > 120) secs = 120;
  return static_cast<uint32_t>(secs) * 1000u;
}

inline bool camera_entity_valid(const std::string &entity_id) {
  return entity_id.size() > 7 && entity_id.compare(0, 7, "camera.") == 0;
}

// Open the camera popup for the given config.
// Safe to call from handle_button_click and subpage event callbacks.
inline void camera_popup_open_from_cfg(const ParsedCfg &p) {
  if (!camera_entity_valid(p.entity)) {
    ESP_LOGW("camera_popup", "Expected camera.* entity, got: %s", p.entity.c_str());
    return;
  }
  auto &fn = camera_popup_open_fn_storage();
  if (!fn) {
    ESP_LOGW("camera_popup", "Camera popup not registered; include camera_popup.yaml");
    return;
  }
  fn(p.entity, camera_refresh_ms(p.sensor));
}
