#include <cassert>

#include "companion_controls.h"

int main() {
  assert(!companion_connected());
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

  const std::string url_config = "url.https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice";
  assert(companion_encoded_url(url_config) == "https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice");
  assert(companion_encoded_url("url.file%3A%2F%2Fetc%2Fpasswd").empty());
  assert(companion_encoded_url("url.javascript%3Aalert(1)").empty());
  assert(companion_encoded_url("url.https%3A%2F%2Fexample.com|INVOKE").empty());

  companion_set_actions({{"com.apple.Safari", "Safari"}});
  companion_set_connected(true);
  assert(companion_connected());
  assert(companion_media_action_valid("media.play_pause"));
  assert(companion_media_action_valid("media.previous"));
  assert(companion_media_action_valid("media.next"));
  assert(!companion_media_action_valid("media.delete_everything"));
  assert(companion_action_available("media.play_pause"));
  assert(companion_volume_control_valid("media.output_volume"));
  assert(companion_volume_control_valid("media.input_volume"));
  assert(!companion_volume_control_valid("media.screen_brightness"));
  companion_set_value("media.output_volume", 72);
  int output_volume = 0;
  assert(companion_value("media.output_volume", output_volume));
  assert(output_volume == 72);
  bool volume_invoked = false;
  register_companion_value_sender([&volume_invoked](const std::string &control, int value,
                                                     const std::string &request) {
    volume_invoked = control == "media.output_volume" && value == 64 && request == "volume-1";
    return volume_invoked;
  });
  assert(invoke_companion_value("media.output_volume", 64, "volume-1"));
  assert(volume_invoked);
  assert(companion_url_available("com.apple.Safari", url_config));
  assert(!companion_url_available("com.google.Chrome", url_config));
  bool invoked = false;
  register_companion_url_sender([&invoked](const std::string &app, const std::string &url,
                                           const std::string &request) {
    invoked = app == "com.apple.Safari" && url.rfind("https%3A%2F%2F", 0) == 0 && request == "test-1";
    return invoked;
  });
  assert(invoke_companion_url("com.apple.Safari", url_config, "test-1"));
  assert(invoked);
  companion_set_connected(false);
  assert(!companion_value("media.output_volume", output_volume));
  return 0;
}
