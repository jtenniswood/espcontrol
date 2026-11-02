#include "gpio_relays.h"
#include "nvs.h"
#include "driver/gpio.h"
#include "esphome/core/hal.h"
#include <cassert>
#include <cstring>
#include <iostream>
using namespace espcontrol;
int main() {
  GpioRelays unsupported;
  unsupported.setup("esp32-p4-86");
  assert(!unsupported.supported() && !unsupported.save({}));
  GpioRelays service;
  service.setup("guition-esp32-p4-jc1060p470");
  auto notify = [](size_t, bool) {};
  service.loop(0, notify);
  assert(service.ready() && gpio_calls.empty()); // Opt-in: no pins touched on a fresh device.
  GpioRelaySettings settings;
  settings.enabled = true;
  assert(!service.save(settings)); // A pin is required.
  settings.controls[0].pin = 1;
  settings.controls[1].pin = 1;
  assert(!service.save(settings)); // Duplicate pins are never accepted.
  settings.controls[1].pin = 4;
  assert(!service.save(settings)); // No display/network/other arbitrary pins.
  settings.controls[1].pin = 2;
  settings.controls[1].inverted = true;
  assert(service.save(settings));
  service.set(0, true);
  assert(!service.state(0)); // Nothing energizes before the loop applies a change.
  service.loop(0, notify);
  assert(levels[1] == 0 && levels[2] == 1 && !service.state(0) && !service.state(1));
  assert(gpio_calls[0].first == 'L' && gpio_calls[1].first == 'D'); // Inactive latch first.
  service.set(0, true); service.set(1, true);
  assert(levels[1] == 1 && levels[2] == 0 && service.state(0) && service.state(1));
  service.set(1, false);
  assert(levels[2] == 1 && !service.state(1));
  esphome::fake_now = UINT32_MAX - 99;
  service.set(1, true, true);
  service.loop(99, notify); assert(service.state(1)); // 199 ms, including clock wrap.
  service.loop(100, notify); assert(!service.state(1) && levels[2] == 1);
  fail_commit = true;
  auto disabled = settings; disabled.enabled = false;
  assert(!service.save(disabled));
  service.loop(101, notify); assert(service.state(0)); // Failed writes preserve active settings.
  fail_commit = false;
  assert(service.save(disabled));
  service.loop(102, notify);
  assert(!service.state(0) && levels[1] == 0 && directions[1] == 0 && directions[2] == 0);
  service.set(0, true); assert(!service.state(0));
  assert(service.save(settings)); service.loop(103, notify); service.set(0, true);
  GpioRelays rebooted;
  rebooted.setup("guition-esp32-p4-jc1060p470-v2"); rebooted.loop(0, notify);
  assert(!rebooted.state(0) && levels[1] == 0); // Persist configuration, never energized state.
  gpio_failure = true;
  assert(rebooted.save(settings)); rebooted.loop(0, notify); rebooted.set(0, true);
  assert(!rebooted.state(0));
  gpio_failure = false;
  service.shutdown(); assert(levels[1] == 0);
  std::cout << "GPIO relay runtime tests passed\n";
}
