#include "../../components/espcontrol/network_status_layout.h"

#include <cassert>

int main() {
  assert(NETWORK_STATUS_BACK_CARD_INDEX == 0);
  assert(NETWORK_STATUS_IP_CARD_INDEX == 1);
  assert(NETWORK_STATUS_WIFI_CARD_INDEX == 2);
  assert(NETWORK_STATUS_BUILD_CARD_INDEX == 3);
  assert(NETWORK_STATUS_CARD_COUNT == 4);

  const auto square_last = network_status_grid_cell(3, 3);
  assert(square_last.column == 0);
  assert(square_last.row == 1);

  for (int index = 0; index < NETWORK_STATUS_CARD_COUNT; ++index) {
    const auto landscape = network_status_grid_cell(index, 5);
    assert(landscape.column == index);
    assert(landscape.row == 0);
  }

  const auto portrait_third = network_status_grid_cell(2, 2);
  assert(portrait_third.column == 0);
  assert(portrait_third.row == 1);

  assert(network_status_grid_rows(2, 3, NETWORK_STATUS_CARD_COUNT) == 3);
  assert(network_status_grid_rows(3, 2, NETWORK_STATUS_CARD_COUNT) == 2);

  const auto fallback = network_status_grid_cell(1, 0);
  assert(fallback.column == 0);
  assert(fallback.row == 1);
}
