#include <cassert>
#include <string>

struct lv_obj_t {};
inline void lv_label_set_text(lv_obj_t *, const char *) {}
inline std::string espcontrol_i18n(const std::string &value) { return value; }

#include "companion_controls.h"
#include "companion_timezone.h"
#include "button_grid_config_parser.h"

using namespace esphome::companion;

int main() {
  assert(!companion_connected());
  assert(!companion_card_refresh_requested().load());
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::PLAYING)) == "Playing");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::PAUSED)) == "Paused");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::STOPPED)) == "Stopped");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::UNAVAILABLE, false)) == "Unavailable");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::UNAVAILABLE)) == "Stopped");
  assert(companion_shortcut_action_valid("shortcut.command+a"));
  assert(companion_shortcut_label("shortcut.command+a") == "\U000F0633" "A");
  assert(companion_shortcut_action_valid("shortcut.control+shift+tab"));
  assert(companion_shortcut_label("shortcut.control+shift+tab") == "\U000F0634\U000F0636" "Tab");
  assert(companion_shortcut_action_valid("shortcut.option+f20"));
  assert(companion_shortcut_label("shortcut.option+f20") == "\U000F0635" "F20");
  assert(companion_shortcut_label("shortcut.command+left") == "\U000F0633\U000F004D");

  assert(!companion_shortcut_action_valid("shortcut.shift+a"));
  assert(!companion_shortcut_action_valid("shortcut.command+command+a"));
  assert(!companion_shortcut_action_valid("shortcut.command+volumeup"));
  assert(!companion_shortcut_action_valid("shortcut.command+f21"));
  assert(!companion_shortcut_action_valid("com.apple.Safari"));

  ParsedCfg safari_launch;
  safari_launch.type = "companion";
  safari_launch.entity = "com.apple.Safari";
  safari_launch.options = "app_shortcuts";
  assert(companion_app_shortcuts_enabled(safari_launch));
  assert(!companion_app_subpage_auto_switch_enabled(safari_launch));
  safari_launch.options = "app_shortcuts,app_shortcuts_auto_switch";
  assert(companion_app_subpage_auto_switch_enabled(safari_launch));
  assert(companion_card_options_normalized(safari_launch) ==
         "app_shortcuts,app_shortcuts_auto_switch");
  ParsedCfg codex_launch = safari_launch;
  codex_launch.entity = "com.openai.codex";
  assert(companion_app_shortcuts_enabled(codex_launch));
  ParsedCfg slack_launch = safari_launch;
  slack_launch.entity = "com.tinyspeck.slackmacgap";
  assert(companion_app_shortcuts_enabled(slack_launch));
  safari_launch.sensor = "url.https%3A%2F%2Fexample.com";
  assert(!companion_app_shortcuts_enabled(safari_launch));
  assert(!companion_app_subpage_auto_switch_enabled(safari_launch));

  ParsedCfg companion_stat_subpage;
  companion_stat_subpage.type = "subpage";
  companion_stat_subpage.entity = "stat.network_throughput";
  companion_stat_subpage.options = "subpage_kind=companion_stat";
  assert(companion_config_tracker_preserves_card_binding(companion_stat_subpage));
  assert(std::string(subpage_companion_stat_default_label(companion_stat_subpage.entity)) == "Network");
  ParsedCfg ordinary_subpage = companion_stat_subpage;
  ordinary_subpage.options = "subpage_kind=media";
  assert(!companion_config_tracker_preserves_card_binding(ordinary_subpage));
  assert(companion_config_tracker_preserves_card_binding(safari_launch));

  const std::string url_config = "url.https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice";
  assert(companion_encoded_url(url_config) == "https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice");
  assert(companion_encoded_url("url.file%3A%2F%2Fetc%2Fpasswd").empty());
  assert(companion_encoded_url("url.javascript%3Aalert(1)").empty());
  assert(companion_encoded_url("url.https%3A%2F%2Fexample.com|INVOKE").empty());
  assert(companion_encoded_url("url." + std::string(129, 'a')).empty());
  assert(companion_default_action_label("window.arrange.left-right") == "Left & Right");
  assert(companion_default_action_label("shortcut.command+a") == "\U000F0633" "A");
  assert(companion_default_action_label("com.apple.Safari") == "com.apple.Safari");
  assert(companion_default_action_label("com.apple.Safari", url_config) == "Open URL");
  assert(companion_default_action_label("") == "Mac App");

  const std::string folder_action = "folder.00000000-0000-0000-0000-000000000001";
  companion_set_actions({{"com.apple.Safari", "Safari"}, {folder_action, "Projects"}});
  assert(companion_card_refresh_requested().load());
  companion_set_connected(true);
  assert(companion_connected());
  assert(companion_window_action_valid("window.left"));
  assert(companion_action_available("window.left"));
  assert(!companion_action_available("window.not-real"));
  assert(companion_metric_key_valid("stat.cpu"));
  assert(!companion_metric_key_valid("sensor.cpu"));
  assert(std::string(companion_metric_label_key("stat.memory")) == "memory");
  assert(std::string(companion_metric_default_unit("stat.network_throughput")) == "MB/s");
  CompanionSystemMetricsSnapshot metrics;
  metrics.generation = 1;
  metrics.cpu_usage_percent = 42.5f;
  metrics.memory_usage_percent = 61.0f;
  metrics.storage_usage_percent = 73.0f;
  metrics.network_throughput_kbps = 512.5f;
  companion_set_system_metrics(metrics);
  float metric_value = 0.0f;
  assert(companion_metric_value(companion_runtime_snapshot(), "stat.cpu", metric_value));
  assert(metric_value == 42.5f);
  assert(companion_metric_value(companion_runtime_snapshot(), "stat.memory_free", metric_value));
  assert(metric_value == 39.0f);
  assert(companion_metric_value(companion_runtime_snapshot(), "stat.storage_free", metric_value));
  assert(metric_value == 27.0f);
  assert(companion_metric_value(companion_runtime_snapshot(), "stat.network_throughput", metric_value));
  assert(metric_value == 512.5f / 1024.0f);
  assert(!companion_metric_value(companion_runtime_snapshot(), "stat.battery", metric_value));
  companion_set_focused_action("com.apple.Safari");
  assert(companion_pending_auto_subpage_action() == "com.apple.Safari");
  assert(companion_consume_auto_subpage_action("com.apple.Safari"));
  assert(companion_pending_auto_subpage_action().empty());
  assert(companion_action_focused("com.apple.Safari"));
  assert(!companion_action_focused("com.google.Chrome"));
  assert(!companion_action_focused(folder_action));
  companion_set_focused_action(folder_action);
  assert(companion_action_focused(folder_action));
  assert(!companion_action_focused("com.apple.Safari"));
  assert(companion_consume_subpage_return_request());
  assert(companion_pending_auto_subpage_action() == folder_action);
  assert(!companion_consume_auto_subpage_action("com.apple.Safari"));
  assert(companion_consume_auto_subpage_action(folder_action));
  companion_set_focused_action(folder_action);
  assert(companion_pending_auto_subpage_action().empty());
  assert(!companion_consume_subpage_return_request());
  companion_set_focused_action("com.apple.Safari");
  assert(companion_consume_subpage_return_request());
  companion_set_focused_action("");
  assert(companion_consume_subpage_return_request());
  assert(!companion_consume_subpage_return_request());
  assert(companion_media_action_valid("media.play_pause"));
  assert(companion_media_action_valid("media.previous"));
  assert(companion_media_action_valid("media.next"));
  assert(!companion_media_action_valid("media.delete_everything"));
  assert(!companion_action_available("media.play_pause"));
  companion_set_media_actions_supported(true);
  assert(companion_action_available("media.play_pause"));
  CompanionNowPlayingSnapshot paused_snapshot;
  paused_snapshot.playback_state = CompanionPlaybackState::PAUSED;
  companion_set_now_playing(paused_snapshot);
  assert(companion_action_available("media.play_pause"));
  assert(!companion_action_active("media.play_pause"));
  CompanionNowPlayingSnapshot playing_snapshot;
  playing_snapshot.playback_state = CompanionPlaybackState::PLAYING;
  companion_set_now_playing(playing_snapshot);
  assert(companion_action_active("media.play_pause"));
  companion_set_now_playing(paused_snapshot);
  bool media_invoked = false;
  register_companion_action_sender([&media_invoked](const std::string &action,
                                                    const std::string &request) {
    media_invoked = action == "media.play_pause" && request == "media-1";
    return media_invoked;
  });
  assert(invoke_companion_action("media.play_pause", "media-1"));
  assert(media_invoked);
  assert(companion_runtime_snapshot().now_playing.playback_state == CompanionPlaybackState::PAUSED);
  assert(companion_volume_control_valid("media.output_volume"));
  assert(companion_volume_control_valid("media.input_volume"));
  assert(!companion_volume_control_valid("media.screen_brightness"));
  companion_set_value("media.output_volume", 72);
  int output_volume = 0;
  assert(companion_value("media.output_volume", output_volume));
  assert(output_volume == 72);
  companion_remove_value("media.output_volume");
  assert(!companion_value("media.output_volume", output_volume));
  companion_set_value("media.output_volume", 72);
  bool volume_invoked = false;
  register_companion_value_sender([&volume_invoked](const std::string &control, int value,
                                                     const std::string &request) {
    volume_invoked = control == "media.output_volume" && value == 64 && request == "volume-1";
    return volume_invoked;
  });
  assert(!invoke_companion_value("media.output_volume", -1, "volume-toggle"));
  assert(!volume_invoked);
  assert(invoke_companion_value("media.output_volume", 64, "volume-1"));
  assert(volume_invoked);
  assert(companion_url_available("com.apple.Safari", url_config));
  assert(!companion_url_available("com.google.Chrome", url_config));
  // Companion reports the focused app, not the page it currently shows, so a
  // URL card must never inherit the app-launch card's checked state.
  companion_set_focused_application("com.apple.Safari");
  assert(companion_application_focused("com.apple.Safari"));
  assert(!companion_card_focus_allowed(url_config));
  assert(companion_card_focus_allowed(""));
  assert(companion_action_available(folder_action));
  bool invoked = false;
  register_companion_url_sender([&invoked](const std::string &app, const std::string &url,
                                           const std::string &request) {
    invoked = app == "com.apple.Safari" && url.rfind("https%3A%2F%2F", 0) == 0 && request == "test-1";
    return invoked;
  });
  assert(invoke_companion_url("com.apple.Safari", url_config, "test-1"));
  assert(invoked);
  bool navigated = false;
  companion_expect_action_result("launch-1", [&navigated]() { navigated = true; });
  companion_deliver_action_result("another-request", "performed");
  assert(!navigated);
  companion_deliver_action_result("launch-1", "not_allowed");
  assert(!navigated);
  companion_deliver_action_result("launch-1", "performed");
  assert(!navigated);
  companion_expect_action_result("launch-2", [&navigated]() { navigated = true; });
  companion_deliver_action_result("launch-2", "performed");
  assert(!navigated);
  companion_expect_action_result("launch-3", [&navigated]() { navigated = true; });
  companion_deliver_action_result("launch-3", "opened");
  assert(!navigated);
  companion_expect_action_result("launch-4", [&navigated]() { navigated = true; });
  companion_deliver_action_result("launch-4", "activated");
  assert(navigated);
  navigated = false;
  companion_expect_action_result("launch-focus", "com.apple.Safari",
                                 [&navigated]() { navigated = true; });
  companion_set_focused_application("com.apple.Safari");
  assert(navigated);
  companion_deliver_action_result("launch-focus", "activated");
  assert(navigated);
  companion_set_timezone_id("Europe/London");
  assert(companion_timezone_id() == "Europe/London");
  assert(companion_timezone_changed());
  assert(companion_take_timezone_changed());
  assert(!companion_timezone_changed());
  companion_set_connected(false);
  assert(companion_consume_subpage_return_request());
  companion_set_timezone_id("");
  assert(companion_timezone_id().empty());
  assert(companion_timezone_changed());
  assert(companion_take_timezone_changed());
  assert(!companion_timezone_changed());
  assert(!companion_metric_value(companion_runtime_snapshot(), "stat.cpu", metric_value));
  assert(!companion_application_focused("com.apple.Safari"));
  assert(!companion_action_active("media.play_pause"));
  assert(companion_runtime_snapshot().now_playing.playback_state == CompanionPlaybackState::UNAVAILABLE);
  companion_set_connected(true);
  assert(!companion_action_available("media.play_pause"));
  assert(!companion_value("media.output_volume", output_volume));
  return 0;
}
