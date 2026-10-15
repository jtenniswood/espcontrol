#include <cassert>
#include "clock_bar_layout.h"
using namespace espcontrol::clockbar;
int main() {
  const std::vector<MeasuredItem> input{{0, "left", 100}, {1, "middle", 60}, {2, "right", 40}};
  auto full = place(input, 400, 8);
  assert(full.items.size() == 3);
  assert(full.items[1].x == 200);
  auto crowded = place(input, 150, 8);
  assert(crowded.items.size() == 2);
  assert(crowded.items[0].index == 0 && crowded.items[1].index == 2);
  auto title = place(input, 120, 8, 110);
  assert(title.items.size() == 1 && title.items[0].index == 2);
  assert(title.title_width == 72);
  std::vector<MeasuredItem> many;
  for (size_t i = 0; i < 12; i++) many.push_back({i, i % 3 == 0 ? "left" : i % 3 == 1 ? "middle" : "right", 24 + static_cast<int>(i) * 9});
  for (int width : {0, 20, 80, 240, 468, 788, 1012, 1188}) {
    for (int title_width : {0, 40, 300, 1500}) {
      auto layout = place(many, width, 6, title_width);
      std::sort(layout.items.begin(), layout.items.end(), [](const Placement &a, const Placement &b) { return a.x < b.x; });
      int end = layout.title_width;
      for (const auto &item : layout.items) {
        assert(item.x >= end && item.x + item.width <= width);
        end = item.x + item.width;
      }
    }
  }
  assert(sensor_type("temperature") && sensor_type("humidity") && sensor_type("co2"));
  assert(!sensor_type("network") && !valid_type("arbitrary"));
}
