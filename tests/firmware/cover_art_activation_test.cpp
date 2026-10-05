// Compiled by cover_art_activation_test.py with production YAML action bodies.
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include "esphome_automation_chain.h"
#include "cover_art.h"
#include "display_mode_controller.h"
#include "artwork_url.h"
#include "ha_read_coordinator.h"

using Action = esphome::Action<>;
using ActionList = esphome::ActionList<>;
int depth = 0, peak_depth = 0, artwork_depth = 0, layout_depth = 0;
uint64_t now_ms = 0;
struct Event { const void *key; uint64_t due; std::function<void()> callback; };
std::vector<Event> events;
void cancel(const void *key) {
  events.erase(std::remove_if(events.begin(), events.end(),
                             [key](auto &e) { return e.key == key; }), events.end());
}
void schedule(const void *key, uint32_t delay, std::function<void()> callback) {
  cancel(key);
  events.push_back({key, now_ms + delay, std::move(callback)});
}
void tick() {
  ++now_ms;
  while (true) {
    auto it = std::find_if(events.begin(), events.end(), [](auto &event) { return event.due <= now_ms; });
    if (it == events.end()) break;
    auto callback = std::move(it->callback);
    events.erase(it);
    callback();
  }
}
void drain() {
  for (int i = 0; !events.empty() && i < 5000; ++i) tick();
  assert(events.empty());
}

struct Script {
  ActionList actions;
  std::tuple<int, int, std::string> args;
  std::function<void()> io;
  int calls = 0;
  void execute() {
    if (actions.is_running()) actions.stop();
    ++calls;
    if (io) io();
    actions.play();
  }
  void execute(int generation) { args = {generation, 0, ""}; execute(); }
  void execute(int generation, int mode) { args = {generation, mode, ""}; execute(); }
  void execute(int generation, int subscription, const std::string &entity) {
    if (actions.is_running()) actions.stop();
    args = {generation, subscription, entity};
    execute();
  }
  void stop() { actions.stop(); }
  bool is_running() { return actions.is_running(); }
};
struct CallbackAction : Action {
  std::function<void()> callback;
  explicit CallbackAction(std::function<void()> fn) : callback(std::move(fn)) {}
  void play_complex() override {
    ++depth; peak_depth = std::max(peak_depth, depth);
    Action::play_complex();
    --depth;
  }
  void play() override { callback(); }
};
struct Delay : Action {
  uint32_t delay;
  explicit Delay(uint32_t delay) : delay(delay) {}
  void play_complex() override {
    ++this->num_running_;
    schedule(this, delay, [this] { this->play_next_(); });
  }
  void play() override {}
  void stop() override { cancel(this); }
};
struct Wait : Delay {
  Script &target;
  explicit Wait(Script &target) : Delay(1), target(target) {}
  void poll() {
    if (!target.is_running()) this->play_next_();
    else schedule(this, 1, [this] { poll(); });
  }
  void play_complex() override { ++this->num_running_; poll(); }
};
struct Branch : Action {
  std::function<bool()> condition;
  ActionList yes, no;
  CallbackAction continuation{[this] { this->play_next_(); }};
  explicit Branch(std::function<bool()> condition) : condition(std::move(condition)) {}
  void play_complex() override {
    ++this->num_running_;
    (condition() ? yes : no).play();
  }
  void play() override {}
  void stop() override { yes.stop(); no.stop(); }
};
std::vector<std::unique_ptr<Action>> owned;
Action *make_action(Script &, std::function<void()> callback) {
  owned.push_back(std::make_unique<CallbackAction>(std::move(callback)));
  return owned.back().get();
}
Action *make_delay(Script &, uint32_t delay) {
  owned.push_back(std::make_unique<Delay>(delay)); return owned.back().get();
}
Action *make_wait(Script &, Script &target) {
  owned.push_back(std::make_unique<Wait>(target)); return owned.back().get();
}
Action *make_if(Script &, std::function<bool()> condition,
                std::initializer_list<Action *> yes, std::initializer_list<Action *> no) {
  auto node = std::make_unique<Branch>(std::move(condition));
  node->yes.add_actions(yes); node->yes.add_action(&node->continuation);
  node->no.add_actions(no); node->no.add_action(&node->continuation);
  owned.push_back(std::move(node)); return owned.back().get();
}

struct App {
  espcontrol::DisplayModeController controller;
  auto &display() { return controller; }
} espcontrol_app;
struct BoolSetting { bool state = true; } cover_art_screensaver_enabled,
    cover_art_hide_external_input_enabled;
struct TextSetting { std::string state = "media_player.issue1854"; } cover_art_media_player_entity;
espcontrol::cover_art::RuntimeState cover_art_runtime;
uint32_t cover_art_transition_generation = 0;
int cover_art_subscription_generation = 1, cover_art_request_id = 0,
    cover_art_artwork_pending_count = 0;
bool cover_art_attribute_conditions_match = true, cover_art_external_input_active = false,
    cover_art_media_playing = true;
std::string cover_art_active_media_player_entity = "media_player.issue1854",
    cover_art_pending_remote_url, cover_art_pending_local_url;
bool image_source_detached = false;
struct Image {
  int releases = 0;
  void cancel_update() {}
  void release() { assert(image_source_detached); ++releases; }
} image;
Image *cover_art_downloaded_image = &image;
espcontrol::cover_art::PlaybackControl cover_art_playback_control;
espcontrol::artwork::RefreshTrigger cover_art_artwork_trigger;
uint8_t cover_art_artwork_retry_mask = 0;
bool cover_art_delay_interrupted_by_transition = false;
uint32_t cover_art_manual_pause_until_ms = 0;
float cover_art_media_position = 0, cover_art_position_anchor = 0, cover_art_media_duration = 0;
int cover_art_position_anchor_epoch = 0, cover_art_last_position_timestamp = 0,
    cover_art_last_progress_percent = -1;
std::string cover_art_last_playback_state = "playing";
int cover_art_progress_bar = 0, cover_art_time_label = 0;
constexpr int LV_ANIM_OFF = 0;
void lv_bar_set_value(int, int, int) {}
void lv_label_set_text(int, const char*) {}
struct Transport {
  using State = std::string;
  using Callback = std::function<void(State)>;
  bool available() const { return true; }
  bool state_connected() const { return true; }
  void subscribe(const std::string&, const std::string&, Callback) {}
};
struct Heap { bool available(const char*, size_t, size_t) { return true; } };
HaReadCoordinator<Transport, Heap> coordinator;
void ha_release_callbacks_for_owner(void *owner) { coordinator.release_owner(owner); }
void ha_log_subscription_diagnostics(const char*) {}
int cover_art_image_widget = 0;
constexpr int LV_OBJ_FLAG_HIDDEN = 1;
void lv_obj_add_flag(int, int) {}
#define id(x) x
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
// GENERATED_SCRIPT_DECLARATIONS

void reset() {
  display_mode_effect_cover_art.stop();
  cover_art_prepare_activation.stop();
  cover_art_deferred_download.stop();
  cover_art_delayed_playback_stopped.stop();
  events.clear();
  espcontrol_app.controller = {};
  espcontrol_app.controller.request(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK,
                                   espcontrol::DisplayMode::COVER_ART);
  cover_art_transition_generation = espcontrol_app.controller.generation();
  cover_art_subscription_generation = 1;
  cover_art_screensaver_enabled.state = true;
  cover_art_attribute_conditions_match = true;
  cover_art_external_input_active = false;
  cover_art_active_media_player_entity = "media_player.issue1854";
  cover_art_playback_control.reset();
  cover_art_media_playing = true;
  cover_art_last_playback_state = "playing";
  image.releases = 0;
  image_source_detached = false;
  coordinator.invalidate_retained_state();
  cover_art_runtime = {};
  cover_art_runtime.sources.update(false, "https://example.test/art.jpg");
  cover_art_request_artwork.calls = cover_art_use_cached_artwork.calls = 0;
  cover_art_download.calls = cover_art_prepare_activation.calls = 0;
  depth = peak_depth = artwork_depth = layout_depth = 0;
}

int main(int argc, char **argv) {
  const bool baseline = argc > 1 && std::string(argv[1]) == "baseline";
  // GENERATED_SCRIPT_SETUP
  cover_art_apply_responsive_layout.io = [] { layout_depth = depth; };
  cover_art_request_artwork.io = [] { artwork_depth = depth; };
  cover_art_clear_image_source.io = [] { image_source_detached = true; };
  display_mode_clear_cover_art.io = [] {
    espcontrol_app.controller.clear(espcontrol::DisplayRequestSource::MEDIA_PLAYBACK);
  };
  cover_art_download.io = [] {
    cover_art_runtime.image_available = true;
    cover_art_runtime.loaded_url = cover_art_runtime.source_url;
    cover_art_runtime.refresh_needed = false;
  };
  reset();
  display_mode_effect_cover_art.execute(cover_art_transition_generation);
  if (baseline) {
    assert(layout_depth > 0 && artwork_depth > 20);
    std::printf("Baseline: artwork ran inline at action depth %d; layout depth %d\n", artwork_depth, layout_depth);
    return 0;
  }
  assert(layout_depth == 0 && artwork_depth == 0);
  tick();  // Visual effect has resumed; artwork must still be deferred.
  assert(layout_depth > 0 && artwork_depth == 0);
  assert(display_mode_effect_cover_art.is_running());
  drain();
  assert(artwork_depth < 5);
  assert(cover_art_use_cached_artwork.calls == 1 && cover_art_download.calls == 1);
  assert(!display_mode_effect_cover_art.is_running());
  std::printf("Fixed: artwork action depth %d; layout depth %d\n", artwork_depth, layout_depth);

  // Restart coalesces all pending activations to a single request.
  reset();
  for (int i = 0; i < 20; ++i)
    cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
  drain();
  assert(cover_art_request_artwork.calls == 1 && cover_art_download.calls == 1);

  // Cancellation, superseding generations, routing and feature eligibility.
  for (int mode = 0; mode < 7; ++mode) {
    reset();
    cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
    switch (mode) {
      case 0: cover_art_prepare_activation.stop(); break;
      case 1: espcontrol_app.controller.request(espcontrol::DisplayRequestSource::USER_WAKE,
                                               espcontrol::DisplayMode::ACTIVE); break;
      case 2: ++cover_art_subscription_generation; break;
      case 3: cover_art_active_media_player_entity = "media_player.second"; break;
      case 4: cover_art_screensaver_enabled.state = false; break;
      case 5: cover_art_attribute_conditions_match = false; break;
      case 6: cover_art_external_input_active = true; break;
    }
    drain();
    assert(cover_art_request_artwork.calls == 0 && cover_art_download.calls == 0);
  }
  // A queued visual effect must not run after Wake either.
  reset();
  display_mode_effect_cover_art.execute(cover_art_transition_generation);
  espcontrol_app.controller.request(espcontrol::DisplayRequestSource::USER_WAKE, espcontrol::DisplayMode::ACTIVE);
  drain();
  assert(layout_depth == 0 && artwork_depth == 0);

  // An obsolete hide callback cannot cancel a newer activation.
  reset();
  cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
  cover_art_hide_effect.execute(cover_art_transition_generation - 1,
                               static_cast<int>(espcontrol::DisplayMode::ACTIVE));
  drain();
  assert(cover_art_request_artwork.calls == 1);
  reset();
  cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
  espcontrol_app.controller.request(espcontrol::DisplayRequestSource::USER_WAKE, espcontrol::DisplayMode::ACTIVE);
  cover_art_hide_effect.execute(espcontrol_app.controller.generation(),
                               static_cast<int>(espcontrol::DisplayMode::ACTIVE));
  assert(!cover_art_prepare_activation.is_running());
  drain();
  assert(cover_art_request_artwork.calls == 0);

  // Cached image remains healthy; same-URL metadata changes still refresh.
  for (bool changed : {false, true}) {
    reset();
    cover_art_runtime.source_url = cover_art_runtime.loaded_url = "https://example.test/art.jpg";
    cover_art_runtime.image_available = true;
    cover_art_runtime.refresh_needed = changed;
    cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
    drain();
    assert(cover_art_download.calls == (changed ? 1 : 0));
  }
  // Missing companion/remote metadata must settle without fake downloads.
  reset();
  cover_art_runtime.sources.clear();
  cover_art_prepare_activation.execute(cover_art_transition_generation, 1, cover_art_active_media_player_entity);
  drain();
  assert(cover_art_download.calls == 0);
  // Late state after reconnect uses the new subscription generation.
  ++cover_art_subscription_generation;
  cover_art_runtime.sources.update(false, "https://example.test/new.jpg");
  cover_art_prepare_activation.execute(cover_art_transition_generation, 2, cover_art_active_media_player_entity);
  drain();
  assert(cover_art_download.calls == 1);
  std::puts("Cover Art activation: scheduling, ownership, caching and cancellation passed");

  // Playback-stop teardown also works after an earlier Wake hid the view.
  assert(coordinator.subscribe("media_player.issue1854", "entity_picture_local",
                               [](std::string) {}, 1u, nullptr, true));
  for (bool already_hidden : {false, true}) {
    reset();
    if (already_hidden)
      espcontrol_app.controller.request(espcontrol::DisplayRequestSource::USER_WAKE,
                                        espcontrol::DisplayMode::ACTIVE);
    cover_art_runtime.image_available = true;
    cover_art_runtime.loaded_url = "https://example.test/art.jpg";
    assert(coordinator.read_retained("media_player.issue1854", "entity_picture_local",
                                    [](std::string) {}, true, 1, 1, &cover_art_runtime));
    // Another consumer's pending read must survive screensaver cleanup.
    assert(coordinator.read_retained("media_player.issue1854", "entity_picture_local",
                                    [](std::string) {}, true, 1, 1, &image));
    cover_art_playback_stopped.execute();
    drain();
    assert(image.releases == 1 && image_source_detached);
    assert(!cover_art_runtime.image_available && cover_art_runtime.loaded_url.empty());
    assert(coordinator.pending_read_count() == 1);
    assert(!cover_art_media_playing);
  }

  // The two-second debounce protects normal track gaps and resumed playback.
  for (const std::string state : {"playing", "buffering", "paused"}) {
    reset();
    cover_art_last_playback_state = "idle";
    cover_art_delayed_playback_stopped.execute(cover_art_transition_generation);
    for (int i = 0; i < 1000; ++i) tick();
    assert(image.releases == 0);
    cover_art_last_playback_state = state;
    drain();
    assert(image.releases == 0);
  }
  // A stop callback from before Wake still releases its unused artwork.
  reset();
  cover_art_last_playback_state = "idle";
  cover_art_delayed_playback_stopped.execute(cover_art_transition_generation);
  espcontrol_app.controller.request(espcontrol::DisplayRequestSource::USER_WAKE,
                                    espcontrol::DisplayMode::ACTIVE);
  drain();
  assert(image.releases == 1 && !cover_art_media_playing);
  assert(espcontrol_app.controller.target_mode_is(espcontrol::DisplayMode::ACTIVE));

  // Cleanup cannot retire artwork that is playing or retained by a local pause.
  reset();
  cover_art_release_stopped_artwork.execute();
  drain();
  assert(image.releases == 0);
  cover_art_playback_control.begin(cover_art_active_media_player_entity, "playing", 1);
  cover_art_playback_control.observe(cover_art_active_media_player_entity, "paused", 2);
  cover_art_media_playing = false;
  cover_art_release_stopped_artwork.execute();
  drain();
  assert(image.releases == 0);
  // Playback can resume while return-home is finishing its display transition.
  reset();
  display_mode_clear_cover_art.io = [] { cover_art_media_playing = true; };
  cover_art_playback_stopped.execute();
  drain();
  assert(image.releases == 0);
  std::printf("Playback teardown: stop, Wake, debounce and retained pause checks passed\n");
}
