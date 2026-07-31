#include <cstdlib>

namespace {

constexpr int canonical_slot(int requested, int slot_count) {
  return requested >= 0 && requested < slot_count ? requested : -1;
}

}  // namespace

int main() {
  if (canonical_slot(0, 4) != 0 || canonical_slot(3, 4) != 3) return EXIT_FAILURE;
  if (canonical_slot(-1, 4) != -1 || canonical_slot(4, 4) != -1) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
