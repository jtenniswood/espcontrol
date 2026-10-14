#pragma once

#include <algorithm>
#include <vector>

namespace espcontrol {
struct WallpaperBackground {
  lv_obj_t *root;
  bool modal;
};
inline std::vector<WallpaperBackground> &wallpaper_backgrounds() {
  static std::vector<WallpaperBackground> backgrounds;
  return backgrounds;
}
inline auto &wallpaper_background_refresh() {
  static void (*refresh)(lv_obj_t *, bool) = nullptr;
  return refresh;
}
inline void wallpaper_register_background(lv_obj_t *root, bool modal = false) {
  if (!root) return;
  auto &backgrounds = wallpaper_backgrounds();
  for (const auto &entry : backgrounds) if (entry.root == root) return;
  backgrounds.push_back({root, modal});
  lv_obj_add_event_cb(root, [](lv_event_t *event) {
    auto *root = static_cast<lv_obj_t *>(lv_event_get_target(event));
    auto &backgrounds = wallpaper_backgrounds();
    backgrounds.erase(std::remove_if(backgrounds.begin(), backgrounds.end(),
        [root](const WallpaperBackground &entry) { return entry.root == root; }), backgrounds.end());
  }, LV_EVENT_DELETE, nullptr);
  // Late-created subpages and modal panels must work without reloading the image.
  auto refresh = [](lv_event_t *event) {
    auto *root = static_cast<lv_obj_t *>(lv_event_get_target(event));
    for (const auto &entry : wallpaper_backgrounds()) {
      if (entry.root == root && wallpaper_background_refresh()) {
        wallpaper_background_refresh()(root, entry.modal);
        break;
      }
    }
  };
  lv_obj_add_event_cb(root, refresh, LV_EVENT_SIZE_CHANGED, nullptr);
  lv_obj_add_event_cb(root, refresh, LV_EVENT_SCREEN_LOAD_START, nullptr);
  if (wallpaper_background_refresh()) wallpaper_background_refresh()(root, modal);
}
}  // namespace espcontrol
