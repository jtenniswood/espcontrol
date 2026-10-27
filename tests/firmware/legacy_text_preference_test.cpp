#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "legacy_text_preference.h"

namespace {
struct Store {
  std::vector<uint8_t> durable, pending;
  size_t peak_pending{0}, saves{0}, flushes{0};
  bool fail_save{false}, fail_flush{false};
  bool load(uint8_t *data, size_t size) {
    if (durable.size() != size)
      return false;
    std::memcpy(data, durable.data(), size);
    return true;
  }
  bool save(const uint8_t *data, size_t size) {
    ++saves;
    if (fail_save)
      return false;
    pending.assign(data, data + size);
    peak_pending = std::max(peak_pending, pending.size());
    return true;
  }
  bool sync() {
    ++flushes;
    // ESPHome clears even unsuccessful queued writes. A retry must compare
    // durable data and queue the same value again, without a stale value cache.
    if (fail_flush) {
      fail_flush = false;
      pending.clear();
      return false;
    }
    if (!pending.empty())
      durable.swap(pending);
    pending.clear();
    return true;
  }
};
bool write(Store &store, const std::string &value, size_t maximum) {
  return espcontrol::configuration::persist_legacy_text(
      store, [&store] { return store.sync(); }, value.data(), value.size(),
      maximum);
}
void compatible_payloads_and_bounded_queue() {
  Store store;
  for (const size_t maximum : {6u, 120u, 255u}) {
    const std::string value(maximum, 'x');
    assert(write(store, value, maximum));
    assert(store.durable.size() == maximum + 1 && store.durable[0] == maximum);
    assert(std::memcmp(store.durable.data() + 1, value.data(), maximum) == 0);
    const auto saves = store.saves;
    assert(write(store, value, maximum) && store.saves == saves);
    assert(write(store, "", maximum));
    assert(store.durable[0] == 0);
    assert(std::all_of(store.durable.begin() + 1, store.durable.end(),
                       [](uint8_t value) { return value == 0; }));
  }
  // A full 20-slot profile has 180 button/subpage text fields plus chrome.
  // Every one completes persistence before the next can queue another blob.
  for (unsigned field = 0; field < 182; ++field) {
    assert(write(store, std::string(250, static_cast<char>('a' + field % 26)),
                 255));
    assert(store.pending.empty() && store.peak_pending <= 256);
  }
  const auto saves = store.saves, flushes = store.flushes;
  assert(!write(store, std::string(256, 'x'), 255));
  assert(!write(store, "x", 256));
  assert(!espcontrol::configuration::persist_legacy_text(
      store, [&store] { return store.sync(); }, nullptr, 1, 255));
  assert(store.saves == saves && store.flushes == flushes);
}
void failures_are_reported_and_identical_values_retry() {
  Store store;
  assert(write(store, "old", 255));
  const auto previous = store.durable;
  store.fail_save = true;
  assert(!write(store, "new", 255) && store.durable == previous);
  store.fail_save = false;
  // Fail after the payload is queued, rather than the initial queue drain.
  unsigned flushes = 0;
  assert(!espcontrol::configuration::persist_legacy_text(
      store,
      [&] {
        if (++flushes == 2)
          store.fail_flush = true;
        return store.sync();
      },
      "new", 3, 255));
  assert(store.pending.empty() && store.durable == previous);
  const auto attempts = store.saves;
  assert(write(store, "new", 255) && store.saves == attempts + 1);
  assert(store.durable[0] == 3 &&
         std::memcmp(store.durable.data() + 1, "new", 3) == 0);
  store.fail_flush = true;
  const auto saves = store.saves;
  assert(!write(store, "later", 255) && store.saves == saves);
}
} // namespace
int main() {
  compatible_payloads_and_bounded_queue();
  failures_are_reported_and_identical_values_retry();
}
