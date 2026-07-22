#pragma once

#include <cstddef>

#include "button_grid_limits.h"

// Phase 2 binds the main grid and every configured subpage card. Keep the
// broker fixed-capacity, but size it for that complete supported card graph.
constexpr size_t HA_CONFIGURED_CARD_CAPACITY =
    static_cast<size_t>(MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS);
constexpr size_t HA_STATE_CHANNEL_CAPACITY =
    HA_CONFIGURED_CARD_CAPACITY * 8 + 32;
constexpr size_t HA_SCOPED_LEASE_CAPACITY =
    HA_CONFIGURED_CARD_CAPACITY * 12 + 64;
constexpr size_t HA_STATE_SUBSCRIBER_CAPACITY =
    HA_SCOPED_LEASE_CAPACITY * 2 + 16;
