#pragma once

// LVGL 9 applies this alpha mask to a card and all its children together.
// Earlier LVGL versions and host tests retain their native rounded corners.
#if defined(LVGL_VERSION_MAJOR) && LVGL_VERSION_MAJOR >= 9

#include <cstdlib>
#include <new>
#include "continuous_corner_geometry.h"
#ifdef ESP_PLATFORM
#include "esp_heap_caps.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "src/misc/cache/instance/lv_image_header_cache.h"
#endif

namespace espcontrol::corners {

struct Mask {
  Mask *next = nullptr;
  int width = 0;
  int height = 0;
  int radius = 0;
  unsigned owners = 0;
  uint8_t *pixels = nullptr;
  lv_image_dsc_t image{};
};

inline Mask *&mask_cache() {
  static Mask *head = nullptr;
  return head;
}

inline Mask *acquire(int width, int height, int radius) {
  // Bound allocations to panel-sized cards. Thin widgets use native rounding.
  if (radius <= 0 || width < radius * 4 || height < radius * 4 ||
      width > 1280 || height > 1280) return nullptr;
  for (Mask *mask = mask_cache(); mask; mask = mask->next) {
    if (mask->width == width && mask->height == height && mask->radius == radius) {
      ++mask->owners;
      return mask;
    }
  }
  auto *mask = new (std::nothrow) Mask;
  if (!mask) return nullptr;
  const int stride = lv_draw_buf_width_to_stride(width, LV_COLOR_FORMAT_A8);
  const size_t bytes = static_cast<size_t>(stride) * height;
#ifdef ESP_PLATFORM
  mask->pixels = static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
  mask->pixels = static_cast<uint8_t *>(std::malloc(bytes));
#endif
  if (!mask->pixels) {
    delete mask;
    return nullptr;
  }
  fill_mask(mask->pixels, width, height, stride, radius);
  mask->width = width;
  mask->height = height;
  mask->radius = radius;
  mask->owners = 1;
  mask->image.header.magic = LV_IMAGE_HEADER_MAGIC;
  mask->image.header.cf = LV_COLOR_FORMAT_A8;
  mask->image.header.w = width;
  mask->image.header.h = height;
  mask->image.header.stride = stride;
  mask->image.data_size = bytes;
  mask->image.data = mask->pixels;
  mask->next = mask_cache();
  mask_cache() = mask;
  return mask;
}

inline void release(Mask *mask) {
  if (!mask || --mask->owners) return;
  Mask **entry = &mask_cache();
  while (*entry != mask) entry = &(*entry)->next;
  *entry = mask->next;
  lv_image_cache_drop(&mask->image);
  lv_image_header_cache_drop(&mask->image);
  std::free(mask->pixels);
  delete mask;
}

struct Binding {
  Mask *mask = nullptr;
  bool updating = false;
};

inline void update(lv_obj_t *obj, Binding &binding) {
  if (binding.updating) return;
  const int width = lv_obj_get_width(obj);
  const int height = lv_obj_get_height(obj);
  const int radius = lv_obj_get_style_radius(obj, LV_PART_MAIN);
  if (binding.mask && binding.mask->width == width &&
      binding.mask->height == height && binding.mask->radius == radius) return;
  binding.updating = true;
  Mask *previous = binding.mask;
  binding.mask = acquire(width, height, radius);
  // Allocation failure leaves a usable card with its native circular radius.
  lv_obj_set_style_bitmap_mask_src(obj, binding.mask ? &binding.mask->image : nullptr, LV_PART_MAIN);
  release(previous);
  binding.updating = false;
}

inline void on_event(lv_event_t *event) {
  auto *binding = static_cast<Binding *>(lv_event_get_user_data(event));
  auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(event));
  // Ignore child events bubbled to the card.
  if (lv_event_get_current_target(event) != obj) return;
  const auto code = lv_event_get_code(event);
  if (code == LV_EVENT_DELETE) {
    release(binding->mask);
    delete binding;
  } else if (code == LV_EVENT_SIZE_CHANGED || code == LV_EVENT_STYLE_CHANGED) {
    update(obj, *binding);
  }
}

inline Binding *binding_for(lv_obj_t *obj) {
  if (!obj) return nullptr;
  for (uint32_t i = 0; i < lv_obj_get_event_count(obj); ++i) {
    auto *event = lv_obj_get_event_dsc(obj, i);
    if (lv_event_dsc_get_cb(event) == on_event) {
      return static_cast<Binding *>(lv_event_dsc_get_user_data(event));
    }
  }
  return nullptr;
}

inline void attach(lv_obj_t *obj) {
  if (!obj) return;
  if (auto *binding = binding_for(obj)) {
    update(obj, *binding);
    return;
  }
  auto *binding = new (std::nothrow) Binding;
  if (!binding) return;
  if (!lv_obj_add_event_cb(obj, on_event, LV_EVENT_ALL, binding)) {
    delete binding;
    return;
  }
  update(obj, *binding);
}

inline void detach(lv_obj_t *obj) {
  if (!obj) return;
  for (uint32_t i = 0; i < lv_obj_get_event_count(obj); ++i) {
    auto *event = lv_obj_get_event_dsc(obj, i);
    if (lv_event_dsc_get_cb(event) != on_event) continue;
    auto *binding = static_cast<Binding *>(lv_event_dsc_get_user_data(event));
    lv_obj_remove_event_cb_with_user_data(obj, on_event, binding);
    lv_obj_set_style_bitmap_mask_src(obj, nullptr, LV_PART_MAIN);
    release(binding->mask);
    delete binding;
    return;
  }
}

}  // namespace espcontrol::corners

inline void apply_continuous_card_corners(lv_obj_t *obj) {
  espcontrol::corners::attach(obj);
}
inline bool card_uses_continuous_corners(lv_obj_t *obj) {
  return espcontrol::corners::binding_for(obj) != nullptr;
}
inline void remove_continuous_card_corners(lv_obj_t *obj) { espcontrol::corners::detach(obj); }
#else
inline void apply_continuous_card_corners(lv_obj_t *) {}
inline void remove_continuous_card_corners(lv_obj_t *) {}
inline bool card_uses_continuous_corners(lv_obj_t *) { return false; }
#endif

inline void apply_grid_card_corners(lv_obj_t *obj, int radius, bool continuous) {
  if (!obj) return;
  if (radius >= 0) {
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, radius,
      static_cast<lv_style_selector_t>(LV_PART_MAIN) | static_cast<lv_style_selector_t>(LV_STATE_PRESSED));
  }
  // LVGL's radius property has no layout-update flag, so setting it does not
  // emit STYLE_CHANGED. Refresh an existing mask explicitly after the change.
  if (continuous) apply_continuous_card_corners(obj);
}
