#pragma once
#include <cstring>
#include <functional>
#include <limits>
#include <string>
#include <vector>

#include "esphome/core/string_ref.h"
namespace esphome {
namespace sensor {
struct Sensor {
  explicit Sensor(const char* key) : key(key) {}
  StringRef get_object_id_to(char* buffer) const {
    std::memcpy(buffer, key.c_str(), key.size() + 1);
    return {buffer, key.size()};
  }
  template <typename F>
  void add_on_state_callback(F&& callback) {
    callbacks.emplace_back(std::forward<F>(callback));
  }
  void publish_state(float value) {
    state = value;
    for (auto& callback : callbacks) callback(value);
  }
  std::string key;
  float state = std::numeric_limits<float>::quiet_NaN();
  std::vector<std::function<void(float)>> callbacks;
};
}  // namespace sensor
namespace text_sensor {
struct TextSensor {
  explicit TextSensor(const char* key) : key(key) {}
  StringRef get_object_id_to(char* buffer) const {
    std::memcpy(buffer, key.c_str(), key.size() + 1);
    return {buffer, key.size()};
  }
  template <typename F>
  void add_on_state_callback(F&& callback) {
    callbacks.emplace_back(std::forward<F>(callback));
  }
  void publish_state(const std::string& value) {
    state = value;
    for (auto& callback : callbacks) callback(value);
  }
  std::string key, state;
  std::vector<std::function<void(const std::string&)>> callbacks;
};
}  // namespace text_sensor
struct Application {
  const auto& get_sensors() const {
    return sensors;
  }
  const auto& get_text_sensors() const {
    return texts;
  }
  std::vector<sensor::Sensor*> sensors;
  std::vector<text_sensor::TextSensor*> texts;
};
inline Application App;
}  // namespace esphome
