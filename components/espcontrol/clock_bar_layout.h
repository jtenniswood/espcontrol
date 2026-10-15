#pragma once
#include <algorithm>
#include <string>
#include <vector>

namespace espcontrol::clockbar {
constexpr size_t MAX_ITEMS = 12;
constexpr size_t MAX_BYTES = 4096;
struct Element {
  std::string id, type, group, source, entity, label, unit;
  int precision = 0;
  std::string icon;
};
inline bool sensor_type(const std::string &type) {
  for (const char *name : {"temperature", "humidity", "battery", "power", "energy", "illuminance", "pressure", "co2"})
    if (type == name) return true;
  return false;
}
inline bool valid_type(const std::string &type) {
  return sensor_type(type) || type == "time" || type == "network" || type == "night" || type == "device_battery";
}
struct MeasuredItem { size_t index; std::string group; int width; };
struct Placement { size_t index; int x, width; };
struct Layout { std::vector<Placement> items; int title_width = 0; };
inline Layout place(const std::vector<MeasuredItem> &input, int width, int gap, int title_width = 0) {
  std::vector<MeasuredItem> left, middle, right;
  for (const auto &item : input) {
    if (item.group == "left" && !title_width) left.push_back(item);
    else if (item.group == "middle") middle.push_back(item);
    else if (item.group == "right") right.push_back(item);
  }
  auto size = [gap](const std::vector<MeasuredItem> &items) {
    int result = items.empty() ? 0 : (static_cast<int>(items.size()) - 1) * gap;
    for (const auto &item : items) result += item.width;
    return result;
  };
  auto required = [&]() {
    int total = 0, count = 0;
    for (int w : {title_width ? title_width : size(left), size(middle), size(right)}) {
      if (w > 0) { total += w; count++; }
    }
    return total + std::max(0, count - 1) * gap;
  };
  while (required() > width && !middle.empty()) middle.pop_back();
  while (required() > width && !left.empty()) left.pop_back();
  while ((size(right) > width || (title_width > 0 && size(right) + gap + std::min(title_width, 48) > width)) && !right.empty()) right.erase(right.begin());
  Layout result;
  const int rw = size(right);
  result.title_width = title_width ? std::max(0, std::min(title_width, width - rw - (rw ? gap : 0))) : 0;
  const int lw = result.title_width ? result.title_width : size(left);
  auto append = [&](const std::vector<MeasuredItem> &items, int x) {
    for (const auto &item : items) { result.items.push_back({item.index, x, item.width}); x += item.width + gap; }
  };
  append(left, 0);
  const int start = lw ? lw + gap : 0;
  const int end = width - rw - (rw ? gap : 0);
  append(middle, start + std::max(0, end - start - size(middle)) / 2);
  append(right, width - rw);
  return result;
}
}  // namespace espcontrol::clockbar
