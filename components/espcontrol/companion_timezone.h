#pragma once

#include <atomic>
#include <mutex>
#include <string>

namespace esphome::companion {

using CompanionTimezoneHomeAssistantConnectedProvider = bool (*)();

struct CompanionTimezoneRuntime {
  std::mutex mutex;
  std::string identifier;
  std::atomic<bool> changed{false};
  CompanionTimezoneHomeAssistantConnectedProvider home_assistant_connected = nullptr;
};

inline CompanionTimezoneRuntime &companion_timezone_runtime() {
  static CompanionTimezoneRuntime runtime;
  return runtime;
}

inline std::string companion_timezone_id() {
  auto &runtime = companion_timezone_runtime();
  std::lock_guard<std::mutex> lock(runtime.mutex);
  return runtime.identifier;
}

inline void companion_set_timezone_id(const std::string &identifier) {
  auto &runtime = companion_timezone_runtime();
  bool changed = false;
  {
    std::lock_guard<std::mutex> lock(runtime.mutex);
    if (runtime.identifier != identifier) {
      runtime.identifier = identifier;
      changed = true;
    }
  }
  if (changed) runtime.changed.store(true, std::memory_order_release);
}

inline bool companion_timezone_changed() {
  return companion_timezone_runtime().changed.load(std::memory_order_acquire);
}

inline bool companion_take_timezone_changed() {
  return companion_timezone_runtime().changed.exchange(false, std::memory_order_acq_rel);
}

inline void register_companion_timezone_home_assistant_connected_provider(
    CompanionTimezoneHomeAssistantConnectedProvider provider) {
  companion_timezone_runtime().home_assistant_connected = provider;
}

inline bool companion_timezone_home_assistant_connected() {
  const auto provider = companion_timezone_runtime().home_assistant_connected;
  return provider && provider();
}

}  // namespace esphome::companion
