#include "page_image_tile_policy.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
using namespace espcontrol::pages;
static size_t allocations = 0;
void* operator new(size_t n) {
  ++allocations;
  if (auto* p = std::malloc(n ? n : 1)) return p;
  std::abort();
}
void* operator new[](size_t n) {
  return ::operator new(n);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, size_t) noexcept {
  std::free(p);
}
int main() {
  const std::string values[] = {"",
                                "on",
                                "off",
                                "unknown",
                                "unavailable",
                                "None",
                                "ON",
                                " on ",
                                "2026-10-18T12:34:56+00:00",
                                std::string(80, 'a'),
                                std::string(80, 'a') + "suffix",
                                std::string(81, 'b'),
                                std::string("on\0x", 4)};
  for (const auto* trigger : {"binary_sensor.motion", "binary_sensor.", "event.doorbell", "event.",
                              "camera.front", "event.UPPER", " event.x", "event.a-b", "event.a_b1"})
    assert(page_image_trigger_valid(trigger) == espcontrol::camera::valid_trigger(trigger));
  for (bool binary : {false, true}) {
    espcontrol::camera::ActivityTrigger reference;
    PageImageActivity bounded;
    uint32_t epoch = UINT32_MAX, random = 1234567;
    for (size_t n = 0; n < 200000; ++n) {
      random = random * 1664525u + 1013904223u;
      if ((random >> 20) % 9 == 0) ++epoch;
      const auto& raw = values[random % std::size(values)];
      const auto value = raw.substr(0, 80);
      const auto before = allocations;
      const bool changed = bounded.observe(raw, binary, epoch);
      assert(allocations == before);
      assert(changed == reference.observe(value, binary, epoch) && bounded.previous() == reference.previous &&
             bounded.baselined() == reference.baselined && bounded.connection() == reference.connection &&
             bounded.on(epoch) == reference.on(epoch));
    }
  }
  espcontrol::camera::ImageRevision reference;
  PageImageRevision bounded;
  reference.latest = bounded.latest = UINT32_MAX - 2;
  uint32_t random = 7654321;
  for (size_t n = 0; n < 200000; ++n) {
    random = random * 1664525u + 1013904223u;
    const auto& raw = values[random % std::size(values)];
    const auto value = raw.substr(0, 80);
    if ((random >> 16) % 7 == 0) {
      reference.invalidate();
      bounded.invalidate();
    }
    const auto before = allocations;
    const bool changed = bounded.observe(raw);
    assert(allocations == before);
    assert(changed == reference.observe(value));
    switch ((random >> 20) % 4) {
      case 0:
        reference.tile_requested = reference.latest;
        bounded.tile_requested = bounded.latest;
        break;
      case 1:
        reference.tile_applied = reference.tile_requested;
        bounded.tile_applied = bounded.tile_requested;
        break;
      case 2:
        reference.modal_requested = reference.latest;
        bounded.modal_requested = bounded.latest;
        break;
      case 3:
        reference.modal_applied = reference.modal_requested;
        bounded.modal_applied = bounded.modal_requested;
        break;
    }
    assert(reference.latest == bounded.latest && bounded.timestamp() == reference.timestamp &&
           bounded.tile_dirty() == reference.tile_dirty() &&
           bounded.modal_dirty() == reference.modal_dirty());
  }
  std::puts(
      "image tile policy: 600000 legacy activity/revision comparisons, bounded bytes and allocation-free "
      "events passed");
}
