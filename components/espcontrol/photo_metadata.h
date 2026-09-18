#pragma once

#include <algorithm>
#include <string>

namespace espcontrol {

// HA states are short, but bound retained text and preserve whole UTF-8 glyphs.
inline std::string photo_metadata_text(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return {};
  value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
  std::string status = value;
  for (char &ch : status) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
  if (status == "unknown" || status == "unavailable" || status == "none") return {};
  constexpr size_t limit = 255;
  if (value.size() > limit) {
    size_t end = limit;
    while (end > 0 && (static_cast<unsigned char>(value[end]) & 0xc0) == 0x80) --end;
    value.resize(end);
  }
  return value;
}

struct PhotoOverlayLayout {
  int margin;
  int bottom;
  int clock_y;
  int metadata_x;
  int metadata_y;
  int metadata_width;
  int metadata_bottom;
};

inline PhotoOverlayLayout photo_overlay_layout(int width, int height, int clock_width,
                                               int clock_height, int metadata_height,
                                               bool wide_panel, bool clock_visible) {
  const int margin = std::clamp(width / 32, 12, 40);
  const int bottom = margin - 10;  // Match the image clock's existing lower position.
  // The compact layouts need more breathing room for the metadata than the
  // clock's deliberately low position allows. Keep the wide-panel treatment
  // unchanged, while giving stacked metadata a 24px edge inset and a tighter
  // gap to the clock above it.
  const int metadata_inset = wide_panel ? margin : std::max(margin, 24);
  const int metadata_bottom = wide_panel ? bottom : std::max(bottom, 24);
  const int gap = wide_panel ? 8 : 4;
  const int metadata_x = wide_panel && clock_visible
      ? margin + clock_width + gap : metadata_inset;
  const int metadata_width = std::max(1, width - metadata_inset - metadata_x);
  const int metadata_y = height - metadata_bottom - metadata_height;
  const int clock_y = !wide_panel && metadata_height > 0
      ? metadata_y - gap - clock_height
      : height - bottom - clock_height;
  return {margin, bottom, clock_y, metadata_x, metadata_y, metadata_width,
          metadata_bottom};
}

}  // namespace espcontrol
