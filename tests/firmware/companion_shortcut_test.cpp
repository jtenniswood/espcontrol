#include <cassert>

#include "companion_controls.h"

int main() {
  assert(!companion_connected());
  assert(companion_shortcut_action_valid("shortcut.command+a"));
  assert(companion_shortcut_label("shortcut.command+a") == "Cmd+A");
  assert(companion_shortcut_action_valid("shortcut.control+shift+tab"));
  assert(companion_shortcut_label("shortcut.control+shift+tab") == "Ctrl+Shift+Tab");
  assert(companion_shortcut_action_valid("shortcut.option+f20"));

  assert(!companion_shortcut_action_valid("shortcut.shift+a"));
  assert(!companion_shortcut_action_valid("shortcut.command+command+a"));
  assert(!companion_shortcut_action_valid("shortcut.command+volumeup"));
  assert(!companion_shortcut_action_valid("shortcut.command+f21"));
  assert(!companion_shortcut_action_valid("com.apple.Safari"));
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::PLAYING)) == "Playing");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::PAUSED)) == "Paused");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::STOPPED)) == "Stopped");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::UNAVAILABLE)) == "Unavailable");
  assert(std::string(companion_play_pause_status(CompanionPlaybackState::PLAYING, false)) == "Unavailable");

  const std::string url_config = "url.https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice";
  assert(companion_encoded_url(url_config) == "https%3A%2F%2Fexample.com%2Fdashboard%3Froom%3Doffice");
  assert(companion_encoded_url("url.file%3A%2F%2Fetc%2Fpasswd").empty());
  assert(companion_encoded_url("url.javascript%3Aalert(1)").empty());
  assert(companion_encoded_url("url.https%3A%2F%2Fexample.com|INVOKE").empty());
  assert(companion_encoded_url("url." + std::string(129, 'a')).empty());

  companion_set_actions({{"com.apple.Safari", "Safari"}, {"media.play_pause", "Media Play/Pause"}});
  companion_set_connected(true);
  assert(companion_connected());
  assert(!companion_action_available("media.play_pause"));
  CompanionNowPlayingSnapshot now_playing;
  now_playing.generation = 1;
  now_playing.playback_state = CompanionPlaybackState::PAUSED;
  companion_set_now_playing(now_playing);
  assert(companion_action_available("media.play_pause"));
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
  bool media_invoked = false;
  register_companion_action_sender([&media_invoked](const std::string &action,
                                                     const std::string &request) {
    media_invoked = action == "media.play_pause" && request == "test-media";
    return media_invoked;
  });
  assert(invoke_companion_action("media.play_pause", "test-media"));
  assert(media_invoked);
  now_playing.playback_state = CompanionPlaybackState::UNAVAILABLE;
  companion_set_now_playing(now_playing);
  assert(!companion_action_available("media.play_pause"));
  assert(!invoke_companion_action("media.play_pause", "test-media-unavailable"));
  companion_set_connected(false);
  assert(!companion_action_available("media.play_pause"));
  return 0;
}
