#include <cstddef>
#include <iostream>

#include "panel_config_storage_selection.h"

namespace {

struct FakeStorage {
  bool partition_result{false};
  bool nvs_result{false};
  int partition_calls{0};
  int nvs_calls{0};
  size_t requested_capacity{0};

  bool begin_card_images_partition(size_t capacity) {
    ++partition_calls;
    requested_capacity = capacity;
    return partition_result;
  }

  bool begin() {
    ++nvs_calls;
    return nvs_result;
  }
};

bool expect(bool condition, const char *message) {
  if (condition) return true;
  std::cerr << message << '\n';
  return false;
}

}  // namespace

int main() {
  constexpr size_t kSlotCapacity = 40 * 1024;

  {
    FakeStorage storage{false, true};
    bool fallback = true;
    if (!expect(espcontrol::configuration::begin_panel_config_storage(
                    storage, false, kSlotCapacity, &fallback),
                "NVS-only storage should start") ||
        !expect(storage.partition_calls == 0,
                "NVS-only storage must not probe the card partition") ||
        !expect(storage.nvs_calls == 1,
                "NVS-only storage should open NVS once") ||
        !expect(!fallback, "NVS-only storage is not a fallback"))
      return 1;
  }

  {
    FakeStorage storage{true, true};
    bool fallback = true;
    if (!expect(espcontrol::configuration::begin_panel_config_storage(
                    storage, true, kSlotCapacity, &fallback),
                "Available card partition should start") ||
        !expect(storage.partition_calls == 1,
                "Preferred card partition should be probed once") ||
        !expect(storage.requested_capacity == kSlotCapacity,
                "Card partition should receive the configured slot capacity") ||
        !expect(storage.nvs_calls == 0,
                "NVS must not open when the card partition is available") ||
        !expect(!fallback, "Available card partition should not use fallback"))
      return 1;
  }

  {
    FakeStorage storage{false, true};
    bool fallback = false;
    if (!expect(espcontrol::configuration::begin_panel_config_storage(
                    storage, true, kSlotCapacity, &fallback),
                "Missing card partition should fall back to NVS") ||
        !expect(storage.partition_calls == 1,
                "Missing card partition should be probed once") ||
        !expect(storage.nvs_calls == 1,
                "Missing card partition should open NVS once") ||
        !expect(fallback, "Missing card partition should report NVS fallback"))
      return 1;
  }

  {
    FakeStorage storage{false, false};
    bool fallback = false;
    if (!expect(!espcontrol::configuration::begin_panel_config_storage(
                    storage, true, kSlotCapacity, &fallback),
                "Storage should fail when both backends are unavailable") ||
        !expect(fallback, "Failed NVS fallback should still be reported"))
      return 1;
  }

  return 0;
}
