#include "media_input_state.h"

#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <new>
#include <string>

#include "cover_art.h"

static size_t allocations = 0;
void* operator new(size_t bytes) {
  ++allocations;
  if (void* p = std::malloc(bytes ? bytes : 1)) return p;
  std::abort();
}
void* operator new[](size_t bytes) {
  return ::operator new(bytes);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, std::size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept {
  std::free(p);
}
using namespace espcontrol::media;
void legacy_classification_parity() {
  const std::array<std::string, 16> pieces = {"TV",
                                              "Line-in",
                                              "line in",
                                              "line_in",
                                              "HDMI 1",
                                              " Spotify ",
                                              "X-Rincon-Stream:",
                                              "x_sonos_htastream:",
                                              ":SPDIF",
                                              "http://",
                                              "  ",
                                              "\t",
                                              "unknown",
                                              "unavailable",
                                              "a",
                                              ""};
  uint32_t random = 1729;
  for (size_t sample = 0; sample < 10000; ++sample) {
    std::string value = pieces[sample % pieces.size()];
    for (size_t part = 0; part < sample % 4; ++part) {
      random = random * 1664525u + 1013904223u;
      value += pieces[random % pieces.size()];
    }
    const std::string expected_content = media_content_id_external_source(value);
    const std::string expected_source = media_source_external_label(value);
    const bool expected_external = espcontrol::cover_art::external_media_source(value);
    const auto before = allocations;
    assert(std::strcmp(input_label_text(content_input_label(value)), expected_content.c_str()) == 0);
    assert(std::strcmp(input_label_text(source_input_label(value)), expected_source.c_str()) == 0);
    assert(external_input_source(value) == expected_external);
    assert(allocations == before);
  }
}
void snapshot_order() {
  MediaInputState input;
  assert(input.source("HDMI 1", {}));
  assert(input.external && input.effective_source("HDMI 1") == "HDMI 1");
  assert(!input.content("x-sonos-htastream:device:spdif"));
  assert(input.effective_source("HDMI 1") == "HDMI 1");
  assert(input.source("Spotify", "HDMI 1"));
  assert(input.external && input.effective_source("Spotify") == "TV");
  assert(input.source("HDMI 1", "Spotify"));
  assert(input.external && input.effective_source("HDMI 1") == "HDMI 1");
  input.content("spotify:track:1");
  assert(!input.external && !input.known);
  assert(!input.source("Spotify", "HDMI 1"));
  assert(input.content("x-rincon-stream:device"));
  assert(input.effective_source("Spotify") == "Line-in");
  input.state_observed();
  input.current_content(false);
  assert(input.external);
  input.current_content(true);
  assert(!input.external && !input.known);
  input.content("x-sonos-htastream:device:spdif");
  input.content("");
  input.source("Spotify", "Spotify");
  assert(!input.external);  // Empty ID is not an observation for this snapshot.
  input.content("x-rincon-stream:device");
  input.source("line_in", "Spotify");
  assert(!input.external);  // Preserve the two existing source predicates.
}
int main() {
  legacy_classification_parity();
  snapshot_order();
}
