#include <cassert>
#include <cstdio>
#include <cstring>

#include "icons.h"

int main() {
  // A custom codepoint resolves to the same glyph as the built-in name.
  assert(std::strcmp(find_icon("mdi:F0335"), find_icon("Lightbulb")) == 0);
  assert(std::strcmp(find_icon("mdi:f0335"), find_icon("Lightbulb")) == 0);
  // Icons outside the built-in table still encode as 4-byte UTF-8.
  assert(std::strcmp(find_icon("mdi:F0F92"), "\U000F0F92") == 0);
  // Pointers stay valid across later lookups.
  const char* first = find_icon("mdi:F0F92");
  find_icon("mdi:F1234");
  assert(std::strcmp(first, "\U000F0F92") == 0);
  // Malformed or non-icon values fall back to the cog.
  const char* cog = "\U000F0493";
  assert(std::strcmp(find_icon("mdi:"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:XYZ1"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F0F92:home-flood"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:0041"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:1F03350"), cog) == 0);
  assert(std::strcmp(find_icon("Not An Icon"), cog) == 0);
  // Once the fixed cache is full, new codepoints fall back to the cog while
  // already-cached ones keep resolving to their original pointers.
  char name[16];
  for (size_t i = 0; i < CUSTOM_MDI_ICON_CACHE_SLOTS; i++) {
    std::snprintf(name, sizeof(name), "mdi:F2%03zX", i);
    find_icon(name);
  }
  assert(std::strcmp(find_icon("mdi:F3000"), cog) == 0);
  assert(find_icon("mdi:F0F92") == first);
  assert(std::strcmp(first, "\U000F0F92") == 0);
}
