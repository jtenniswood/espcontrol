#pragma once
#include "wallpaper.h"
#include "src/misc/cache/instance/lv_image_cache.h"

// Included after theme_runtime_ui.h; all LVGL work stays on the main task.
namespace espcontrol {
struct WallpaperUi {
  uint8_t *pixels = nullptr;
  uint32_t revision = 0;
  bool loaded = false;
  lv_image_dsc_t image{};
  uint8_t *retired_pixels = nullptr;
  uint32_t retired_at = 0;
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
    widget = lv_image_create(screen);
    lv_obj_add_flag(widget, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(widget, LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(widget, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(widget, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(widget, wallpaper_widget_deleted, LV_EVENT_DELETE, nullptr);
    ui.widgets.push_back(widget);
  }
  lv_image_set_src(widget, &ui.image);
  lv_obj_update_layout(screen);
  const int width = lv_obj_get_width(screen), height = lv_obj_get_height(screen);
  const int zoom = std::max((width * 256 + int(ui.image.header.w) - 1) / int(ui.image.header.w),
                            (height * 256 + int(ui.image.header.h) - 1) / int(ui.image.header.h));
  lv_image_set_scale(widget, zoom);
  lv_obj_center(widget);
  lv_obj_move_background(widget);
}
inline void wallpaper_apply_card(lv_obj_t *card, lv_opa_t opacity) {
  if (!card) return;
  lv_obj_set_style_bg_opa(card, opacity, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(card, opacity, LV_PART_MAIN | LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(card, opacity, LV_PART_MAIN | LV_STATE_PRESSED);
}
inline void wallpaper_apply_cards() {
  const lv_opa_t opacity = wallpaper_ui().pixels ? 204 : LV_OPA_COVER;
  auto &targets = theme_grid_targets();
  wallpaper_bind_screen(targets.main_page);
  for (int i = 0; i < targets.count; ++i)
    wallpaper_apply_card(targets.buttons[i], opacity);
  for (auto &page : navigation_subpages()) {
    wallpaper_bind_screen(page.screen);
    wallpaper_apply_card(page.back_button, opacity);
    for (auto &card : page.cards)
      wallpaper_apply_card(card.button, opacity);
  }
}
inline void wallpaper_tick() {
  theme_grid_wallpaper_refresh() = wallpaper_apply_cards;
  auto &ui = wallpaper_ui();
  // Keep the old buffer through in-flight display drawing, like artwork images.
  if (ui.retired_pixels && esphome::millis() - ui.retired_at < 1000) return;
  free(ui.retired_pixels);
  ui.retired_pixels = nullptr;
  auto &store = wallpaper_store();
  if (!theme_grid_targets().main_page || !store.available()) return;
  const uint32_t revision = store.revision();
  if (ui.loaded && revision == ui.revision) return;
  size_t size; uint32_t loaded_revision;
  uint8_t *pixels = store.load(size, loaded_revision);
  if (size && !pixels) return;  // Keep the old image and retry if allocation fails.
  lv_image_cache_drop(&ui.image);
  auto *previous = ui.pixels;
  ui.pixels = pixels;
  ui.revision = loaded_revision;
  ui.loaded = true;
  ui.image = {};
  if (pixels) {
    ui.image.header.magic = LV_IMAGE_HEADER_MAGIC;
    ui.image.header.cf = LV_COLOR_FORMAT_RGB565;
    ui.image.header.w = wallpaper_u32(pixels + 4);
    ui.image.header.h = wallpaper_u32(pixels + 8);
    ui.image.header.stride = ui.image.header.w * 2;
    ui.image.data_size = size - WALLPAPER_HEADER_BYTES;
    ui.image.data = pixels + WALLPAPER_HEADER_BYTES;
  }
  wallpaper_apply_cards();
  ui.retired_pixels = previous;
  ui.retired_at = esphome::millis();
}
}  // namespace espcontrol
