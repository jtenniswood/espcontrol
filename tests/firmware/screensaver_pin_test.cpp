#include <algorithm>
#include <cassert>
#include <thread>
#include "nvs.h"
#include "screensaver_pin.h"
#include "pin_keypad_order.h"
#include "display_mode_controller.h"

void invalid_records_can_be_replaced() {
  struct Record { uint32_t version{2}; char pin[17]{"0042"}; uint8_t enabled{1}; uint8_t randomize{0}; };
  static_assert(sizeof(Record) == 24);
  auto bytes = [](const Record &record) {
    const auto *first = reinterpret_cast<const uint8_t *>(&record);
    return std::vector<uint8_t>(first, first + sizeof(record));
  };
  std::vector<std::vector<uint8_t>> invalid;
  Record record;
  record.version = 99; invalid.push_back(bytes(record));
  record = {}; record.enabled = 2; invalid.push_back(bytes(record));
  record = {}; record.randomize = 2; invalid.push_back(bytes(record));
  record = {}; record.pin[1] = 'x'; invalid.push_back(bytes(record));
  record = {}; std::memset(record.pin, '1', sizeof(record.pin)); invalid.push_back(bytes(record));
  record = {};
  auto truncated = bytes(record); truncated.pop_back(); invalid.push_back(truncated);
  auto oversized = bytes(record); oversized.push_back(0); invalid.push_back(oversized);
  for (const auto &blob : invalid) {
    identity_flash = blob;
    espcontrol::ScreensaverPin recovered;
    recovered.setup();
    assert(recovered.ready() && !recovered.has_pin() && !recovered.enabled() && !recovered.locked());
    assert(!recovered.randomize_keypad());
    std::string replacement = "9876";
    assert(recovered.save(true, &replacement));
    espcontrol::ScreensaverPin rebooted;
    rebooted.setup();
    assert(rebooted.ready() && rebooted.locked() && rebooted.submit(replacement));
  }
  read_error = ESP_ERR_NVS_TYPE_MISMATCH;
  espcontrol::ScreensaverPin wrong_type;
  wrong_type.setup();
  assert(wrong_type.ready() && !wrong_type.has_pin());
  read_error = 2; // Genuine NVS read failure keeps writes unavailable.
  espcontrol::ScreensaverPin read_failed;
  read_failed.setup();
  std::string code = "0042";
  assert(!read_failed.ready() && !read_failed.save(true, &code));
  read_error = ESP_OK;
  fail_open = true;
  espcontrol::ScreensaverPin open_failed;
  open_failed.setup();
  assert(!open_failed.ready() && !open_failed.save(true, &code));
  fail_open = false;
  identity_flash.clear();
}

int main() {
  invalid_records_can_be_replaced();
  using espcontrol::ScreensaverPin;
  ScreensaverPin pin;
  pin.setup();
  assert(pin.ready() && !pin.has_pin() && pin.pin_length() == 0 && !pin.locked());
  std::string code = "0042";
  assert(pin.save(true, &code) && pin.pin_length() == 4);
  pin.arm(); pin.wake();
  assert(pin.locked());
  pin.dismiss_keypad();
  pin.wake();
  assert(pin.locked() && !pin.keypad_requested());
  pin.request_keypad();
  assert(pin.keypad_requested());
  assert(!pin.submit("42") && pin.locked());
  assert(!pin.submit("") && pin.locked());
  assert(pin.submit("0042") && !pin.locked());
  // Manual Screen Lock uses the same PIN even when automatic wake locking is off.
  assert(pin.save(false, nullptr));
  assert(pin.lock(true) && pin.locked() && !pin.keypad_requested());
  pin.request_keypad();
  assert(pin.keypad_requested());
  pin.arm(); pin.wake();
  assert(pin.locked() && !pin.submit("wrong"));
  assert(pin.submit(code) && !pin.locked());
  assert(pin.lock(true) && !pin.keypad_requested());
  pin.arm(espcontrol::DisplayMode::CLOCK); pin.wake();
  assert(pin.locked() && pin.keypad_requested());
  assert(pin.return_mode() == espcontrol::DisplayMode::CLOCK);
  assert(pin.submit(code));
  assert(pin.return_mode() == espcontrol::DisplayMode::ACTIVE);
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
