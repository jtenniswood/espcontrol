#include <algorithm>
#include <cassert>
#include <thread>
#include "nvs.h"
#include "screensaver_pin.h"
#include "pin_keypad_order.h"
#include "display_mode_controller.h"

int main() {
  using espcontrol::ScreensaverPin;
  ScreensaverPin pin;
  pin.setup();
  assert(pin.ready() && !pin.has_pin() && pin.pin_length() == 0 && !pin.locked());
  std::string code = "0042";
  assert(pin.save(true, &code) && pin.pin_length() == 4);
  pin.arm(); pin.wake();
  assert(pin.locked());
  assert(!pin.submit("42") && pin.locked());
  assert(!pin.submit("") && pin.locked());
  assert(pin.submit("0042") && !pin.locked());
  // Manual Screen Lock uses the same PIN even when automatic wake locking is off.
  assert(pin.save(false, nullptr));
  assert(pin.lock() && pin.locked());
  pin.arm(); pin.wake();
  assert(pin.locked() && !pin.submit("wrong"));
  assert(pin.submit(code) && !pin.locked());
  // A screensaver unlock also releases a preceding manual card lock.
  assert(pin.save(true, nullptr) && pin.lock());
  pin.arm(); pin.wake();
  assert(pin.submit(code) && !pin.locked());
  pin.arm(); pin.wake();
  assert(pin.locked());
  // A toggle preserves the existing secret; reboot restores both settings.
  assert(pin.save(false, nullptr) && pin.has_pin() && !pin.locked());
  // Upgrade the old on-device record without losing the PIN; records without a saved preference use fixed order.
  struct LegacyPinRecord { uint32_t version{1}; char pin[17]{}; uint8_t enabled{0}; } legacy;
  std::memcpy(legacy.pin, code.data(), code.size());
  const auto *legacy_bytes = reinterpret_cast<const uint8_t *>(&legacy);
  identity_flash.assign(legacy_bytes, legacy_bytes + sizeof(legacy));
  ScreensaverPin rebooted;
  rebooted.setup();
  rebooted.arm(); rebooted.wake();
  assert(rebooted.has_pin() && rebooted.pin_length() == 4 && !rebooted.enabled() && !rebooted.locked());
  assert(!rebooted.randomize_keypad());
  bool randomize = false;
  assert(rebooted.save(true, nullptr, &randomize));
  ScreensaverPin layout_reboot;
  layout_reboot.setup();
  assert(layout_reboot.enabled() && layout_reboot.pin_length() == 4 && !layout_reboot.randomize_keypad());
  randomize = true;
  fail_commit = true;
  assert(!rebooted.save(true, nullptr, &randomize) && !rebooted.randomize_keypad());
  fail_commit = false;
  rebooted.arm(); rebooted.wake();
  assert(rebooted.locked());
  std::string changed = "1234567890123456";
  fail_write = true;
  assert(!rebooted.save(true, &changed));
  fail_write = false;
  fail_commit = true;
  assert(!rebooted.save(true, &changed));
  fail_commit = false;
  assert(!rebooted.submit(changed));
  assert(rebooted.submit(code));
  assert(rebooted.save(true, &changed) && rebooted.pin_length() == 16);
  rebooted.arm(); rebooted.wake();
  assert(!rebooted.submit(code) && rebooted.locked());
  assert(rebooted.submit(changed));
  std::string invalid = "12x4";
  assert(!rebooted.save(true, &invalid));
  invalid = std::string(17, '1');
  assert(!rebooted.save(true, &invalid));
  std::string empty;
  rebooted.arm(); rebooted.wake();
  assert(rebooted.save(true, &empty));
  assert(!rebooted.has_pin() && rebooted.pin_length() == 0 && !rebooted.enabled() && !rebooted.locked());
  assert(!rebooted.lock() && !rebooted.locked());
  ScreensaverPin empty_reboot;
  empty_reboot.setup();
  assert(!empty_reboot.enabled() && !empty_reboot.has_pin());
  rebooted.arm(); rebooted.wake();
  assert(!rebooted.locked());
  // The same clear operation is used by backup import.
  assert(rebooted.save(false, &empty));
  ScreensaverPin cleared;
  cleared.setup();
  assert(!cleared.has_pin() && cleared.pin_length() == 0 && !cleared.enabled() && !cleared.locked());

  // Enabling while asleep is effective on the very next wake, even before
  // the periodic display callback runs. Enabling after an unprotected wake
  // takes effect on the next sleep instead.
  assert(cleared.save(false, &empty));
  cleared.arm();
  assert(cleared.save(true, &code) && cleared.locked());
  cleared.wake();
  assert(cleared.locked() && cleared.submit(code));
  assert(cleared.save(false, nullptr));
  cleared.arm(); cleared.wake();
  assert(cleared.save(true, nullptr) && !cleared.locked());
  assert(cleared.lock());
  assert(cleared.save(false, nullptr) && cleared.locked());
  assert(cleared.save(false, &empty) && !cleared.locked());

  // Web saves and touchscreen entry share the production object's mutex.
  std::thread web([&]() {
    for (int i = 0; i < 1000; ++i) assert(cleared.save(i % 2, &code));
  });
  std::thread touch([&]() {
    for (int i = 0; i < 1000; ++i) {
      cleared.arm(); cleared.wake();
      cleared.submit("wrong");
      (void) cleared.locked();
    }
  });
  web.join();
  touch.join();
  assert(cleared.save(true, &code));
  cleared.arm(); cleared.wake();
  assert(!cleared.submit("wrong") && cleared.locked());
  assert(cleared.submit(code));

  uint32_t seed = 7;
  auto random = [&]() { seed = seed * 1664525u + 1013904223u; return seed; };
  auto previous = scrambled_pin_digits(random);
  bool different = false;
  for (int round = 0; round < 100; ++round) {
    auto digits = scrambled_pin_digits(random);
    different |= digits != previous;
    auto sorted = digits;
    std::sort(sorted.begin(), sorted.end());
    for (int i = 0; i < 10; ++i) assert(sorted[i] == i);
  }
  assert(different);
  int random_calls = 0;
  const auto ordered = pin_keypad_digits(false, [&]() { ++random_calls; return random(); });
  const std::array<uint8_t, 10> expected{{1, 2, 3, 4, 5, 6, 7, 8, 9, 0}};
  assert(ordered == expected && random_calls == 0);
  const auto shuffled = pin_keypad_digits(true, [&]() { ++random_calls; return random(); });
  assert(random_calls == 9);
  auto sorted_shuffled = shuffled;
  std::sort(sorted_shuffled.begin(), sorted_shuffled.end());
  for (int i = 0; i < 10; ++i) assert(sorted_shuffled[i] == i);

  using namespace espcontrol;
  for (auto mode : {DisplayMode::DISPLAY_OFF, DisplayMode::DIMMED,
                    DisplayMode::CLOCK, DisplayMode::CAMERA}) {
    DisplayModeController display;
    display.request(DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART);
    assert(display.request(DisplayRequestSource::MANUAL_SLEEP, mode));
    assert(display.target_mode_is(mode));
    assert(!presence_can_wake_display(display.resolve()));
    assert(!display.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
    assert(display.clear(DisplayRequestSource::MANUAL_SLEEP));
    display.clear(DisplayRequestSource::MEDIA_PLAYBACK);
    assert(display.target_mode_is(DisplayMode::ACTIVE));
  }
}
