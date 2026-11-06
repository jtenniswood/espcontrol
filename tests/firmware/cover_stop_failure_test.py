"""Stop failures/cancellation must never issue a movement command."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/espcontrol/button_grid_actions.h").read_text()


def function(name):
    matches = re.findall(rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}", source, re.M | re.S)
    assert matches, name
    return matches[0] + "\n"


code = r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>
#include <string>
#include <vector>
#define ESP_LOGW(...) ((void)0)
struct ParsedCfg { std::string entity, sensor, unit; };
namespace esphome::api {
struct HomeassistantActionRequest { std::string service; uint32_t call_id = 0; };
struct ActionResponse {
  bool success; std::string error;
  bool is_success() const { return success; }
  const std::string &get_error_message() const { return error; }
};
}
using Callback = std::function<void(const esphome::api::ActionResponse &)>;
std::map<uint32_t, Callback> callbacks;
std::vector<std::string> actions;
uint32_t generation = 1;
bool accept_send = true, accept_callback = true;
uint32_t ha_subscription_generation() { return generation; }
const char *cover_command_service(const std::string &) { return "cover.stop_cover"; }
const char *cover_tilt_command_service(const std::string &) { return "cover.stop_cover_tilt"; }
bool ha_action_begin(esphome::api::HomeassistantActionRequest &request, const char *service,
                     bool, size_t, uint32_t call_id = 0) {
  request.service = service; request.call_id = call_id; return true;
}
void ha_action_add_entity(esphome::api::HomeassistantActionRequest &, const std::string &) {}
void ha_action_add_data(esphome::api::HomeassistantActionRequest &, const char *, const char *) {}
bool ha_action_send(esphome::api::HomeassistantActionRequest &request) {
  if (!accept_send) return false;
  actions.push_back(request.service); return true;
}
void send_toggle_action(const std::string &) { actions.push_back("homeassistant.toggle"); }
bool ha_register_action_response_callback(uint32_t call_id, Callback callback) {
  if (!accept_callback) return false;
  callbacks.emplace(call_id, std::move(callback)); return true;
}
bool ha_cancel_action_response_callback(uint32_t call_id, const char *error) {
  auto it = callbacks.find(call_id); if (it == callbacks.end()) return false;
  auto callback = std::move(it->second); callbacks.erase(it);
  callback({false, error}); return true;
}
'''
for name in ("cover_position_value", "next_cover_stop_call_id", "cover_stop_pending_call_id",
             "cover_stop_clear_pending", "cover_stop_track_pending", "cover_stop_cancel_pending_request",
             "send_cover_command_action"):
    code += function(name)
code += r'''
int main() {
  // Cover, Gate and the Cover modal use this shared sender.
    for (const char *error : {"unsupported service", "permission denied", "timeout", "api disconnected"}) {
      actions.clear(); send_cover_command_action({"cover.door", "stop", ""});
      assert(actions.size() == 1 && actions.front() == "cover.stop_cover");
      const uint32_t call_id = cover_stop_pending_call_id(); assert(call_id != 0);
      ha_cancel_action_response_callback(call_id, error);
      assert(actions.size() == 1 && cover_stop_pending_call_id() == 0 && callbacks.empty());
    }
  actions.clear(); send_cover_command_action({"cover.door", "stop", ""});
  auto call_id = cover_stop_pending_call_id(); auto callback = std::move(callbacks.at(call_id));
  callbacks.erase(call_id); callback({true, ""}); assert(actions.size() == 1 && !cover_stop_pending_call_id());
  actions.clear(); send_cover_command_action({"cover.door", "stop", ""});
  ++generation; ha_cancel_action_response_callback(cover_stop_pending_call_id(), "late failure");
  assert(actions.size() == 1 && !cover_stop_pending_call_id());
  send_cover_command_action({"cover.door", "stop", ""}); cover_stop_cancel_pending_request();
  assert(actions.size() == 2 && callbacks.empty());
  actions.clear(); accept_send = false; send_cover_command_action({"cover.door", "stop", ""});
  assert(actions.empty() && callbacks.empty() && !cover_stop_pending_call_id());
  accept_send = true; accept_callback = false; send_cover_command_action({"cover.door", "stop", ""});
  assert(actions.size() == 1 && callbacks.empty() && !cover_stop_pending_call_id());
  actions.clear(); send_cover_command_action({"cover.door", "stop", ""}, true);
  assert(actions.size() == 1 && actions[0] == "cover.stop_cover_tilt" && callbacks.empty());
  puts("Cover/Gate Stop failures, cancellation, late responses and tilt passed.");
}
'''
with tempfile.TemporaryDirectory(prefix="cover-stop-failure-") as temporary:
    path = Path(temporary) / "test.cpp"
    binary = Path(temporary) / "test"
    path.write_text(code)
    compiler = shlex.split(os.environ.get("CXX", sys.argv[1] if len(sys.argv) > 1 else "c++"))
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
