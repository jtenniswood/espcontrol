#!/usr/bin/env python3
"""Exercise the actual screensaver requester when artwork attributes never reply."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
yaml = (root / "common/device/screen_cover_art.yaml").read_text()
script = yaml.split("  - id: cover_art_request_paired_artwork\n", 1)[1].split("\n  - id:", 1)[0]
request = script.split("- lambda: |-\n", 1)[1]
ha = (root / "components/espcontrol/button_grid_ha.h").read_text()
wrapper = ha.split("inline bool ha_read_retained_attribute(", 1)[1].split(
    "inline bool ha_read_retained_state(", 1
)[0]

source = r'''
#include <cassert>
#include <functional>
#include <string>
#include <vector>
#include "cover_art.h"
#include "ha_read_coordinator.h"
namespace esphome { using StringRef = std::string; }
namespace espcontrol { enum class DisplayMode { COVER_ART }; }
struct Transport {
  using State = esphome::StringRef;
  using Callback = std::function<void(State)>;
  std::vector<Callback> callbacks;
  bool available() const { return true; }
  bool state_connected() const { return true; }
  void subscribe(const std::string&, const std::string&, Callback callback) {
    callbacks.push_back(std::move(callback));
  }
};
struct Heap { bool available(const char*, size_t, size_t) { return true; } };
HaReadCoordinator<Transport, Heap> coordinator;
auto &ha_read_coordinator() { return coordinator; }
void *ha_callback_owner() { return nullptr; }
struct lv_obj_t {};
bool lv_obj_is_valid(lv_obj_t*) { return false; }
using HomeAssistantStateCallback = Transport::Callback;
constexpr size_t HA_READ_INTERNAL_FREE_MIN_BYTES = 8192;
constexpr size_t HA_READ_INTERNAL_LARGEST_MIN_BYTES = 4096;
struct Switch { bool state = true; } cover_art_screensaver_enabled;
bool cover_art_media_playing = true;
espcontrol::cover_art::PlaybackControl cover_art_playback_control;
struct Script {
  int calls = 0;
  void execute() { ++calls; }
  bool is_running() const { return false; }
} cover_art_delayed_playback_stopped, cover_art_process_cached_artwork,
  cover_art_deferred_request_artwork;
struct Display { bool target_mode_is(espcontrol::DisplayMode) { return false; } };
struct App { Display display() { return {}; } } espcontrol_app;
espcontrol::cover_art::RuntimeState cover_art_runtime;
espcontrol::artwork::RefreshTrigger cover_art_artwork_trigger;
int cover_art_subscription_generation = 1;
uint8_t cover_art_artwork_retry_mask = 0;
bool cover_art_local_source_relative = false, cover_art_remote_source_relative = false;
std::string cover_art_active_media_player_entity = "media_player.room";
std::string cover_art_home_assistant_base_url = "http://homeassistant";
std::string string_ref_limited(esphome::StringRef value, size_t limit) {
  return value.substr(0, limit);
}
#define id(x) x
#define ESP_LOGD(...) do {} while(false)
inline bool ha_read_retained_attribute(''' + wrapper + r'''
void request_artwork() {
''' + request + r'''
}
void publish(size_t channel, const std::string &value) {
  auto callback = coordinator.transport().callbacks.at(channel);
  callback(value);
}
int main() {
  assert(coordinator.subscribe("media_player.room", "entity_picture",
                               [](std::string) {}, 1u, nullptr, true));
  assert(coordinator.subscribe("media_player.room", "entity_picture_local",
                               [](std::string) {}, 1u, nullptr, true));
  // A common player supplies remote artwork but has no entity_picture_local.
  publish(0, "/remote.jpg");
  size_t capacity = 0;
  for (int refresh = 0; refresh < 300; ++refresh) {
    request_artwork();
    assert(coordinator.pending_read_count() == 1);
    assert(cover_art_artwork_retry_mask == 0);
    if (refresh == 0) capacity = coordinator.persistent_container_capacity_bytes();
    assert(coordinator.persistent_container_capacity_bytes() == capacity);
  }
  // The surviving callback belongs to the latest batch and still works.
  publish(1, "/local.jpg");
  assert(coordinator.pending_read_count() == 0);
  assert(cover_art_runtime.sources.get(true) == "http://homeassistant/local.jpg");
  assert(cover_art_runtime.artwork_refresh.complete());
  // Also cover providers/reconnects that haven't returned either attribute.
  coordinator.invalidate_retained_state();
  cover_art_runtime.clear_image();
  for (int refresh = 0; refresh < 300; ++refresh) {
    request_artwork();
    assert(coordinator.pending_read_count() == 2);
    assert(cover_art_artwork_retry_mask == 0);
    if (refresh == 0) capacity = coordinator.persistent_container_capacity_bytes();
    assert(coordinator.persistent_container_capacity_bytes() == capacity);
  }
  publish(0, "/new-remote.jpg");
  publish(1, "/new-local.jpg");
  assert(coordinator.pending_read_count() == 0);
  assert(cover_art_runtime.artwork_refresh.complete());
  assert(cover_art_runtime.sources.get(false) == "http://homeassistant/new-remote.jpg");
  assert(cover_art_runtime.sources.get(true) == "http://homeassistant/new-local.jpg");
  assert(coordinator.transport().callbacks.size() == 2);
  // Idle subscription/metadata notifications must not recreate stopped reads.
  coordinator.invalidate_retained_state();
  cover_art_media_playing = false;
  request_artwork();
  assert(coordinator.pending_read_count() == 0);
  // A pause requested on the screensaver intentionally keeps its artwork live.
  cover_art_playback_control.begin(cover_art_active_media_player_entity, "playing", 1);
  cover_art_playback_control.observe(cover_art_active_media_player_entity, "paused", 2);
  request_artwork();
  assert(coordinator.pending_read_count() == 2);
}
'''
with tempfile.TemporaryDirectory(prefix="cover-art-pending-") as temp:
    cpp = Path(temp) / "pending.cpp"
    cpp.write_text(source)
    binary = Path(temp) / "pending"
    subprocess.run([sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                    "-I", str(root / "components/espcontrol"), str(cpp), "-o", str(binary)],
                   check=True)
    subprocess.run([str(binary)], check=True)
    print("Absent artwork replies: repeated screensaver reads stay bounded and latest replies work")
