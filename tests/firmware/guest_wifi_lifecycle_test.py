"""Exercise the production modal lifecycle/action functions with fake HA and LVGL."""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / "components/espcontrol/button_grid_wifi_qr.h").read_text()
names = ("wifi_qr_hide_modal", "wifi_qr_guest_configuration_current",
         "wifi_qr_guest_tick", "wifi_qr_toggle_guest")
functions = []
for name in names:
    start = source.index("inline ", source.rfind("\ninline ", 0, source.index(name + "(")))
    end = source.find("\ninline ", start + 1)
    functions.append(source[start:end])

harness = r'''
#include <cassert>
#include <functional>
#include <string>
#include "guest_wifi_state.h"
namespace esphome { using StringRef = std::string; }
struct WifiQrModalUi {
  void *overlay = nullptr;
  void *guest_group = nullptr;
  void *guest_timer = nullptr;
  uint32_t guest_generation = 0;
  bool guest_subscribed = false;
  std::string guest_entity;
  GuestWifiState guest_state;
};
WifiQrModalUi ui;
WifiQrModalUi &wifi_qr_modal_ui() { return ui; }
enum class ControlModalKind { WIFI_QR };
uint32_t generation = 1;
bool connected = true;
int actions = 0, subscriptions = 0, releases = 0, timers_deleted = 0, overlays_deleted = 0;
std::function<void(esphome::StringRef)> state_callback;
uint32_t ha_subscription_generation() { return generation; }
bool ha_api_state_connected() { return connected; }
uint32_t lv_tick_get() { return 0; }
void lv_timer_del(void *) { ++timers_deleted; }
void ha_release_callbacks_for_owner(void *) { ++releases; state_callback = {}; }
void control_modal_delete_overlay(ControlModalKind, void *) { ++overlays_deleted; }
void wifi_qr_apply_guest_state() {}
struct HaCallbackOwnerScope { explicit HaCallbackOwnerScope(void *) {} };
bool ha_subscribe_state(const std::string &, std::function<void(esphome::StringRef)> callback) {
  ++subscriptions; state_callback = std::move(callback); return true;
}
bool ha_send_entity_action(const std::string &entity, const char *service) {
  assert(entity == "switch.guest_wifi");
  assert(std::string(service) == "switch.turn_on");
  ++actions; return true;
}
'''
harness += "\n".join(functions)
harness += r'''
void open_modal() {
  ui = WifiQrModalUi();
  ui.overlay = ui.guest_group = ui.guest_timer = &ui;
  ui.guest_entity = "switch.guest_wifi";
  ui.guest_generation = generation;
}
int main() {
  open_modal();
  wifi_qr_guest_tick();
  assert(subscriptions == 1 && actions == 0);
  state_callback("off");
  wifi_qr_toggle_guest();
  wifi_qr_toggle_guest();
  assert(actions == 1); // Await confirmation rather than sending a second command.
  state_callback("on");
  assert(ui.guest_state.on && !ui.guest_state.pending);
  ++generation; // Card edited/deleted while its modal is still open.
  wifi_qr_toggle_guest(); // A tap before the refresh timer must also be rejected.
  assert(actions == 1 && !ui.overlay && !state_callback);
  assert(timers_deleted == 1 && overlays_deleted == 1 && releases == 1);

  open_modal();
  ++generation;
  wifi_qr_guest_tick();
  assert(!ui.overlay && subscriptions == 1); // Never resubscribe to the old entity.
  assert(timers_deleted == 2 && overlays_deleted == 2);

  open_modal();
  wifi_qr_guest_tick();
  state_callback("off");
  connected = false;
  wifi_qr_guest_tick();
  wifi_qr_toggle_guest();
  assert(actions == 1 && !ui.guest_state.known && !state_callback);
  connected = true;
  wifi_qr_guest_tick();
  state_callback("off");
  assert(ui.guest_state.known && !ui.guest_state.on);
  wifi_qr_hide_modal();
  assert(!ui.overlay && !state_callback);
}
'''
with tempfile.TemporaryDirectory(prefix="guest-wifi-lifecycle-") as directory:
    cpp = Path(directory) / "test.cpp"
    binary = Path(directory) / "test"
    cpp.write_text(harness)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(root / "components/espcontrol"), str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Guest Wi-Fi modal lifecycle checks passed.")
