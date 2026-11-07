#pragma once

#include <cstdint>

#include "page_image_tile_policy.h"

namespace espcontrol::pages {
// Fits in the card definition's existing option union. The decoded trigger is
// stored separately in its shared decoded-option view during preparation.
struct BasicImageOptions {
  uint8_t interval_seconds{10};
  uint8_t refresh{0};
  bool fit{false}, label{false}, icon{false};
};
}  // namespace espcontrol::pages
