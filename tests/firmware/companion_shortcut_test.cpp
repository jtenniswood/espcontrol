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

  companion_set_actions({{"com.apple.Safari", "Safari"}});
  companion_set_connected(true);
  assert(companion_connected());
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
  bool invoked = false;
  register_companion_url_sender([&invoked](const std::string &app, const std::string &url,
                                           const std::string &request) {
    invoked = app == "com.apple.Safari" && url.rfind("https%3A%2F%2F", 0) == 0 && request == "test-1";
    return invoked;
  });
  assert(invoke_companion_url("com.apple.Safari", url_config, "test-1"));
  assert(invoked);
  return 0;
}
