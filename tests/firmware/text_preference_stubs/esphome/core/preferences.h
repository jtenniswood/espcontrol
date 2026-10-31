#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>
namespace esphome {
inline uint32_t fnv1_hash(const char *value) {
  assert(value && !*value);
  return 2166136261u;
}
inline size_t uint32_to_str(char (&buffer)[11], uint32_t value) {
  return static_cast<size_t>(
      std::snprintf(buffer, sizeof(buffer), "%u", value));
}
struct Preferences {
  uint32_t nvs_handle{7};
  std::map<uint32_t, std::vector<uint8_t>> durable, pending;
  size_t syncs{0}, saves{0}, peak_queued_bytes{0}, fail_sync{0};
  size_t capacity_entries{SIZE_MAX}, erases{0}, commits{0};
  bool fail_erase{false}, fail_commit{false};
  std::vector<std::string> operations;
  static size_t entries(size_t bytes) { return 2 + (bytes + 31) / 32; }
  size_t used_entries() const {
    size_t count = 0;
    for (const auto &item : durable)
      count += entries(item.second.size());
    return count;
  }
  bool sync() {
    ++syncs;
    if (syncs == fail_sync) {
      pending.clear();
      return false;
    }
    bool success = true;
    for (const auto &item : pending) {
      const auto old = durable.find(item.first);
      if (old != durable.end() && old->second == item.second)
        continue;
      // NVS needs space for a replacement before retiring the previous blob.
      const auto used = used_entries();
      if (used > capacity_entries ||
          entries(item.second.size()) > capacity_entries - used) {
        success = false;
        continue;
      }
      durable[item.first] = item.second;
      operations.push_back("write:" + std::to_string(item.first));
    }
    pending.clear();
    return success;
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
