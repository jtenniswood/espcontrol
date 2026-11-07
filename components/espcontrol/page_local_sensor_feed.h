#pragma once
#include <string_view>

namespace espcontrol::pages {
// Installed only after legacy view bindings have retired. Producers retain this
// firmware-lifetime facade, never a page, widget or configuration pointer.
struct PageLocalSensorFeed {
  void* owner{nullptr};
  bool (*number)(void*, std::string_view, float){nullptr};
  bool (*text)(void*, std::string_view, std::string_view){nullptr};
  bool apply_number(std::string_view key, float value) const {
    const auto feed = *this;
    return feed.owner && feed.number && feed.number(feed.owner, key, value);
  }
  bool apply_text(std::string_view key, std::string_view value) const {
    const auto feed = *this;
    return feed.owner && feed.text && feed.text(feed.owner, key, value);
  }
};
inline PageLocalSensorFeed& page_local_sensor_feed() {
  static PageLocalSensorFeed feed;
  return feed;
}
}  // namespace espcontrol::pages
