#pragma once

// Companion-owned runtime state. UI/card code consumes immutable snapshots and
// asks this service to perform state transitions; it never owns connection or
// catalogue state itself.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

struct CompanionAction {
  std::string id;
  std::string label;
};

struct CompanionValue {
  std::string id;
  int value{0};
};

enum class CompanionPlaybackState : uint8_t {
  UNAVAILABLE = 0,
  STOPPED,
  PAUSED,
  PLAYING,
};

inline const char *companion_play_pause_status(CompanionPlaybackState state,
                                                bool available = true) {
  if (!available) return "Unavailable";
  if (state == CompanionPlaybackState::PLAYING) return "Playing";
  if (state == CompanionPlaybackState::PAUSED) return "Paused";
  return "Stopped";
}

struct CompanionNowPlayingSnapshot {
  uint32_t generation{0};
  CompanionPlaybackState playback_state{CompanionPlaybackState::UNAVAILABLE};
  std::string source_application_id;
  std::string source_application_name;
  std::string content_id;
  std::string title;
  std::string artist;
  std::string album;
  float duration{0.0f};
  float position{0.0f};
  float playback_rate{0.0f};
  bool artwork_follows{false};
};

struct CompanionSystemMetricsSnapshot {
  uint32_t generation{0};
  float cpu_usage_percent{NAN};
  float memory_usage_percent{NAN};
  float storage_usage_percent{NAN};
  float battery_percent{NAN};
  float network_throughput_kbps{NAN};
};

struct CompanionRuntimeSnapshot {
  std::vector<CompanionAction> actions;
  std::vector<CompanionValue> values;
  std::string focused_action_id;
  bool media_actions_supported{false};
  std::vector<std::string> window_actions;
  bool connected{false};
  CompanionNowPlayingSnapshot now_playing;
  CompanionSystemMetricsSnapshot system_metrics;
};

class CompanionRuntimeService {
 public:
  CompanionRuntimeSnapshot snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {actions_, values_, focused_action_id_, media_actions_supported_, window_actions_,
            connected_, now_playing_, system_metrics_};
  }

  void set_actions(std::vector<CompanionAction> actions) {
    std::lock_guard<std::mutex> lock(mutex_);
    actions_ = std::move(actions);
    request_refresh_();
  }

  void set_media_actions_supported(bool supported) {
    std::lock_guard<std::mutex> lock(mutex_);
    media_actions_supported_ = supported;
    request_refresh_();
  }

  void set_window_actions(std::vector<std::string> actions) {
    std::lock_guard<std::mutex> lock(mutex_);
    window_actions_ = std::move(actions);
    request_refresh_();
  }

  void set_value(const std::string &control_id, int value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto item = std::find_if(values_.begin(), values_.end(),
      [&control_id](const CompanionValue &candidate) { return candidate.id == control_id; });
    if (item == values_.end()) values_.push_back({control_id, value});
    else item->value = value;
    request_refresh_();
  }

  void remove_value(const std::string &control_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    values_.erase(std::remove_if(values_.begin(), values_.end(),
      [&control_id](const CompanionValue &candidate) { return candidate.id == control_id; }),
      values_.end());
    request_refresh_();
  }

  void set_now_playing(CompanionNowPlayingSnapshot snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    now_playing_ = std::move(snapshot);
    request_refresh_();
  }

  void set_system_metrics(CompanionSystemMetricsSnapshot snapshot) {
    std::lock_guard<std::mutex> lock(mutex_);
    system_metrics_ = std::move(snapshot);
    request_refresh_();
  }

  bool set_focused_action(std::string action_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    const bool should_return = connected_ && !focused_action_id_.empty() &&
      action_id != focused_action_id_;
    if (action_id.empty() || !connected_) {
      pending_auto_subpage_action_id_.clear();
    } else if (focused_action_id_ != action_id) {
      pending_auto_subpage_action_id_ = action_id;
    }
    focused_action_id_ = std::move(action_id);
    request_refresh_();
    return should_return;
  }

  std::string pending_auto_subpage_action() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return pending_auto_subpage_action_id_;
  }

  bool consume_auto_subpage_action(const std::string &action_id) {
    if (action_id.empty()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (pending_auto_subpage_action_id_ != action_id) return false;
    pending_auto_subpage_action_id_.clear();
    return true;
  }

  void set_connected(bool connected) {
    std::lock_guard<std::mutex> lock(mutex_);
    connected_ = connected;
    if (!connected_) {
      values_.clear();
      focused_action_id_.clear();
      pending_auto_subpage_action_id_.clear();
      media_actions_supported_ = false;
      window_actions_.clear();
      now_playing_ = {};
      system_metrics_ = {};
    }
    request_refresh_();
  }

  void request_refresh() { request_refresh_(); }
  bool consume_refresh_request() { return refresh_requested_.exchange(false); }
  std::atomic<bool> &refresh_flag() { return refresh_requested_; }

 private:
  void request_refresh_() { refresh_requested_.store(true); }

  mutable std::mutex mutex_;
  std::vector<CompanionAction> actions_;
  std::vector<CompanionValue> values_;
  std::string focused_action_id_;
  std::string pending_auto_subpage_action_id_;
  bool media_actions_supported_{false};
  std::vector<std::string> window_actions_;
  bool connected_{false};
  CompanionNowPlayingSnapshot now_playing_;
  CompanionSystemMetricsSnapshot system_metrics_;
  std::atomic<bool> refresh_requested_{false};
};

inline CompanionRuntimeService &companion_runtime_service() {
  static CompanionRuntimeService service;
  return service;
}
