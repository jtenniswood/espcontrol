#pragma once
#include <array>
#include <cstring>
#include <atomic>
#include <mutex>
#include <memory>
#include "clock_bar_layout.h"
#include "esphome/components/json/json_util.h"
#include <nvs.h>

namespace espcontrol::clockbar {
inline bool parse(const std::string &value, std::vector<Element> &output) {
  if (value.empty()) { output.clear(); return true; }
  if (value.size() > MAX_BYTES) return false;
  std::vector<Element> items;
  bool valid = esphome::json::parse_json(value, [&](JsonObject root) {
    if (!root["version"].is<int>() || root["version"].as<int>() != 1 || !root["items"].is<JsonArray>()) return false;
    JsonArray list = root["items"].as<JsonArray>();
    if (list.size() > MAX_ITEMS) return false;
    for (JsonObject raw : list) {
      Element item;
      const std::array<std::pair<const char *, std::string *>, 7> fields{{
        {"id", &item.id}, {"type", &item.type}, {"group", &item.group}, {"source", &item.source},
        {"entity", &item.entity}, {"label", &item.label}, {"unit", &item.unit}}};
      for (const auto &pair : fields) {
        if (!raw[pair.first].is<const char *>()) return false;
        const JsonString text = raw[pair.first].as<JsonString>();
        pair.second->assign(text.c_str(), text.size());
        if (pair.second->find('\0') != std::string::npos) return false;
      }
      if (item.id.empty() || item.id.size() > 24 || !valid_type(item.type) ||
          (item.group != "left" && item.group != "middle" && item.group != "right") ||
          (item.source != "ha" && item.source != "local") || item.entity.size() > 120 ||
          item.label.size() > 128 || item.unit.size() > 48 || !raw["precision"].is<int>()) return false;
      for (char c : item.id) if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '-') return false;
      item.precision = raw["precision"].as<int>();
      if (item.precision < 0 || item.precision > 3) return false;
      if (!raw["icon"].isUnbound()) {
        if (!raw["icon"].is<const char *>()) return false;
        const JsonString icon = raw["icon"].as<JsonString>();
        item.icon.assign(icon.c_str(), icon.size());
        if (item.icon.size() > 48 || item.icon.find('\0') != std::string::npos) return false;
      }
      for (const auto &other : items) if (other.id == item.id) return false;
      items.push_back(std::move(item));
    }
    return true;
  });
  if (valid) output = std::move(items);
  return valid;
}

// One bounded preference record keeps a layout update together, including an intentionally empty bar.
class Configuration {
 public:
  void load() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (ready_) return;
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;
    auto record = std::make_unique<Record>();
    size_t size = sizeof(Record);
    const auto result = nvs_get_blob(handle, "layout", record.get(), &size);
    nvs_close(handle);
    if (result != ESP_OK && result != ESP_ERR_NVS_NOT_FOUND && result != ESP_ERR_NVS_INVALID_LENGTH && result != ESP_ERR_NVS_TYPE_MISMATCH) return;
    if (result == ESP_OK && size == sizeof(Record) && record->magic == 0xCB4A0001 && record->size <= MAX_BYTES) {
      std::string restored(record->text.data(), record->size);
      std::vector<Element> items;
      if (parse(restored, items)) { value_ = std::move(restored); items_ = std::move(items); }
    }
    ready_ = true;
  }
  bool save(const std::string &value) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!ready_ || value.size() > MAX_BYTES) return false;
    std::vector<Element> items;
    if (!parse(value, items)) return false;
    if (value == value_) return true;
    auto record = std::make_unique<Record>();
    record->magic = 0xCB4A0001;
    record->size = value.size();
    std::memcpy(record->text.data(), value.data(), value.size());
    nvs_handle_t handle;
    if (nvs_open(NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    const bool saved = nvs_set_blob(handle, "layout", record.get(), sizeof(Record)) == ESP_OK && nvs_commit(handle) == ESP_OK;
    nvs_close(handle);
    if (!saved) return false;
    value_ = value;
    items_ = std::move(items);
    changed_.store(true);
    return true;
  }
  bool contains(const std::string &type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return std::any_of(items_.begin(), items_.end(), [&](const Element &item) { return item.type == type; });
  }
  bool ready() const { std::lock_guard<std::mutex> lock(mutex_); return ready_; }
  std::string value() const { std::lock_guard<std::mutex> lock(mutex_); return value_; }
  bool take_changed() { return changed_.exchange(false); }
 private:
  struct Record { uint32_t magic = 0; uint16_t size = 0; std::array<char, MAX_BYTES> text{}; };
  mutable std::mutex mutex_;
  std::atomic<bool> changed_{false};
  static constexpr char NAMESPACE[] = "espcontrol_cb";
  bool ready_ = false;
  std::string value_;
  std::vector<Element> items_;
};
inline Configuration &configuration() { static Configuration value; return value; }
}  // namespace espcontrol::clockbar
