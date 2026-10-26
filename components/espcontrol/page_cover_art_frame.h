#pragma once

#include <algorithm>
#include <cstdint>

#include "lvgl.h"

namespace espcontrol::pages {
// Draw into a bank-owned surface which is a child of the rounded card. The
// surface has the same position/size as the old image widget; the parent's
// corner clipping still applies. No image/overlay widget or pixel copy is owned
// here. Clear this frame before retiring its image lease, then drain draw work
// on maintenance before the transport releases the descriptor/pixel storage.
struct PageCoverArtFrame {
  const lv_image_dsc_t* source{nullptr};
  uint32_t tint_color{0};
  int32_t overlay_width{0}, overlay_height{0};
  bool tint{false};
  bool fit{false};

  int32_t scale(lv_obj_t* surface) const {
    if (!surface || !source || !source->data || source->header.w == 0 || source->header.h == 0) return 0;
    const int32_t width = lv_obj_get_width(surface), height = lv_obj_get_height(surface);
    if (width <= 0 || height <= 0) return 0;
    const int64_t scale_x = static_cast<int64_t>(width) * LV_SCALE_NONE / source->header.w;
    const int64_t scale_y = static_cast<int64_t>(height) * LV_SCALE_NONE / source->header.h;
    const int64_t result =
        fit ? std::max<int64_t>(1, std::min(scale_x, scale_y)) : std::max(scale_x, scale_y);
    // LVGL's image transform bounds use a 16-bit scale. Real decoded artwork
    // is already sized for its card; reject an invalid extreme transform.
    return result > 0 && result <= UINT16_MAX ? static_cast<int32_t>(result) : 0;
  }

  void extend_draw_size(lv_obj_t* surface, int32_t& size) const {
    const auto factor = scale(surface);
    if (!factor) return;
    const auto width = lv_obj_get_width(surface), height = lv_obj_get_height(surface);
    lv_point_t end{width, height};
    const lv_point_t pivot{0, 0};
    // Public geometry API, with positive scale, zero rotation and a zero pivot.
    // Match the image widget's transformed extent so border-edge pixels are not
    // clipped by the generic surface before reaching the rounded parent clip.
    lv_point_transform(&end, 0, factor, factor, &pivot, true);
    size = std::max({size, end.x - 1 - width, end.y - 1 - height});
  }

  bool draw_image(lv_obj_t* surface, lv_layer_t* layer) const {
    const auto factor = scale(surface);
    if (!factor || !layer) return false;
    lv_area_t area;
    lv_obj_get_coords(surface, &area);
    const auto width = lv_obj_get_width(surface), height = lv_obj_get_height(surface);
    const int32_t image_width = source->header.w, image_height = source->header.h;
    const int32_t scaled_width = static_cast<int64_t>(image_width) * factor / LV_SCALE_NONE;
    const int32_t scaled_height = static_cast<int64_t>(image_height) * factor / LV_SCALE_NONE;
    const int32_t x = (width - scaled_width) / 2;
    const int32_t y = (height - scaled_height) / 2;
    const lv_area_t coordinates{area.x1 + x, area.y1 + y, area.x1 + x + image_width - 1,
                                area.y1 + y + image_height - 1};
    lv_draw_image_dsc_t image;
    lv_draw_image_dsc_init(&image);
    image.base.layer = layer;
    lv_obj_init_draw_image_dsc(surface, LV_PART_MAIN, &image);
    image.src = source;
    image.scale_x = image.scale_y = factor;
    image.pivot = {0, 0};
    image.image_area = coordinates;
    image.clip_radius = lv_obj_get_style_radius(surface, LV_PART_MAIN);
    image.antialias = LV_COLOR_DEPTH > 8;
    lv_draw_image(layer, &image, &coordinates);
    return true;
  }

  // Keep this separate so the port can revalidate its binding after LVGL has
  // dispatched any draw-task callbacks for the image and before adding tint.
  void draw_tint(lv_obj_t* surface, lv_layer_t* layer) const {
    if (!tint || !surface || !layer || overlay_width <= 0 || overlay_height <= 0) return;
    lv_area_t area;
    lv_obj_get_coords(surface, &area);
    area.x2 = area.x1 + overlay_width - 1;
    area.y2 = area.y1 + overlay_height - 1;
    lv_draw_rect_dsc_t overlay;
    lv_draw_rect_dsc_init(&overlay);
    overlay.base.obj = surface;
    overlay.base.layer = layer;
    const auto recolor = lv_obj_get_style_recolor_recursive(surface, LV_PART_MAIN);
    overlay.bg_color = lv_color_mix(lv_color_make(recolor.red, recolor.green, recolor.blue),
                                    lv_color_hex(tint_color), recolor.alpha);
    const auto opacity = lv_obj_get_style_opa_recursive(surface, LV_PART_MAIN);
    overlay.bg_opa =
        opacity < LV_OPA_MAX ? LV_OPA_MIX2(LV_OPA_60, opacity) : static_cast<lv_opa_t>(LV_OPA_60);
    lv_draw_rect(layer, &overlay, &area);
  }
};
}  // namespace espcontrol::pages
