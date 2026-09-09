#include "../../components/espcontrol/network_status_layout.h"

#include <cassert>

int main() {
  const auto square_last = network_status_grid_cell(3, 3);
  assert(square_last.column == 0);
  assert(square_last.row == 1);

  for (int index = 0; index < 4; ++index) {
    const auto landscape = network_status_grid_cell(index, 5);
    assert(landscape.column == index);
    assert(landscape.row == 0);
  }

  const auto portrait_third = network_status_grid_cell(2, 2);
  assert(portrait_third.column == 0);
  assert(portrait_third.row == 1);

  const auto fallback = network_status_grid_cell(1, 0);
  assert(fallback.column == 0);
  assert(fallback.row == 1);
}
