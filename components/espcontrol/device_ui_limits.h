#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace espcontrol {

struct DeviceCardTypeSupport {
  const char *type;
  bool allow_in_subpage;
};

// Configuration capacity is independent of the currently visible orientation.
// Inactive cards (for example slots 19/20 on the 18-slot V3 portrait grid)
// must survive rotation, save, restore and future runtime migrations.
struct DeviceUiLimits {
  const char *device_profile;
  uint8_t grid_columns;
  uint8_t grid_rows;
  uint8_t slot_count;
  uint8_t max_first_level_subpages;
  uint8_t cards_per_subpage;  // Excludes the reserved Back slot.
  uint8_t max_navigation_depth;  // Root is depth zero.
  uint8_t image_slot_count;
  uint8_t legacy_subpage_chunks;
  uint16_t legacy_text_bytes;
  const DeviceCardTypeSupport *card_types;
  size_t card_type_count;

  constexpr size_t max_child_cards() const {
    return max_first_level_subpages * cards_per_subpage;
  }

  constexpr size_t max_cards() const { return slot_count + max_child_cards(); }

  // This is a byte ceiling, not a guarantee that every UTF-8 string fits:
  // a character cannot be split across legacy text chunks.
  constexpr size_t legacy_subpage_byte_ceiling() const {
    return legacy_subpage_chunks * legacy_text_bytes;
  }

  bool supports_card_type(const char *type, bool in_subpage = false) const {
    if (type == nullptr) return false;
    for (size_t i = 0; i < card_type_count; ++i) {
      if (std::strcmp(type, card_types[i].type) == 0)
        return !in_subpage || card_types[i].allow_in_subpage;
    }
    return false;
  }
};

}  // namespace espcontrol

#include "device_ui_limits_generated.h"
