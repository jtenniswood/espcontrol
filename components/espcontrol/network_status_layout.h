#pragma once

#include <algorithm>

struct NetworkStatusGridCell {
  int column = 0;
  int row = 0;
};

inline NetworkStatusGridCell network_status_grid_cell(int card_index,
                                                       int columns) {
  const int safe_columns = std::max(1, columns);
  return {card_index % safe_columns, card_index / safe_columns};
}
