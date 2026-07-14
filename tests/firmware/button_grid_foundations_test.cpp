#include <cstdlib>
#include <string>

#include "button_grid_limits.h"
#include "button_grid_string.h"

int main() {
  static_assert(MAX_GRID_SLOTS == ESPCONTROL_MAX_GRID_SLOTS);
  static_assert(MAX_SUBPAGE_ITEMS == MAX_GRID_SLOTS * MAX_GRID_SLOTS);

  if (string_ref_limited(esphome::StringRef("calendar"), 4) != "cale") return EXIT_FAILURE;
  if (string_ref_limited(esphome::StringRef("clock"), 32) != "clock") return EXIT_FAILURE;

  const char embedded_null[] = {'a', '\0', 'b'};
  const std::string copied = string_ref_limited(esphome::StringRef(embedded_null, 3), 3);
  if (copied.size() != 3 || copied[0] != 'a' || copied[1] != '\0' || copied[2] != 'b') {
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
