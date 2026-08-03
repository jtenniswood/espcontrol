#include <cstdlib>

#include "nvs_configuration_capacity.h"

namespace {

using Policy = espcontrol::configuration::NvsConfigurationCapacity;

bool capacity_tracks_the_guaranteed_two_slot_budget() {
  if (Policy::slot_capacity_for_entry_budget(
          Policy::RESERVED_SHARED_ENTRIES) != 0) {
    return false;
  }

  const size_t one_chunk_budget =
      Policy::RESERVED_SHARED_ENTRIES +
      Policy::entries_for_capacity(Policy::CHUNK_SIZE);
  if (Policy::slot_capacity_for_entry_budget(one_chunk_budget) !=
      Policy::CHUNK_SIZE) {
    return false;
  }

  const size_t maximum_budget =
      Policy::RESERVED_SHARED_ENTRIES +
      Policy::entries_for_capacity(Policy::MAX_SLOT_CAPACITY);
  if (Policy::slot_capacity_for_entry_budget(maximum_budget) !=
      Policy::MAX_SLOT_CAPACITY ||
      Policy::slot_capacity_for_entry_budget(maximum_budget + 10000) !=
          Policy::MAX_SLOT_CAPACITY) {
    return false;
  }

  const size_t partial_budget =
      Policy::RESERVED_SHARED_ENTRIES +
      Policy::entries_for_capacity(7 * Policy::CHUNK_SIZE) - 1;
  return Policy::slot_capacity_for_entry_budget(partial_budget) ==
         6 * Policy::CHUNK_SIZE;
}

}  // namespace

int main() {
  return capacity_tracks_the_guaranteed_two_slot_budget() ? EXIT_SUCCESS
                                                           : EXIT_FAILURE;
}
