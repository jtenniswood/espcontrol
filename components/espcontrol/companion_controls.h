#pragma once

// Runtime boundary for Companion cards. The grid knows only opaque action IDs;
// the companion service owns pairing, transport and the Mac app catalogue.

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "companion_capabilities_generated.h"
#include "companion_timezone.h"

#ifdef USE_WEBSERVER
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_config_http_context.h"
#endif

#ifdef USE_LVGL
#include "esphome/components/lvgl/lvgl_esphome.h"
#include "display_text.h"
#include "i18n_generated.h"
#endif

struct CompanionAction {
  std::string id;
  std::string label;
};

struct CompanionValue {
  std::string id;
  int value{0};
};

using CompanionActionSender = std::function<bool(const std::string &, const std::string &)>;
using CompanionUrlSender = std::function<bool(const std::string &, const std::string &, const std::string &)>;
using CompanionValueSender = std::function<bool(const std::string &, int, const std::string &)>;
using CompanionActionResultHandler = std::function<void()>;
using CompanionConnectionChangedHandler = std::function<void(bool)>;

struct CompanionPendingActionResult {
  std::mutex mutex;
  std::string request_id;
  std::string expected_application_id;
  CompanionActionResultHandler success;
};

inline CompanionPendingActionResult &companion_pending_action_result() {
  static CompanionPendingActionResult pending;
  return pending;
}

inline void companion_expect_action_result(const std::string &request_id,
                                           CompanionActionResultHandler success) {
  auto &pending = companion_pending_action_result();
  std::lock_guard<std::mutex> lock(pending.mutex);
  pending.request_id = request_id;
  pending.expected_application_id.clear();
  pending.success = std::move(success);
}

inline void companion_expect_action_result(
    const std::string &request_id, const std::string &expected_application_id,
    CompanionActionResultHandler success) {
  auto &pending = companion_pending_action_result();
  std::lock_guard<std::mutex> lock(pending.mutex);
  pending.request_id = request_id;
  pending.expected_application_id = expected_application_id;
  pending.success = std::move(success);
}

inline void companion_cancel_action_result(const std::string &request_id = "") {
  auto &pending = companion_pending_action_result();
  std::lock_guard<std::mutex> lock(pending.mutex);
  if (!request_id.empty() && pending.request_id != request_id) return;
  pending.request_id.clear();
  pending.expected_application_id.clear();
  pending.success = nullptr;
}

inline void companion_deliver_action_result(const std::string &request_id,
                                            const std::string &status) {
  CompanionActionResultHandler success;
  {
    auto &pending = companion_pending_action_result();
    std::lock_guard<std::mutex> lock(pending.mutex);
    if (pending.request_id != request_id) return;
    // Only a current Companion build sends "activated" after it has verified
    // that a launched application is frontmost. Older builds report
    // "performed" as soon as launch scheduling succeeds, which is not safe
    // enough to expose keyboard shortcuts.
    if (status == "activated") success = std::move(pending.success);
    pending.request_id.clear();
    pending.expected_application_id.clear();
    pending.success = nullptr;
  }
  if (success) success();
}

struct CompanionPairingSnapshot {
  bool available{false};
  bool active{false};
  bool paired{false};
  bool connected{false};
  uint32_t expires_in_seconds{0};
  uint16_t port{8443};
  uint32_t system_metrics_generation{0};
  std::string pairing_code;
  std::string mdns_name;
};

using CompanionPairingProvider = std::function<CompanionPairingSnapshot()>;
using CompanionPairingStarter = std::function<CompanionPairingSnapshot()>;

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
  // A command-capable Companion can still receive Play/Pause when there is
  // no active Now Playing session. Keep the card enabled and show the same
  // idle state used by a stopped player instead of treating the command as
  // unavailable.
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

using CompanionNowPlayingHandler = std::function<void(const CompanionNowPlayingSnapshot &)>;
// Ownership of data transfers to the handler only when it returns true.
using CompanionArtworkHandler = std::function<bool(uint32_t generation, uint8_t *data, size_t size)>;

inline void companion_request_card_refresh();

inline std::atomic<bool> &companion_card_refresh_requested() {
  static std::atomic<bool> requested{false};
  return requested;
}

inline void companion_request_card_refresh() {
  companion_card_refresh_requested().store(true);
}

struct CompanionRuntimeState {
  std::mutex mutex;
  std::vector<CompanionAction> actions;
  std::vector<CompanionValue> values;
  std::string focused_action_id;
  std::string pending_auto_subpage_action_id;
  bool media_actions_supported{false};
  bool connected{false};
  CompanionNowPlayingSnapshot now_playing;
  CompanionSystemMetricsSnapshot system_metrics;
};

struct CompanionRuntimeSnapshot {
  std::vector<CompanionAction> actions;
  std::vector<CompanionValue> values;
  std::string focused_action_id;
  bool media_actions_supported{false};
  bool connected{false};
  CompanionNowPlayingSnapshot now_playing;
  CompanionSystemMetricsSnapshot system_metrics;
};

inline CompanionRuntimeState &companion_runtime_state() {
  static CompanionRuntimeState state;
  return state;
}

inline CompanionRuntimeSnapshot companion_runtime_snapshot() {
  auto &state = companion_runtime_state();
  std::lock_guard<std::mutex> lock(state.mutex);
  return {state.actions, state.values, state.focused_action_id, state.media_actions_supported,
          state.connected, state.now_playing, state.system_metrics};
}

inline CompanionNowPlayingHandler &companion_now_playing_handler() {
  static CompanionNowPlayingHandler handler;
  return handler;
}

inline CompanionConnectionChangedHandler &companion_connection_changed_handler() {
  static CompanionConnectionChangedHandler handler;
  return handler;
}

inline void register_companion_connection_changed_handler(
    CompanionConnectionChangedHandler handler) {
  companion_connection_changed_handler() = std::move(handler);
}

inline CompanionArtworkHandler &companion_artwork_handler() {
  static CompanionArtworkHandler handler;
  return handler;
}

inline void register_companion_now_playing_handlers(CompanionNowPlayingHandler now_playing,
                                                     CompanionArtworkHandler artwork) {
  companion_now_playing_handler() = std::move(now_playing);
  companion_artwork_handler() = std::move(artwork);
}

inline void companion_set_now_playing(CompanionNowPlayingSnapshot snapshot) {
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.now_playing = snapshot;
  }
  if (companion_now_playing_handler()) companion_now_playing_handler()(snapshot);
  companion_request_card_refresh();
}

inline bool companion_metric_key_valid(const std::string &key) {
  return companion_metric_capability(key) != nullptr;
}

inline const char *companion_metric_label_key(const std::string &key) {
  const auto *capability = companion_metric_capability(key);
  return capability ? capability->label_key : "";
}

inline const char *companion_metric_default_unit(const std::string &key) {
  const auto *capability = companion_metric_capability(key);
  return capability ? capability->unit : "";
}

inline bool companion_metric_value(const CompanionRuntimeSnapshot &snapshot,
                                   const std::string &key, float &value) {
  if (!snapshot.connected) return false;
  if (key == "stat.cpu") value = snapshot.system_metrics.cpu_usage_percent;
  else if (key == "stat.memory") value = snapshot.system_metrics.memory_usage_percent;
  else if (key == "stat.memory_free") value = 100.0f - snapshot.system_metrics.memory_usage_percent;
  else if (key == "stat.storage") value = snapshot.system_metrics.storage_usage_percent;
  else if (key == "stat.storage_free") value = 100.0f - snapshot.system_metrics.storage_usage_percent;
  else if (key == "stat.battery") value = snapshot.system_metrics.battery_percent;
  else if (key == "stat.network_throughput") {
    // The Companion protocol remains in KB/s; cards display megabytes per second.
    value = snapshot.system_metrics.network_throughput_kbps / 1024.0f;
  }
  else return false;
  return std::isfinite(value);
}

inline void companion_set_system_metrics(CompanionSystemMetricsSnapshot snapshot) {
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.system_metrics = snapshot;
  }
  companion_request_card_refresh();
}

inline bool companion_deliver_artwork(uint32_t generation, uint8_t *data, size_t size) {
  return companion_artwork_handler() && companion_artwork_handler()(generation, data, size);
}

inline bool companion_connected() {
  return companion_runtime_snapshot().connected;
}

inline CompanionActionSender &companion_action_sender() {
  static CompanionActionSender sender;
  return sender;
}

inline CompanionUrlSender &companion_url_sender() {
  static CompanionUrlSender sender;
  return sender;
}

inline CompanionValueSender &companion_value_sender() {
  static CompanionValueSender sender;
  return sender;
}

inline CompanionPairingProvider &companion_pairing_provider() {
  static CompanionPairingProvider provider;
  return provider;
}

inline CompanionPairingStarter &companion_pairing_starter() {
  static CompanionPairingStarter starter;
  return starter;
}

inline void companion_set_actions(std::vector<CompanionAction> actions) {
  actions.erase(std::remove_if(actions.begin(), actions.end(),
    [](const CompanionAction &action) {
      return action.id.empty() || action.id.size() > 96 || action.label.size() > 96;
    }), actions.end());
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.actions = std::move(actions);
  }
  companion_request_card_refresh();
}

inline bool companion_media_action_valid(const std::string &action_id) {
  return companion_generated_media_action_valid(action_id);
}

inline void companion_set_media_actions_supported(bool supported) {
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.media_actions_supported = supported;
  }
  companion_request_card_refresh();
}

inline bool companion_volume_control_valid(const std::string &control_id) {
  return control_id == "media.output_volume" || control_id == "media.input_volume";
}

inline const char *companion_volume_control_label(const std::string &control_id) {
  if (control_id == "media.output_volume") return "Output Volume";
  if (control_id == "media.input_volume") return "Input Volume";
  return "Volume";
}

inline bool companion_value(const std::string &control_id, int &value) {
  const auto snapshot = companion_runtime_snapshot();
  const auto item = std::find_if(snapshot.values.begin(), snapshot.values.end(),
    [&control_id](const CompanionValue &candidate) { return candidate.id == control_id; });
  if (item == snapshot.values.end()) return false;
  value = item->value;
  return true;
}

inline void companion_set_value(const std::string &control_id, int value) {
  if (!companion_volume_control_valid(control_id)) return;
  value = std::max(0, std::min(100, value));
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    auto item = std::find_if(state.values.begin(), state.values.end(),
      [&control_id](const CompanionValue &candidate) { return candidate.id == control_id; });
    if (item == state.values.end()) state.values.push_back({control_id, value});
    else item->value = value;
  }
  companion_request_card_refresh();
}

inline void companion_remove_value(const std::string &control_id) {
  if (!companion_volume_control_valid(control_id)) return;
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.values.erase(std::remove_if(state.values.begin(), state.values.end(),
      [&control_id](const CompanionValue &candidate) { return candidate.id == control_id; }),
      state.values.end());
  }
  companion_request_card_refresh();
}

inline std::atomic<bool> &companion_subpage_return_requested();

inline void companion_set_focused_action(std::string action_id) {
  if (action_id.size() > 96) action_id.clear();
  bool should_return_from_subpage = false;
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    should_return_from_subpage = state.connected && !state.focused_action_id.empty() &&
      action_id != state.focused_action_id;
    if (action_id.empty() || !state.connected) {
      state.pending_auto_subpage_action_id.clear();
    } else if (state.focused_action_id != action_id) {
      state.pending_auto_subpage_action_id = action_id;
    }
    state.focused_action_id = std::move(action_id);
  }
  if (should_return_from_subpage) companion_subpage_return_requested().store(true);
  companion_request_card_refresh();
}

inline std::atomic<bool> &companion_subpage_return_requested() {
  static std::atomic<bool> requested{false};
  return requested;
}

inline bool companion_consume_subpage_return_request() {
  return companion_subpage_return_requested().exchange(false);
}

inline std::string companion_pending_auto_subpage_action() {
  auto &state = companion_runtime_state();
  std::lock_guard<std::mutex> lock(state.mutex);
  return state.pending_auto_subpage_action_id;
}

inline bool companion_consume_auto_subpage_action(const std::string &action_id) {
  if (action_id.empty()) return false;
  auto &state = companion_runtime_state();
  std::lock_guard<std::mutex> lock(state.mutex);
  if (state.pending_auto_subpage_action_id != action_id) return false;
  state.pending_auto_subpage_action_id.clear();
  return true;
}

inline bool companion_action_focused(const std::string &action_id) {
  if (action_id.empty() ||
      action_id.rfind("shortcut.", 0) == 0 || action_id.rfind("media.", 0) == 0) return false;
  const auto snapshot = companion_runtime_snapshot();
  return snapshot.connected && snapshot.focused_action_id == action_id;
}

inline void companion_set_focused_application(std::string application_id) {
  if (application_id.size() > 96) application_id.clear();
  const std::string focused_application_id = application_id;
  companion_set_focused_action(std::move(application_id));
  CompanionActionResultHandler success;
  {
    auto &pending = companion_pending_action_result();
    std::lock_guard<std::mutex> lock(pending.mutex);
    if (!pending.expected_application_id.empty() &&
        pending.expected_application_id == focused_application_id) {
      success = std::move(pending.success);
      pending.request_id.clear();
      pending.expected_application_id.clear();
      pending.success = nullptr;
    }
  }
  if (success) success();
  companion_request_card_refresh();
}

inline bool companion_application_focused(const std::string &application_id) {
  return companion_action_focused(application_id);
}

inline bool companion_action_active(const std::string &action_id) {
  const auto snapshot = companion_runtime_snapshot();
  if (action_id == "media.play_pause") {
    return snapshot.connected && snapshot.media_actions_supported &&
           snapshot.now_playing.playback_state == CompanionPlaybackState::PLAYING;
  }
  return companion_action_focused(action_id);
}

inline uint32_t companion_next_request_number() {
  static std::atomic<uint32_t> request_number{0};
  return ++request_number;
}

inline void companion_set_connected(bool connected) {
  auto &state = companion_runtime_state();
  {
    std::lock_guard<std::mutex> lock(state.mutex);
    state.connected = connected;
    if (!connected) {
      state.values.clear();
      state.focused_action_id.clear();
      state.pending_auto_subpage_action_id.clear();
      state.media_actions_supported = false;
      state.now_playing = {};
      state.system_metrics = {};
    }
  }
  if (!connected) {
    companion_cancel_action_result();
    companion_subpage_return_requested().store(true);
  }
  companion_request_card_refresh();
  auto &connection_changed = companion_connection_changed_handler();
  if (connection_changed) connection_changed(connected);
}

inline void register_companion_action_sender(CompanionActionSender sender) {
  companion_action_sender() = std::move(sender);
}

inline void register_companion_url_sender(CompanionUrlSender sender) {
  companion_url_sender() = std::move(sender);
}

inline void register_companion_value_sender(CompanionValueSender sender) {
  companion_value_sender() = std::move(sender);
}

inline void register_companion_pairing_callbacks(CompanionPairingProvider provider,
                                                  CompanionPairingStarter starter) {
  companion_pairing_provider() = std::move(provider);
  companion_pairing_starter() = std::move(starter);
}

inline std::vector<std::string> companion_shortcut_parts(const std::string &action_id) {
  static const std::string prefix = "shortcut.";
  if (action_id.rfind(prefix, 0) != 0 || action_id.size() <= prefix.size() || action_id.size() > 96) return {};
  std::vector<std::string> parts;
  size_t start = prefix.size();
  while (start <= action_id.size()) {
    const size_t end = action_id.find('+', start);
    parts.push_back(action_id.substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return parts;
}

inline bool companion_shortcut_action_valid(const std::string &action_id) {
  const auto parts = companion_shortcut_parts(action_id);
  if (parts.size() < 2 || parts.size() > 5) return false;
  bool command = false, control = false, option = false, shift = false;
  for (size_t i = 0; i + 1 < parts.size(); i++) {
    const auto &part = parts[i];
    if (part == "command" && !command) command = true;
    else if (part == "control" && !control) control = true;
    else if (part == "option" && !option) option = true;
    else if (part == "shift" && !shift) shift = true;
    else return false;
  }
  if (!command && !control && !option) return false;
  const auto &key = parts.back();
  if (key.size() == 1 && ((key[0] >= 'a' && key[0] <= 'z') || (key[0] >= '0' && key[0] <= '9'))) return true;
  static const std::vector<std::string> named_keys{
    "space", "enter", "tab", "escape", "delete", "forwarddelete",
    "left", "right", "up", "down", "home", "end", "pageup", "pagedown",
    "keycomma", "keyperiod", "keyslash", "keysemicolon", "keyquote", "keybackslash", "keyminus",
    "keyequal", "keybracketleft", "keybracketright", "keybackquote",
  };
  if (std::find(named_keys.begin(), named_keys.end(), key) != named_keys.end()) return true;
  if (key.size() >= 2 && key[0] == 'f') {
    const int number = std::atoi(key.c_str() + 1);
    return number >= 1 && number <= 20 && key == "f" + std::to_string(number);
  }
  return false;
}

inline std::string companion_shortcut_label(const std::string &action_id) {
  if (!companion_shortcut_action_valid(action_id)) return "";
  const auto parts = companion_shortcut_parts(action_id);
  std::string label;
  for (size_t i = 0; i + 1 < parts.size(); i++) {
    if (parts[i] == "command") label += "\U000F0633";
    else if (parts[i] == "control") label += "\U000F0634";
    else if (parts[i] == "option") label += "\U000F0635";
    else if (parts[i] == "shift") label += "\U000F0636";
  }
  std::string key = parts.back();
  if (key.size() == 1 && key[0] >= 'a' && key[0] <= 'z') key[0] = static_cast<char>(key[0] - 'a' + 'A');
  else if (key == "enter") key = "Return";
  else if (key == "escape") key = "Esc";
  else if (key == "forwarddelete") key = "Forward Delete";
  else if (key == "left") key = "\U000F004D";
  else if (key == "right") key = "\U000F0054";
  else if (key == "up") key = "\U000F005D";
  else if (key == "down") key = "\U000F0045";
  else if (key == "pageup") key = "Page Up";
  else if (key == "pagedown") key = "Page Down";
  else if (key == "keycomma") key = ",";
  else if (key == "keyperiod") key = ".";
  else if (key == "keyslash") key = "/";
  else if (key == "keysemicolon") key = ";";
  else if (key == "keyquote") key = "'";
  else if (key == "keybackslash") key = "\\";
  else if (key == "keyminus") key = "-";
  else if (key == "keyequal") key = "=";
  else if (key == "keybracketleft") key = "[";
  else if (key == "keybracketright") key = "]";
  else if (key == "keybackquote") key = "`";
  else if (!key.empty()) key[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[0])));
  return label + key;
}

inline bool companion_window_action_valid(const std::string &action_id) {
  return companion_window_capability(action_id) != nullptr;
}

inline std::string companion_window_action_label(const std::string &action_id) {
  const auto *capability = companion_window_capability(action_id);
  return capability ? capability->label : "";
}

inline bool companion_action_available(const std::string &action_id) {
  if (action_id.empty()) return false;
  const auto snapshot = companion_runtime_snapshot();
  if (!snapshot.connected) return false;
  if (companion_shortcut_action_valid(action_id)) return true;
  if (companion_window_action_valid(action_id)) return true;
  if (companion_media_action_valid(action_id)) return snapshot.media_actions_supported;
  return std::any_of(snapshot.actions.begin(), snapshot.actions.end(), [&action_id](const CompanionAction &action) {
    return action.id == action_id;
  });
}

inline std::string companion_encoded_url(const std::string &url_config) {
  static const std::string prefix = "url.";
  if (url_config.rfind(prefix, 0) != 0 || url_config.size() <= prefix.size()) return "";
  const std::string encoded = url_config.substr(prefix.size());
  if (encoded.size() > 128 ||
      (encoded.rfind("http%3A%2F%2F", 0) != 0 && encoded.rfind("https%3A%2F%2F", 0) != 0)) return "";
  if (!std::all_of(encoded.begin(), encoded.end(), [](unsigned char byte) {
        return byte >= 0x21 && byte <= 0x7e && byte != '|' && byte != ',';
      })) return "";
  return encoded;
}

inline std::string companion_default_action_label(const std::string &action_id,
                                                  const std::string &url_config = "") {
  const std::string shortcut_label = companion_shortcut_label(action_id);
  if (!shortcut_label.empty()) return shortcut_label;
  const std::string window_label = companion_window_action_label(action_id);
  if (!window_label.empty()) return window_label;
  if (!companion_encoded_url(url_config).empty()) return "Open URL";
  return action_id.empty() ? "Mac App" : action_id;
}

inline bool companion_metric_card_should_disable(bool connected, bool preserve_navigation) {
  return !connected && !preserve_navigation;
}

inline bool companion_url_available(const std::string &app_id, const std::string &url_config) {
  return !companion_encoded_url(url_config).empty() && companion_action_available(app_id);
}

inline bool companion_card_focus_allowed(const std::string &url_config) {
  return url_config.empty();
}

#ifdef USE_LVGL
struct CompanionCardRef {
  lv_obj_t *button = nullptr;
  lv_obj_t *text_label = nullptr;
  std::string action_id;
  std::string url_config;
  lv_obj_t *value_label = nullptr;
  lv_obj_t *unit_label = nullptr;
  std::string metric_key;
  std::string metric_unit;
  int precision{0};
  bool preserve_navigation{false};
};

struct CompanionSliderRef {
  lv_obj_t *slider = nullptr;
  lv_obj_t *icon_label = nullptr;
  std::string control_id;
  bool has_icon_on = false;
  const char *icon_off = nullptr;
  const char *icon_on = nullptr;
};

inline void companion_apply_card_focus(lv_obj_t *button, const std::string &action_id,
                                       const std::string &url_config = "") {
  if (!button) return;
  // A URL card targets a page inside the app, but Companion only reports the
  // frontmost app. Keep URL cards from appearing active for the wrong page.
  if (!companion_card_focus_allowed(url_config)) {
    lv_obj_clear_state(button, LV_STATE_CHECKED);
    return;
  }
  if (companion_action_active(action_id)) lv_obj_add_state(button, LV_STATE_CHECKED);
  else lv_obj_clear_state(button, LV_STATE_CHECKED);
}

inline std::vector<CompanionCardRef> &companion_card_refs() {
  static std::vector<CompanionCardRef> refs;
  return refs;
}

inline std::vector<CompanionSliderRef> &companion_slider_refs() {
  static std::vector<CompanionSliderRef> refs;
  return refs;
}

inline void companion_forget_card(lv_obj_t *button) {
  auto &refs = companion_card_refs();
  refs.erase(std::remove_if(refs.begin(), refs.end(), [button](const CompanionCardRef &ref) {
    return ref.button == button;
  }), refs.end());
}

inline void companion_card_deleted(lv_event_t *event) {
  companion_forget_card(static_cast<lv_obj_t *>(lv_event_get_target(event)));
}

inline void companion_slider_deleted(lv_event_t *event) {
  lv_obj_t *slider = static_cast<lv_obj_t *>(lv_event_get_target(event));
  auto &refs = companion_slider_refs();
  refs.erase(std::remove_if(refs.begin(), refs.end(), [slider](const CompanionSliderRef &ref) {
    return ref.slider == slider;
  }), refs.end());
}

inline void companion_track_slider(lv_obj_t *slider, const std::string &control_id,
                                   lv_obj_t *icon_label, bool has_icon_on,
                                   const char *icon_off, const char *icon_on) {
  if (!slider || !companion_volume_control_valid(control_id)) return;
  auto &refs = companion_slider_refs();
  auto existing = std::find_if(refs.begin(), refs.end(), [slider](const CompanionSliderRef &ref) {
    return ref.slider == slider;
  });
  if (existing != refs.end()) {
    existing->control_id = control_id;
    existing->icon_label = icon_label;
    existing->has_icon_on = has_icon_on;
    existing->icon_off = icon_off;
    existing->icon_on = icon_on;
  } else {
    refs.push_back({slider, icon_label, control_id, has_icon_on, icon_off, icon_on});
    lv_obj_add_event_cb(slider, companion_slider_deleted, LV_EVENT_DELETE, nullptr);
  }
  companion_request_card_refresh();
}

inline void companion_track_card(lv_obj_t *button, const std::string &action_id,
                                 const std::string &url_config = "",
                                 lv_obj_t *text_label = nullptr) {
  if (!button) return;
  auto &refs = companion_card_refs();
  auto existing = std::find_if(refs.begin(), refs.end(), [button](const CompanionCardRef &ref) {
    return ref.button == button;
  });
  if (existing != refs.end()) {
    if (companion_metric_key_valid(action_id) && !existing->metric_key.empty()) return;
    existing->action_id = action_id;
    existing->url_config = url_config;
    // The periodic config tracker does not have the label pointer. Preserve
    // the pointer registered while the card was rendered so state updates can
    // continue replacing the Play/Pause label.
    if (text_label) existing->text_label = text_label;
    existing->metric_key.clear();
    existing->metric_unit.clear();
    existing->value_label = nullptr;
    existing->unit_label = nullptr;
    return;
  }
  refs.push_back({button, text_label, action_id, url_config, nullptr, nullptr, "", "", 0, false});
  lv_obj_add_event_cb(button, companion_card_deleted, LV_EVENT_DELETE, nullptr);
}

inline void companion_track_metric_card(lv_obj_t *button, lv_obj_t *value_label,
                                        lv_obj_t *unit_label, const std::string &metric_key,
                                        const std::string &unit, int precision,
                                        bool preserve_navigation = false) {
  if (!button || !companion_metric_key_valid(metric_key)) return;
  auto &refs = companion_card_refs();
  auto existing = std::find_if(refs.begin(), refs.end(), [button](const CompanionCardRef &ref) {
    return ref.button == button;
  });
  CompanionCardRef value{button, nullptr, "", "", value_label, unit_label, metric_key, unit,
                         std::max(0, std::min(2, precision)), preserve_navigation};
  if (existing != refs.end()) {
    *existing = std::move(value);
  } else {
    refs.push_back(std::move(value));
    lv_obj_add_event_cb(button, companion_card_deleted, LV_EVENT_DELETE, nullptr);
  }
  companion_request_card_refresh();
}

inline void companion_refresh_cards_if_requested() {
  if (!companion_card_refresh_requested().exchange(false)) return;
  auto &refs = companion_card_refs();
  const auto snapshot = companion_runtime_snapshot();
  for (auto it = refs.begin(); it != refs.end();) {
    if (!it->button || !lv_obj_is_valid(it->button)) {
      it = refs.erase(it);
      continue;
    }
    if (!it->metric_key.empty()) {
      float value = NAN;
      const bool available = companion_metric_value(snapshot, it->metric_key, value);
      if (it->value_label) {
        char buffer[32];
        if (!available) snprintf(buffer, sizeof(buffer), "--");
        else if (it->precision == 2) snprintf(buffer, sizeof(buffer), "%.2f", value);
        else if (it->precision == 1) snprintf(buffer, sizeof(buffer), "%.1f", value);
        else snprintf(buffer, sizeof(buffer), "%.0f", value);
        lv_label_set_display_text(it->value_label, buffer);
      }
      if (it->unit_label) {
        lv_label_set_display_text(it->unit_label, available ? it->metric_unit.c_str() : "");
      }
      // Match the unavailable state used by other cards when their backing
      // connection is offline. A missing optional metric (for example,
      // battery on a desktop Mac) remains enabled while Companion is online.
      if (companion_metric_card_should_disable(snapshot.connected, it->preserve_navigation)) {
        lv_obj_add_state(it->button, LV_STATE_DISABLED);
      } else {
        lv_obj_clear_state(it->button, LV_STATE_DISABLED);
      }
      ++it;
      continue;
    }
    const bool available = it->url_config.empty()
      ? companion_action_available(it->action_id)
      : companion_url_available(it->action_id, it->url_config);
    if (it->action_id == "media.play_pause" && it->text_label &&
        lv_obj_is_valid(it->text_label)) {
      const auto snapshot = companion_runtime_snapshot();
      const char *status = companion_play_pause_status(
        snapshot.now_playing.playback_state, available);
      const std::string translated_status = espcontrol_i18n(std::string(status));
      lv_label_set_display_text(it->text_label, translated_status.c_str());
    }
    if (available) {
      lv_obj_clear_state(it->button, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(it->button, LV_STATE_DISABLED);
    }
    companion_apply_card_focus(it->button, it->action_id, it->url_config);
    ++it;
  }
  auto &sliders = companion_slider_refs();
  for (auto it = sliders.begin(); it != sliders.end();) {
    if (!it->slider || !lv_obj_is_valid(it->slider)) {
      it = sliders.erase(it);
      continue;
    }
    const auto value = std::find_if(snapshot.values.begin(), snapshot.values.end(),
      [it](const CompanionValue &candidate) { return candidate.id == it->control_id; });
    const bool available = snapshot.connected && value != snapshot.values.end();
    if (available) {
      lv_slider_set_value(it->slider, value->value, LV_ANIM_OFF);
      lv_obj_send_event(it->slider, LV_EVENT_VALUE_CHANGED, nullptr);
      if (it->has_icon_on && it->icon_label && lv_obj_is_valid(it->icon_label)) {
        lv_label_set_text(
          it->icon_label, value->value > 0 ? it->icon_on : it->icon_off);
      }
      lv_obj_clear_state(it->slider, LV_STATE_DISABLED);
    } else {
      lv_obj_add_state(it->slider, LV_STATE_DISABLED);
    }
    ++it;
  }
}
#else
inline void companion_track_card(void *, const std::string &, const std::string & = "", void * = nullptr) {}
inline void companion_track_metric_card(void *, void *, void *, const std::string &,
                                        const std::string &, int) {}
inline void companion_apply_card_focus(void *, const std::string &, const std::string & = "") {}
inline void companion_track_slider(void *, const std::string &, void *, bool,
                                   const char *, const char *) {}
inline void companion_refresh_cards_if_requested() {}
#endif

inline bool invoke_companion_action(const std::string &action_id,
                                    const std::string &request_id) {
  if (!companion_action_available(action_id) || !companion_action_sender()) return false;
  return companion_action_sender()(action_id, request_id);
}

inline bool invoke_companion_url(const std::string &app_id,
                                 const std::string &url_config,
                                 const std::string &request_id) {
  const std::string encoded_url = companion_encoded_url(url_config);
  if (encoded_url.empty() || !companion_action_available(app_id) || !companion_url_sender()) return false;
  return companion_url_sender()(app_id, encoded_url, request_id);
}

inline bool invoke_companion_value(const std::string &control_id, int value,
                                   const std::string &request_id) {
  if (!companion_connected() || !companion_volume_control_valid(control_id) ||
      value < 0 || value > 100 || !companion_value_sender()) return false;
  return companion_value_sender()(control_id, value, request_id);
}

#ifdef USE_WEBSERVER
inline bool companion_authorize_web_request(
    esphome::web_server_idf::AsyncWebServerRequest *request) {
#ifdef USE_WEBSERVER_AUTH
  auto &context = espcontrol::configuration::panel_config_http_context();
  if (!espcontrol::configuration::panel_config_http_context_ready()) {
    httpd_req_t *raw_request = *request;
    httpd_resp_set_status(raw_request, "503 Service Unavailable");
    httpd_resp_set_type(raw_request, "text/plain");
    httpd_resp_send(raw_request, "Web authentication is starting",
                    HTTPD_RESP_USE_STRLEN);
    return false;
  }
  if (!request->authenticate(context.username, context.password)) {
    request->requestAuthentication();
    return false;
  }
#endif
  return true;
}

inline std::string companion_json_escape(const std::string &value) {
  std::string result;
  result.reserve(value.size() + 8);
  for (char ch : value) {
    switch (ch) {
      case '\\': result += "\\\\"; break;
      case '\"': result += "\\\""; break;
      case '\n': result += "\\n"; break;
      case '\r': result += "\\r"; break;
      case '\t': result += "\\t"; break;
      default:
        if (static_cast<unsigned char>(ch) >= 0x20) result.push_back(ch);
    }
  }
  return result;
}

class CompanionActionsHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buf) == "/companion/actions";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!companion_authorize_web_request(request)) return;
    std::string json = "[";
    bool first = true;
    const auto snapshot = companion_runtime_snapshot();
    if (snapshot.connected) {
      for (const auto &action : snapshot.actions) {
        if (!first) json += ",";
        first = false;
        json += "{\"id\":\"" + companion_json_escape(action.id) +
          "\",\"label\":\"" + companion_json_escape(action.label) + "\"}";
      }
    }
    json += "]";
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }
};

inline std::string companion_pairing_json(const CompanionPairingSnapshot &snapshot) {
  return std::string("{\"available\":") + (snapshot.available ? "true" : "false") +
    ",\"active\":" + (snapshot.active ? "true" : "false") +
    ",\"paired\":" + (snapshot.paired ? "true" : "false") +
    ",\"connected\":" + (snapshot.connected ? "true" : "false") +
    ",\"expires_in_seconds\":" + std::to_string(snapshot.expires_in_seconds) +
    ",\"port\":" + std::to_string(snapshot.port) +
    ",\"system_metrics_generation\":" + std::to_string(snapshot.system_metrics_generation) +
    ",\"pairing_code\":\"" + companion_json_escape(snapshot.pairing_code) +
    "\",\"mdns_name\":\"" + companion_json_escape(snapshot.mdns_name) + "\"}";
}

class CompanionPairingHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET && request->method() != HTTP_POST) return false;
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buf) == "/companion/pairing";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (!companion_authorize_web_request(request)) return;
    CompanionPairingSnapshot snapshot;
    if (request->method() == HTTP_POST) {
      if (companion_pairing_starter()) snapshot = companion_pairing_starter()();
    } else if (companion_pairing_provider()) {
      snapshot = companion_pairing_provider()();
    }
    const std::string json = companion_pairing_json(snapshot);
    httpd_req_t *req = *request;
    httpd_resp_set_status(req, snapshot.available ? "200 OK" : "404 Not Found");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, json.c_str(), HTTPD_RESP_USE_STRLEN);
  }
};

inline void register_companion_actions_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (registered) return;
  server.addHandler(new CompanionActionsHandler());
  server.addHandler(new CompanionPairingHandler());
  registered = true;
}

inline void register_companion_actions_endpoint() {
  auto *server = esphome::web_server_idf::global_async_web_server();
  if (server) register_companion_actions_endpoint(*server);
}
#else
inline void register_companion_actions_endpoint() {}
#endif
