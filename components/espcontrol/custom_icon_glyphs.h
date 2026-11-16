#pragma once

// Icons compiled into font_icon_main beyond the built-in set.
//
// People who compile their own firmware can add Material Design Icons to the
// icon font. Codegen (components/espcontrol/custom_icons.py) passes their
// codepoints to set_custom_icon_codepoints() once during setup, before any
// card is built or web request served. Each glyph's UTF-8 text is prepared
// then, so lookups only read this table: no locks, no runtime allocation and
// no limit on how many custom icons a panel uses. Official prebuilt firmware
// has none.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

#include "config_field_decode.h"

// Custom icons use the supplementary Private Use Areas, where Material Design
// Icons live. The bundled font has nothing in the older U+E000-F8FF area.
constexpr uint32_t CUSTOM_ICON_MIN_CODEPOINT = 0xF0000;
constexpr uint32_t CUSTOM_ICON_MAX_CODEPOINT = 0x10FFFD;

struct CustomIconGlyph {
  uint32_t codepoint;
  char utf8[5];
};

inline std::vector<CustomIconGlyph> &custom_icon_glyph_table() {
  static std::vector<CustomIconGlyph> glyphs;
  return glyphs;
}

// Sorted by codepoint, without duplicates.
inline const std::vector<CustomIconGlyph> &custom_icon_glyphs() {
  return custom_icon_glyph_table();
}

// Call once during setup. Replacing the list later would invalidate glyph
// pointers that cards already hold.
inline void set_custom_icon_codepoints(std::initializer_list<uint32_t> codepoints) {
  std::vector<uint32_t> sorted;
  for (uint32_t codepoint : codepoints) {
    if (codepoint >= CUSTOM_ICON_MIN_CODEPOINT && codepoint <= CUSTOM_ICON_MAX_CODEPOINT)
      sorted.push_back(codepoint);
  }
  std::sort(sorted.begin(), sorted.end());
  sorted.erase(std::unique(sorted.begin(), sorted.end()), sorted.end());
  std::vector<CustomIconGlyph> &glyphs = custom_icon_glyph_table();
  glyphs.clear();
  glyphs.reserve(sorted.size());
  for (uint32_t codepoint : sorted) {
    CustomIconGlyph glyph{codepoint, {}};
    glyph.utf8[encode_utf8_codepoint(codepoint, glyph.utf8)] = '\0';
    glyphs.push_back(glyph);
  }
}

// The glyph text for a compiled-in custom icon, or nullptr if the font does
// not contain that codepoint.
inline const char *find_custom_icon_glyph(uint32_t codepoint) {
  const std::vector<CustomIconGlyph> &glyphs = custom_icon_glyphs();
  auto it = std::lower_bound(glyphs.begin(), glyphs.end(), codepoint,
                             [](const CustomIconGlyph &glyph, uint32_t value) {
                               return glyph.codepoint < value;
                             });
  return it != glyphs.end() && it->codepoint == codepoint ? it->utf8 : nullptr;
}
