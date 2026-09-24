#include "photo_metadata.h"
#include <cassert>
#include <string>

int main() {
  using namespace espcontrol;
  for (const auto *value : {"", " \n\t", "unknown", " UNKNOWN ", "Unavailable", "none"})
    assert(photo_metadata_text(value).empty());
  assert(photo_metadata_text("  Paris\nJune 2026  ") == "Paris\nJune 2026");
  assert(photo_metadata_text("Unknown artist") == "Unknown artist");
  assert(photo_metadata_text(std::string(300, 'x')).size() == 255);
  assert(photo_metadata_text(std::string(254, 'x') + "é").size() == 254);
  assert(photo_metadata_text(std::string(253, 'x') + "é!").size() == 255);
  for (const auto width : {480, 600, 720, 800}) {
    const auto empty = photo_overlay_layout(width, 800, 280, 120, 0, true);
    const auto shown = photo_overlay_layout(width, 800, 280, 120, 60, true);
    assert(empty.clock_y == shown.clock_y);
    assert(shown.metadata_x == std::max(shown.margin, 24));
    assert(shown.metadata_y == std::max(shown.margin, 24));
    assert(shown.metadata_x + shown.metadata_width == width - shown.metadata_x);
    assert(shown.metadata_bottom == std::max(shown.bottom, 24));
  }
  for (const auto width : {1024, 1280}) {
    const auto empty = photo_overlay_layout(width, 800, 440, 170, 0, true);
    const auto shown = photo_overlay_layout(width, 800, 440, 170, 60, true);
    assert(empty.clock_y == shown.clock_y);
    assert(shown.metadata_x == shown.margin + 440 + 8 - 20);
    assert(shown.metadata_x + shown.metadata_width == width - shown.margin);
    assert(shown.metadata_y + 60 == shown.clock_y + 170 - 30);
    const auto alone = photo_overlay_layout(width, 800, 440, 170, 60, false);
    assert(alone.metadata_width == width - 2 * alone.margin);
    assert(alone.metadata_y + 60 == 800 - alone.metadata_bottom);
    assert(shown.metadata_bottom == shown.bottom);
  }
}
