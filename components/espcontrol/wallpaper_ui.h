#pragma once
#include "wallpaper.h"
#include "wallpaper_decode.h"
#include "wallpaper_background.h"
#include "src/misc/cache/instance/lv_image_cache.h"

// Included after theme_runtime_ui.h; all LVGL work stays on the main task.
namespace espcontrol {
struct WallpaperUi {
  uint8_t *pixels = nullptr;
  uint32_t revision = 0;
  bool loaded = false;
  WallpaperOpacity opacities;
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
inline void wallpaper_bind_screen(lv_obj_t *screen, bool modal = false) {
  if (!screen) return;
  auto &ui = wallpaper_ui();
  lv_obj_t *widget = nullptr;
  for (auto *candidate : ui.widgets) if (lv_obj_get_parent(candidate) == screen) { widget = candidate; break; }
  // Modal panels keep their solid theme fill, regardless of card opacity.
  if (modal || !ui.pixels) { if (widget) lv_obj_del(widget); return; }
  const int width = lv_obj_get_width(screen), height = lv_obj_get_height(screen);
  if (width <= 0 || height <= 0) return;
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
  lv_obj_set_style_image_opa(widget, LV_OPA_COVER, LV_PART_MAIN);
  const int zoom = std::max((width * 256 + int(ui.image.header.w) - 1) / int(ui.image.header.w),
                            (height * 256 + int(ui.image.header.h) - 1) / int(ui.image.header.h));
  lv_image_set_scale(widget, zoom);
  // Grid padding reserves space for the clock bar. Centre the wallpaper on
  // the whole screen rather than the padded card area, so it covers the HUD.
  lv_obj_align(widget, LV_ALIGN_CENTER,
      (lv_obj_get_style_pad_right(screen, LV_PART_MAIN) - lv_obj_get_style_pad_left(screen, LV_PART_MAIN)) / 2,
      (lv_obj_get_style_pad_bottom(screen, LV_PART_MAIN) - lv_obj_get_style_pad_top(screen, LV_PART_MAIN)) / 2);
  lv_obj_move_background(widget);
}
inline void wallpaper_apply_card(lv_obj_t *card, lv_opa_t off, lv_opa_t active) {
  if (!card) return;
  lv_obj_set_style_bg_opa(card, off, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(card, active, static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_CHECKED);
  lv_obj_set_style_bg_opa(card, active, static_cast<lv_style_selector_t>(LV_PART_MAIN) | LV_STATE_PRESSED);
}
inline void wallpaper_apply_card_opacity() {
  const auto &ui = wallpaper_ui();
  const lv_opa_t off = ui.pixels ? wallpaper_opacity_byte(ui.opacities.off) : static_cast<lv_opa_t>(LV_OPA_COVER);
  const lv_opa_t active = ui.pixels ? wallpaper_opacity_byte(ui.opacities.active) : static_cast<lv_opa_t>(LV_OPA_COVER);
  auto &targets = theme_grid_targets();
  for (int i = 0; i < targets.count; ++i)
    wallpaper_apply_card(targets.buttons[i], off, active);
  for (auto &page : navigation_subpages()) {
    wallpaper_apply_card(page.back_button, off, active);
    for (auto &card : page.cards)
      wallpaper_apply_card(card.button, off, active);
  }
}
inline void wallpaper_apply_cards() {
  wallpaper_register_background(theme_grid_targets().main_page);
  for (auto &page : navigation_subpages()) wallpaper_register_background(page.screen);
  for (auto &page : theme_static_pages()) wallpaper_register_background(page.page);
  for (const auto &background : wallpaper_backgrounds())
    wallpaper_bind_screen(background.root, background.modal);
  wallpaper_apply_card_opacity();
}
inline void wallpaper_tick() {
  theme_grid_wallpaper_refresh() = wallpaper_apply_cards;
  wallpaper_background_refresh() = wallpaper_bind_screen;
  auto &ui = wallpaper_ui();
  // Keep the old buffer through in-flight display drawing, like artwork images.
  if (ui.retired_pixels && esphome::millis() - ui.retired_at < 1000) return;
  free(ui.retired_pixels);
  ui.retired_pixels = nullptr;
  auto &store = wallpaper_store();
  if (!lv_is_initialized() || !lv_display_get_default() || !store.available()) return;
  const auto opacities = store.opacities();
  if (opacities.off != ui.opacities.off || opacities.active != ui.opacities.active) {
    ui.opacities = opacities;
    wallpaper_apply_card_opacity();
  }
  const uint32_t revision = store.revision();
  if (ui.loaded && revision == ui.revision) return;
  size_t size; uint32_t loaded_revision;
  uint8_t *packet = store.load(size, loaded_revision);
  if (size && !packet) return;  // Keep the old image and retry if allocation fails.
  DecodedWallpaper decoded;
  const bool decoded_ok = !packet || wallpaper_decode(packet, size, decoded);
  free(packet);
  if (!decoded_ok) return;
  lv_image_cache_drop(&ui.image);
  auto *previous = ui.pixels;
  ui.pixels = decoded.pixels;
  ui.revision = loaded_revision;
  ui.loaded = true;
  ui.image = {};
  if (ui.pixels) {
    ui.image.header.magic = LV_IMAGE_HEADER_MAGIC;
    ui.image.header.cf = decoded.rgb888 ? LV_COLOR_FORMAT_RGB888 : LV_COLOR_FORMAT_RGB565;
    ui.image.header.w = decoded.width;
    ui.image.header.h = decoded.height;
    ui.image.header.stride = ui.image.header.w * (decoded.rgb888 ? 3 : 2);
    ui.image.data_size = decoded.size;
    ui.image.data = decoded.pixels;
  }
  wallpaper_apply_cards();
  ui.retired_pixels = previous;
  ui.retired_at = esphome::millis();
}
}  // namespace espcontrol
