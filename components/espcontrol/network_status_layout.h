#pragma once

#include <algorithm>

struct NetworkStatusGridCell {
  int column = 0;
  int row = 0;
};

constexpr int NETWORK_STATUS_BACK_CARD_INDEX = 0;
constexpr int NETWORK_STATUS_IP_CARD_INDEX = 1;
constexpr int NETWORK_STATUS_BUILD_CARD_INDEX = 2;
constexpr int NETWORK_STATUS_BACKLIGHT_CARD_INDEX = 3;

inline NetworkStatusGridCell network_status_grid_cell(int card_index,
                                                       int columns) {
  const int safe_columns = std::max(1, columns);
  return {card_index % safe_columns, card_index / safe_columns};
}
