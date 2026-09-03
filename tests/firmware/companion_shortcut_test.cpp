#include <cassert>

#include "companion_controls.h"

int main() {
  assert(!companion_connected());
  assert(!companion_card_refresh_requested().load());
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

  const std::vector<std::string> window_actions{
    "window.close", "window.minimize", "window.hide", "window.fullscreen",
    "window.fill", "window.center", "window.left", "window.right", "window.top", "window.bottom",
    "window.restore", "window.arrange.left-right", "window.arrange.right-left",
    "window.arrange.top-bottom", "window.arrange.bottom-top", "window.arrange.left-quarters",
    "window.arrange.right-quarters", "window.arrange.top-quarters", "window.arrange.bottom-quarters",
  };
  for (const auto &action : window_actions) assert(companion_window_action_valid(action));
  assert(companion_window_action_label("window.minimize") == "Minimise");
  assert(companion_window_action_label("window.arrange.left-quarters") == "Left & Quarters");
  assert(!companion_window_action_valid("window.top-left"));
  assert(!companion_window_action_valid("window.arrange"));

  const std::string url_config = "url.https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice";
  assert(companion_encoded_url(url_config) == "https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice");
  assert(companion_encoded_url("url.file%3A%2F%2Fetc%2Fpasswd").empty());
  assert(companion_encoded_url("url.javascript%3Aalert(1)").empty());
  assert(companion_encoded_url("url.https%3A%2F%2Fexample.com|INVOKE").empty());
  assert(companion_encoded_url("url." + std::string(129, 'a')).empty());

  const std::string folder_action = "folder.00000000-0000-0000-0000-000000000001";
  companion_set_actions({{"com.apple.Safari", "Safari"}, {folder_action, "Projects"}});
  assert(companion_card_refresh_requested().load());
  companion_set_connected(true);
  assert(companion_connected());
  assert(companion_metric_key_valid("stat.cpu"));
  assert(!companion_metric_key_valid("sensor.cpu"));
  assert(std::string(companion_metric_label_key("stat.memory")) == "memory");
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
  assert(metric_value == 512.5f);
  assert(!companion_metric_value(companion_runtime_snapshot(), "stat.battery", metric_value));
  companion_set_focused_application("com.apple.Safari");
  assert(companion_application_focused("com.apple.Safari"));
  assert(!companion_application_focused("com.google.Chrome"));
  assert(!companion_application_focused(folder_action));
  assert(companion_media_action_valid("media.play_pause"));
  assert(companion_media_action_valid("media.previous"));
  assert(companion_media_action_valid("media.next"));
  assert(!companion_media_action_valid("media.delete_everything"));
  assert(!companion_action_available("media.play_pause"));
  companion_set_media_actions_supported(true);
  assert(companion_action_available("media.play_pause"));
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
  assert(companion_action_available("window.fill"));
  bool window_invoked = false;
  register_companion_action_sender([&window_invoked](const std::string &action, const std::string &request) {
    window_invoked = action == "window.fill" && request == "window-test";
    return window_invoked;
  });
  assert(invoke_companion_action("window.fill", "window-test"));
  assert(window_invoked);
  assert(!invoke_companion_action("window.unknown", "window-test"));
  assert(companion_action_available(folder_action));
  bool invoked = false;
  register_companion_url_sender([&invoked](const std::string &app, const std::string &url,
                                           const std::string &request) {
    invoked = app == "com.apple.Safari" && url.rfind("https%3A%2F%2F", 0) == 0 && request == "test-1";
    return invoked;
  });
  assert(invoke_companion_url("com.apple.Safari", url_config, "test-1"));
  assert(invoked);
  companion_set_connected(false);
  assert(!companion_metric_value(companion_runtime_snapshot(), "stat.cpu", metric_value));
  assert(!companion_application_focused("com.apple.Safari"));
  companion_set_connected(true);
  assert(!companion_action_available("media.play_pause"));
  assert(!companion_value("media.output_volume", output_volume));
  return 0;
}
