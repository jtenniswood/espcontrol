#include <cassert>
#include <cstdint>
#include <string>
#include "guest_wifi_state.h"

int main() {
  assert(guest_wifi_valid_entity("switch.guest_wifi"));
  for (const auto *entity : {"", "switch.", "light.guest", "switch.guest;light.other", "switch.Guest"})
    assert(!guest_wifi_valid_entity(entity));
  GuestWifiState state;
  assert(!state.begin(0));
  state.receive("off");
  assert(state.begin(10));
  assert(!state.on && state.target_on && state.pending);
  assert(!state.begin(11));
  state.receive("off"); // An old state must not acknowledge the command.
  assert(state.pending);
  state.receive("on");
  assert(state.on && !state.pending && !state.failed);
  assert(state.begin(20));
  state.tick(10019);
  assert(state.pending);
  state.tick(10020);
  assert(state.failed && !state.pending && state.on);
  assert(state.begin(10021));
  state.send_failed();
  assert(!state.pending && state.failed && state.on);
  state.disconnect();
  assert(!state.known && !state.begin(10022));
  state.receive("unavailable");
  assert(!state.known);
  state.receive("unknown");
  assert(!state.known);
  state.receive("off");
  assert(state.known && !state.on);
  // Timeout arithmetic must survive the LVGL millisecond counter wrapping.
  assert(state.begin(UINT32_MAX - 5000));
  state.tick(4999);
  assert(state.failed && !state.pending);
  state.receive("on");
  assert(state.on && !state.failed);
}
