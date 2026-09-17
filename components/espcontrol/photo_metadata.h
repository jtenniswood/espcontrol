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
};

inline PhotoOverlayLayout photo_overlay_layout(int width, int height, int clock_width,
                                               int clock_height, int metadata_height,
                                               bool wide_panel, bool clock_visible) {
  const int margin = std::clamp(width / 32, 12, 40);
  const int bottom = margin - 10;  // Match the image clock's existing lower position.
  const int gap = 8;
  const int metadata_x = wide_panel && clock_visible ? margin + clock_width + gap : margin;
  const int metadata_width = std::max(1, width - margin - metadata_x);
  const int lift = !wide_panel && metadata_height > 0 ? metadata_height + gap : 0;
  return {margin, bottom, height - bottom - clock_height - lift,
          metadata_x, height - bottom - metadata_height, metadata_width};
}

}  // namespace espcontrol
