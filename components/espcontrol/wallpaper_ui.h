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
  bool retry_pending = false;
  uint32_t retry_revision = 0;
  uint32_t retry_started_at = 0;
  WallpaperOpacity opacities;
  WallpaperBackgroundSetting background;
  lv_image_dsc_t image{};
  lv_image_dsc_t colour_image{};
  uint16_t colour_pixels[64]{};
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
inline const auto *wallpaper_image_source() {
  const auto &ui = wallpaper_ui();
  return ui.background.colour ? &ui.colour_image : ui.pixels ? &ui.image : nullptr;
}
inline void wallpaper_bind_screen(lv_obj_t *screen, bool solid = false) {
  if (!screen) return;
  auto &ui = wallpaper_ui();
  lv_obj_t *widget = nullptr;
  for (auto *candidate : ui.widgets) if (lv_obj_get_parent(candidate) == screen) { widget = candidate; break; }
  // Modal panels and screens with backgrounds disabled keep their solid fill.
  const auto *image = wallpaper_image_source();
  if (solid || !image) { if (widget) lv_obj_del(widget); return; }
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
  lv_image_set_src(widget, image);
  int dimming = 0;
  bool screensaver_dimming = false;
  for (const auto &background : wallpaper_backgrounds())
    if (background.root == screen) { dimming = background.dimming; screensaver_dimming = background.dimming_override; break; }
  // Recolour the wallpaper itself toward black; cards and text remain unchanged.
  // The clock has its own dimming, applied by image opacity against its black root.
  lv_obj_set_style_image_recolor(widget, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_image_recolor_opa(widget,
      !ui.background.colour && !screensaver_dimming ? wallpaper_opacity_byte(ui.background.dimming) : 0, LV_PART_MAIN);
  // Fade only the background image against the root's solid fill; text stays bright.
  lv_obj_set_style_image_opa(widget, (255 * (100 - dimming) + 50) / 100, LV_PART_MAIN);
  const int zoom = std::max((width * 256 + int(image->header.w) - 1) / int(image->header.w),
                            (height * 256 + int(image->header.h) - 1) / int(image->header.h));
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
  const bool enabled = ui.background.colour || ui.pixels;
  const lv_opa_t off = enabled ? wallpaper_opacity_byte(ui.opacities.off) : static_cast<lv_opa_t>(LV_OPA_COVER);
  const lv_opa_t active = enabled ? wallpaper_opacity_byte(ui.opacities.active) : static_cast<lv_opa_t>(LV_OPA_COVER);
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
    wallpaper_refresh_background(background);
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
  if (!lv_is_initialized() || !lv_display_get_default()) return;
  const auto background = store.background();
  if (background.colour != ui.background.colour || background.rgb != ui.background.rgb || background.dimming != ui.background.dimming) {
    ui.background = background;
    const uint32_t rgb = correct_display_color(background.rgb);
    const uint16_t pixel = ((rgb >> 19) << 11) | (((rgb >> 10) & 63) << 5) | ((rgb >> 3) & 31);
    std::fill(ui.colour_pixels, ui.colour_pixels + 64, pixel);
    lv_image_cache_drop(&ui.colour_image);
    ui.colour_image = {};
    ui.colour_image.header.magic = LV_IMAGE_HEADER_MAGIC;
    ui.colour_image.header.cf = LV_COLOR_FORMAT_RGB565;
    ui.colour_image.header.w = ui.colour_image.header.h = 8;
    ui.colour_image.header.stride = 16;
    ui.colour_image.data_size = sizeof(ui.colour_pixels);
    ui.colour_image.data = reinterpret_cast<const uint8_t *>(ui.colour_pixels);
    wallpaper_apply_cards();
  }
  const auto opacities = store.opacities();
  if (opacities.off != ui.opacities.off || opacities.active != ui.opacities.active) {
    ui.opacities = opacities;
    wallpaper_apply_card_opacity();
  }
  if (ui.background.colour) {
    if (ui.pixels) {
      // Widgets already use colour_image; keep pixels alive until drawing finishes.
      lv_image_cache_drop(&ui.image);
      ui.retired_pixels = ui.pixels;
      ui.retired_at = esphome::millis();
      ui.pixels = nullptr;
      ui.image = {};
    }
    ui.loaded = false;  // Reload the preserved flash copy when Wallpaper returns.
    ui.retry_pending = false;
    return;
  }
  if (!store.available()) return;
  const uint32_t revision = store.revision();
  if (ui.loaded && revision == ui.revision) return;
  // Retry an unchanged image at most once every five seconds, including across
  // millis() rollover. A replacement upload can be attempted immediately.
  if (ui.retry_pending && ui.retry_revision == revision &&
      uint32_t(esphome::millis() - ui.retry_started_at) < 5000) return;
  ui.retry_pending = true;
  ui.retry_revision = revision;
  ui.retry_started_at = esphome::millis();
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
  ui.retry_pending = false;
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
