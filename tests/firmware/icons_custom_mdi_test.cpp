#include <cassert>
#include <array>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

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
  // The older U+E000-F8FF area has no MDI glyphs, so it is rejected too.
  assert(std::strcmp(find_icon("mdi:E000"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F8FF"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F0EF"), cog) == 0);
  assert(std::strcmp(find_icon("mdi:F0000"), "\U000F0000") == 0);
  assert(std::strcmp(find_icon("mdi:10FFFD"), "\U0010FFFD") == 0);
  assert(std::strcmp(find_icon("mdi:1F03350"), cog) == 0);
  assert(std::strcmp(find_icon("Not An Icon"), cog) == 0);
  assert(std::strcmp(find_icon(nullptr), cog) == 0);
  // Lookups from several tasks at once agree on one pointer per codepoint
  // and always see a fully written glyph, including while slots are added.
  constexpr size_t kThreads = 8;
  constexpr size_t kCodepoints = 16;
  std::array<std::array<const char*, kCodepoints>, kThreads> seen{};
  std::vector<std::thread> workers;
  for (size_t t = 0; t < kThreads; t++) {
    workers.emplace_back([t, &seen]() {
      char lookup[16];
      for (size_t round = 0; round < 2000; round++) {
        // Each thread walks the codepoints in a different order.
        const size_t i = (round * (t + 1) + t) % kCodepoints;
        std::snprintf(lookup, sizeof(lookup), "mdi:F40%02zX", i);
        const char* glyph = find_icon(lookup);
        char expected[5] = {};
        encode_utf8_codepoint(0xF4000 + static_cast<uint32_t>(i), expected);
        assert(std::strcmp(glyph, expected) == 0);
        if (seen[t][i] == nullptr) seen[t][i] = glyph;
        assert(seen[t][i] == glyph);
      }
    });
  }
  for (std::thread& worker : workers) worker.join();
  for (size_t i = 0; i < kCodepoints; i++) {
    for (size_t t = 1; t < kThreads; t++) {
      if (seen[t][i] != nullptr && seen[0][i] != nullptr) assert(seen[t][i] == seen[0][i]);
    }
  }
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
