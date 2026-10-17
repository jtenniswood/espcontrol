#pragma once
#include "weather_daily_forecast.h"
namespace espcontrol::weather {
struct TextBox {
  int x{0}, y{0}, width{0}, height{0};
};
struct DayBoxes {
  TextBox icon, day, temperature;
};
struct CardFrameLayout {
  CardLayout layout;
  std::array<DayBoxes, MAX_DAYS> days{};
};
// Shared by the existing widget renderer and the fixed forecast drawing view.
inline CardFrameLayout card_frame_layout(int width, int height, int font_height, int temperature_height,
                                         int icon_height, int minimum_day_width, int list_text_width,
                                         size_t available_days, bool stale) {
  CardFrameLayout result;
  const int caption_height = stale ? font_height + 2 : 0,
            content_height = std::max(1, height - caption_height);
  const int day_gap = std::max(8, font_height), row_gap = std::max(8, font_height / 2),
            list_gap = std::max(8, font_height / 3);
  const int minimum_height = icon_height + font_height + temperature_height + 4;
  auto& layout = result.layout;
  layout = card_layout(
      width, content_height, minimum_day_width, minimum_height, icon_height + list_gap + list_text_width,
      std::max(icon_height, font_height + temperature_height + 2), day_gap, row_gap, available_days);
  if (layout.count == 0) return result;
  const bool grid = !layout.list && layout.rows > 1;
  const int icon_gap = grid ? std::min(std::max(6, font_height / 3),
                                       (content_height - row_gap * static_cast<int>(layout.rows - 1)) /
                                               static_cast<int>(layout.rows) -
                                           icon_height - font_height - temperature_height - 2)
                            : 2;
  const int group_height = icon_height + icon_gap + font_height + temperature_height + 2;
  const int grid_row_gap = grid ? std::min(std::max(row_gap, font_height),
                                           (content_height - group_height * static_cast<int>(layout.rows)) /
                                               static_cast<int>(layout.rows - 1))
                                : row_gap;
  for (size_t i = 0; i < layout.count; ++i) {
    const int column = static_cast<int>(i % layout.columns), row = static_cast<int>(i / layout.columns);
    const int usable_width = width - day_gap * static_cast<int>(layout.columns - 1),
              usable_height = content_height - grid_row_gap * static_cast<int>(layout.rows - 1);
    const int base_width = usable_width / static_cast<int>(layout.columns),
              remainder_width = usable_width % layout.columns;
    const int base_height = usable_height / static_cast<int>(layout.rows),
              remainder_height = usable_height % layout.rows;
    const int cell_width = base_width + (column < remainder_width ? 1 : 0),
              cell_height = base_height + (row < remainder_height ? 1 : 0);
    const int x = column * (base_width + day_gap) + std::min(column, remainder_width),
              y = row * (base_height + grid_row_gap) + std::min(row, remainder_height);
    const int icon_y = y + (layout.list ? (cell_height - icon_height) / 2
                            : grid      ? (cell_height - group_height) / 2
                                        : 0);
    const int text_x = x + (layout.list ? icon_height + list_gap : 0), text_width = cell_width - (text_x - x);
    const int text_top =
        layout.list ? y + (cell_height - font_height - temperature_height - 2) / 2
        : grid      ? icon_y + icon_height + icon_gap
                    : y + std::max(icon_height + 2, cell_height - temperature_height - font_height - 2);
    result.days[i] = {{x, icon_y, layout.list ? icon_height : cell_width, icon_height},
                      {text_x, text_top, text_width, font_height},
                      {text_x, text_top + font_height + 2, text_width, temperature_height}};
  }
  return result;
}
}  // namespace espcontrol::weather
