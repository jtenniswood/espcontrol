#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
namespace esphome {
inline uint32_t fnv1_hash(const char *value) {
  assert(value && !*value);
  return 2166136261u;
}
struct Preferences {
  uint32_t nvs_handle{7};
  std::map<uint32_t, std::vector<uint8_t>> durable, pending;
  size_t syncs{0}, saves{0}, peak_queued_bytes{0}, fail_sync{0};
  bool sync() {
    ++syncs;
    if (syncs == fail_sync) {
      pending.clear();
      return false;
    }
    for (const auto &item : pending)
      durable[item.first] = item.second;
    pending.clear();
    return true;
  }
};
inline Preferences *global_preferences = nullptr;
struct ESPPreferenceBackend {
  uint32_t key{0}, nvs_handle{0};
  bool save(const uint8_t *data, size_t size) {
    assert(global_preferences && nvs_handle == global_preferences->nvs_handle);
    ++global_preferences->saves;
    global_preferences->pending[key] = {data, data + size};
    size_t queued = 0;
    for (const auto &item : global_preferences->pending)
      queued += item.second.size();
    global_preferences->peak_queued_bytes =
        std::max(global_preferences->peak_queued_bytes, queued);
    return true;
  }
  bool load(uint8_t *data, size_t size) {
    assert(global_preferences && nvs_handle == global_preferences->nvs_handle);
    const auto item = global_preferences->durable.find(key);
    if (item == global_preferences->durable.end() ||
        item->second.size() != size)
      return false;
    std::memcpy(data, item->second.data(), size);
    return true;
  }
};
} // namespace esphome
