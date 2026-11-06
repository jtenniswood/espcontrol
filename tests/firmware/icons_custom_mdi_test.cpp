#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "icons.h"

int main() {
  const char* cog = "\U000F0493";

  // Official firmware has no custom icons, so a well-formed codepoint the
  // font lacks shows no icon rather than the cog.
  assert(custom_icon_glyphs().empty());
  assert(std::strcmp(find_icon("mdi:F0EFA"), "") == 0);

  // Built-in icons can be referenced by codepoint, case-insensitively, and
  // resolve to the same glyph as their name.
  assert(find_icon("mdi:F0335") == find_icon("Lightbulb"));
  assert(find_icon("mdi:f0335") == find_icon("Lightbulb"));
  assert(std::strcmp(find_icon("mdi:F0493"), cog) == 0);
  assert(find_icon("mdi:F0F92") == find_icon("Water Boiler"));

  // Firmware compiled with extra icons gets each one, with duplicates and
  // out-of-range values dropped, sorted for lookup.
  set_custom_icon_codepoints({0xF1234, 0xF0EFA, 0x41, 0xF0EFA, 0x10FFFD, 0x110000});
  assert(custom_icon_glyphs().size() == 3);
  assert(custom_icon_glyphs()[0].codepoint == 0xF0EFA);
  assert(custom_icon_glyphs()[2].codepoint == 0x10FFFD);
  assert(std::strcmp(find_icon("mdi:F0EFA"), "\U000F0EFA") == 0);
  assert(std::strcmp(find_icon("mdi:f1234"), "\U000F1234") == 0);
  assert(std::strcmp(find_icon("mdi:10FFFD"), "\U0010FFFD") == 0);
  // The same pointer comes back every time, so cards can keep it.
  const char* first = find_icon("mdi:F0EFA");
  assert(find_icon("mdi:F0EFA") == first);
  // A codepoint not compiled into the font shows no icon.
  assert(std::strcmp(find_icon("mdi:F1D00"), "") == 0);

  // There is no limit on how many custom icons a panel can use: every
  // compiled-in icon keeps resolving, however many there are. (The setter
  // takes an initializer list, so fill the table directly here.)
  std::vector<CustomIconGlyph>& table = custom_icon_glyph_table();
  table.clear();
  for (uint32_t codepoint = 0xF2000; codepoint < 0xF2000 + 500; codepoint++) {
    CustomIconGlyph glyph{codepoint, {}};
    glyph.utf8[encode_utf8_codepoint(codepoint, glyph.utf8)] = '\0';
    table.push_back(glyph);
  }
  char name[16];
  for (uint32_t codepoint = 0xF2000; codepoint < 0xF2000 + 500; codepoint++) {
    std::snprintf(name, sizeof(name), "mdi:%X", static_cast<unsigned>(codepoint));
    char expected[5] = {};
    encode_utf8_codepoint(codepoint, expected);
    assert(std::strcmp(find_icon(name), expected) == 0);
  }

  // Malformed or non-icon values fall back to the cog.
  assert(std::strcmp(find_icon("mdi:"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:XYZ12"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F0F92:home-flood"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:0041"), cog) == 0);
  // The older U+E000-F8FF area has no MDI glyphs, so it is rejected too.
  assert(std::strcmp(find_icon("mdi:E000"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F8FF"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F0EF"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:EFFFF"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:1F03350"), cog) == 0);
  assert(std::strcmp(find_icon("Not An Icon"), cog) == 0);
  assert(std::strcmp(find_icon(nullptr), cog) == 0);
}
