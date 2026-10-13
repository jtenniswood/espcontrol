#pragma once
#include "wallpaper.h"

// Included after theme_runtime_ui.h; all LVGL work stays on the main task.
namespace espcontrol {
struct WallpaperUi {
  uint8_t *pixels = nullptr;
  uint32_t revision = 0;
  bool loaded = false;
  lv_img_dsc_t image{};
  std::vector<lv_obj_t *> widgets;
};
inline WallpaperUi &wallpaper_ui() { static WallpaperUi ui; return ui; }
inline void wallpaper_widget_deleted(lv_event_t *event) {
  auto &widgets = wallpaper_ui().widgets;
  auto *widget = lv_event_get_target(event);
  widgets.erase(std::remove(widgets.begin(), widgets.end(), widget), widgets.end());
}
inline void wallpaper_bind_screen(lv_obj_t *screen) {
  if (!screen) return;
  auto &ui = wallpaper_ui();
  lv_obj_t *widget = nullptr;
  for (auto *candidate : ui.widgets) if (lv_obj_get_parent(candidate) == screen) { widget = candidate; break; }
  if (!ui.pixels) { if (widget) lv_obj_del(widget); return; }
  if (!widget) {
    widget = lv_img_create(screen);
    lv_obj_add_flag(widget, LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(widget, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(widget, wallpaper_widget_deleted, LV_EVENT_DELETE, nullptr);
    ui.widgets.push_back(widget);
  }
  lv_img_set_src(widget, &ui.image);
  lv_obj_update_layout(screen);
  const int width = lv_obj_get_width(screen), height = lv_obj_get_height(screen);
  const int zoom = std::max((width * 256 + int(ui.image.header.w) - 1) / int(ui.image.header.w),
                            (height * 256 + int(ui.image.header.h) - 1) / int(ui.image.header.h));
  lv_img_set_zoom(widget, zoom);
  lv_obj_center(widget);
  lv_obj_move_background(widget);
}
inline void wallpaper_apply_cards() {
  const lv_opa_t opacity = wallpaper_ui().pixels ? 204 : LV_OPA_COVER;
  auto &targets = theme_grid_targets();
  wallpaper_bind_screen(targets.main_page);
  for (int i = 0; i < targets.count; ++i)
    if (targets.buttons[i]) lv_obj_set_style_bg_opa(targets.buttons[i], opacity, LV_PART_MAIN);
  for (auto &page : navigation_subpages()) {
    wallpaper_bind_screen(page.screen);
    if (page.back_button) lv_obj_set_style_bg_opa(page.back_button, opacity, LV_PART_MAIN);
    for (auto &card : page.cards)
      if (card.button) lv_obj_set_style_bg_opa(card.button, opacity, LV_PART_MAIN);
  }
}
inline void wallpaper_tick() {
  theme_grid_wallpaper_refresh() = wallpaper_apply_cards;
  auto &ui = wallpaper_ui();
  auto &store = wallpaper_store();
  if (!theme_grid_targets().main_page || !store.available()) return;
  const uint32_t revision = store.revision();
  if (ui.loaded && revision == ui.revision) return;
  size_t size; uint32_t loaded_revision;
  uint8_t *pixels = store.load(size, loaded_revision);
  if (size && !pixels) return;  // Keep the old image and retry if allocation fails.
  lv_img_cache_invalidate_src(&ui.image);
  auto *previous = ui.pixels;
  ui.pixels = pixels;
  ui.revision = loaded_revision;
  ui.loaded = true;
  ui.image = {};
  if (pixels) {
    ui.image.header.cf = LV_IMG_CF_TRUE_COLOR;
    ui.image.header.w = wallpaper_u32(pixels + 4);
    ui.image.header.h = wallpaper_u32(pixels + 8);
    ui.image.data_size = size - WALLPAPER_HEADER_BYTES;
    ui.image.data = pixels + WALLPAPER_HEADER_BYTES;
  }
  wallpaper_apply_cards();
  free(previous);
}
}  // namespace espcontrol
