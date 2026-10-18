#pragma once

#include <array>
#include <cstdint>
#include <string_view>

#include "wifi_qr_layout.h"

namespace espcontrol::pages {
// Codec payloads are capped at 255 bytes. MEDIUM needs at most version 12;
// pinned LVGL's quiet-zone size selection may choose two versions above that.
constexpr size_t PAGE_QR_PAYLOAD_MAX = 255;
constexpr uint8_t PAGE_QR_VERSION_MAX = 14;
constexpr uint8_t PAGE_QR_MODULES_MAX = PAGE_QR_VERSION_MAX * 4 + 17;
constexpr size_t PAGE_QR_BITMAP_BYTES = 1 + (PAGE_QR_MODULES_MAX * PAGE_QR_MODULES_MAX + 7) / 8;
struct PageQrLayout {
  uint16_t side{0}, scale{0}, margin{0};
  uint8_t version{0}, modules{0};
  bool valid() const {
    const uint32_t span = static_cast<uint32_t>(modules) * scale;
    return version >= 1 && version <= PAGE_QR_VERSION_MAX && modules == version * 4 + 17 && side && scale &&
           span <= side && margin == (side - span) / 2 && margin >= 2u * scale;
  }
};
inline PageQrLayout page_qr_layout(int minimum, int side, bool compact) {
  if (minimum < 1 || minimum > PAGE_QR_VERSION_MAX || side <= 0 || side > UINT16_MAX) return {};
  int version = minimum, scale = 0;
  if (compact)
    scale = wifi_qr_compact_scale(side, minimum * 4 + 17);
  else {
    // Exact pinned LVGL quiet-zone version/rounding rule, including strict
    // tie handling. The surrounding white card supplies additional separation.
    int offset = side;
    for (int candidate = minimum; candidate <= minimum + 2; ++candidate) {
      if (candidate > PAGE_QR_VERSION_MAX) return {};
      const int denominator = (candidate + 1) * 4 + 17;
      const int next_offset = side % denominator;
      if (next_offset < offset) {
        offset = next_offset;
        version = candidate;
        scale = side / denominator;
      }
    }
  }
  if (scale <= 0) return {};
  const int modules = version * 4 + 17;
  return {static_cast<uint16_t>(side), static_cast<uint16_t>(scale),
          static_cast<uint16_t>((side - modules * scale) / 2), static_cast<uint8_t>(version),
          static_cast<uint8_t>(modules)};
}
// Fixed packed module bitmap, not a full-size canvas. Byte zero is reserved
// for the pinned encoder's dimension, then row-major modules are LSB first.
// Only overwrite after old descriptor/pixel readers have passed a draw fence.
struct PageQrMatrix {
  PageQrLayout layout;
  std::array<uint8_t, PAGE_QR_BITMAP_BYTES> bytes{};
  size_t size() const {
    return layout.valid() ? 1 + (layout.modules * layout.modules + 7) / 8 : 0;
  }
  void clear() {
    layout = {};
    bytes.fill(0);
  }
  bool dark(unsigned x, unsigned y) const {
    if (x >= layout.modules || y >= layout.modules) return false;
    const size_t bit = y * layout.modules + x;
    return bytes[1 + bit / 8] & (1u << (bit % 8));
  }
  void dark(unsigned x, unsigned y, bool value) {
    if (x >= layout.modules || y >= layout.modules) return;
    const size_t bit = y * layout.modules + x;
    auto& byte = bytes[1 + bit / 8];
    const auto mask = 1u << (bit % 8);
    if (value)
      byte |= mask;
    else
      byte &= ~mask;
  }
};
inline std::string_view page_qr_compact_prefix(std::string_view payload) {
  constexpr std::string_view suffix = "H:false;;";
  return payload.size() >= suffix.size() && payload.substr(payload.size() - suffix.size()) == suffix
             ? payload.substr(0, payload.size() - suffix.size())
             : payload;
}
class PageQrEncoder {
 public:
  virtual ~PageQrEncoder() = default;
  // Safe maintenance only, after a draw fence for this output. Input/output
  // views last through this call; implementation does not retain those views.
  virtual bool encode(std::string_view payload, int side, bool compact, PageQrMatrix& output) = 0;
};
}  // namespace espcontrol::pages
