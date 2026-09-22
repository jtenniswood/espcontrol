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
                                               bool clock_visible) {
  const bool wide_panel = width >= 1024;
  const int margin = std::clamp(width / 32, 12, 40);
  const int bottom = margin - 10;  // Match the image clock's existing lower position.
  // Compact layouts keep metadata in the upper-right corner, while wide
  // panels keep their side-by-side placement and align the metadata's bottom
  // edge with the clock's bottom edge.
  const int metadata_inset = wide_panel ? margin : std::max(margin, 24);
  const int metadata_bottom = wide_panel ? bottom : std::max(bottom, 24);
  const int gap = wide_panel ? 8 : 0;
  constexpr int wide_metadata_left_shift = 20;
  constexpr int wide_metadata_up_shift = 30;
  const int metadata_x = wide_panel && clock_visible
      ? margin + clock_width + gap - wide_metadata_left_shift : metadata_inset;
  const int metadata_width = std::max(1, width - metadata_inset - metadata_x);
  const int clock_y = height - bottom - clock_height;
  const int metadata_y = !wide_panel
      ? metadata_inset
      : wide_panel && clock_visible
          ? clock_y + clock_height - metadata_height - wide_metadata_up_shift
          : height - metadata_bottom - metadata_height;
  return {margin, bottom, clock_y, metadata_x, metadata_y, metadata_width,
          metadata_bottom};
}

}  // namespace espcontrol
