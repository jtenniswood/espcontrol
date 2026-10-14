#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace espcontrol {
struct WallpaperBackground {
  lv_obj_t *root;
  bool modal;
  bool enabled = true;
  uint8_t dimming = 0;
};
inline std::vector<WallpaperBackground> &wallpaper_backgrounds() {
  static std::vector<WallpaperBackground> backgrounds;
  return backgrounds;
}
inline auto &wallpaper_background_refresh() {
  static void (*refresh)(lv_obj_t *, bool) = nullptr;
  return refresh;
}
inline void wallpaper_refresh_background(const WallpaperBackground &background) {
  if (wallpaper_background_refresh())
    wallpaper_background_refresh()(background.root, background.modal || !background.enabled);
}
inline void wallpaper_set_background_enabled(lv_obj_t *root, bool enabled) {
  if (!root) return;
  for (auto &background : wallpaper_backgrounds()) {
    if (background.root != root) continue;
    background.enabled = enabled;
    wallpaper_refresh_background(background);
    return;
  }
}
inline void wallpaper_set_background_dimming(lv_obj_t *root, int percent) {
  for (auto &background : wallpaper_backgrounds()) {
    if (background.root != root) continue;
    background.dimming = static_cast<uint8_t>(std::max(0, std::min(100, percent)));
    wallpaper_refresh_background(background);
    return;
  }
}
inline void wallpaper_register_background(lv_obj_t *root, bool modal = false, bool enabled = true) {
  if (!root) return;
  auto &backgrounds = wallpaper_backgrounds();
  for (const auto &entry : backgrounds) if (entry.root == root) return;
  backgrounds.push_back({root, modal, enabled});
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
        wallpaper_refresh_background(entry);
        break;
      }
    }
  };
  lv_obj_add_event_cb(root, refresh, LV_EVENT_SIZE_CHANGED, nullptr);
  lv_obj_add_event_cb(root, refresh, LV_EVENT_SCREEN_LOAD_START, nullptr);
  wallpaper_refresh_background(backgrounds.back());
}
}  // namespace espcontrol
