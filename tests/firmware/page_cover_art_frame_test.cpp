#include "page_cover_art_frame.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "src/misc/cache/instance/lv_image_cache.h"

using namespace espcontrol::pages;
static size_t allocations = 0, live_bytes = 0;
struct alignas(std::max_align_t) Allocation {
  size_t bytes;
};
extern "C" {
void lv_mem_init() {}
void lv_mem_deinit() {}
lv_mem_pool_t lv_mem_add_pool(void*, size_t) {
  return nullptr;
}
void lv_mem_remove_pool(lv_mem_pool_t) {}
void* lv_malloc_core(size_t size) {
  ++allocations;
  auto* p = static_cast<Allocation*>(std::malloc(size + sizeof(Allocation)));
  assert(p);
  p->bytes = size;
  live_bytes += size;
  return p + 1;
}
void lv_free_core(void* ptr) {
  if (ptr) {
    auto* p = static_cast<Allocation*>(ptr) - 1;
    live_bytes -= p->bytes;
    std::free(p);
  }
}
void* lv_realloc_core(void* ptr, size_t size) {
  if (!ptr) return lv_malloc_core(size);
  if (!size) {
    lv_free_core(ptr);
    return nullptr;
  }
  auto* p = static_cast<Allocation*>(ptr) - 1;
  const auto old = p->bytes;
  auto* next = lv_malloc_core(size);
  std::memcpy(next, ptr, std::min(size, old));
  lv_free_core(ptr);
  return next;
}
void lv_mem_monitor_core(lv_mem_monitor_t* m) {
  std::memset(m, 0, sizeof(*m));
}
lv_result_t lv_mem_test_core() {
  return LV_RESULT_OK;
}
void __wrap_lv_display_add_event_cb(lv_display_t*, lv_event_cb_t, lv_event_code_t, void*);
uint32_t __wrap_lv_display_remove_event_cb_with_user_data(lv_display_t*, lv_event_cb_t, void*);
}
static std::array<uint16_t, 96 * 96> pixels{};
static size_t cpp_allocations = 0;
void* operator new(size_t size) {
  ++cpp_allocations;
  if (void* ptr = std::malloc(size ? size : 1)) return ptr;
  std::abort();
}
void* operator new[](size_t size) {
  return ::operator new(size);
}
void operator delete(void* ptr) noexcept {
  std::free(ptr);
}
void operator delete[](void* ptr) noexcept {
  std::free(ptr);
}
void operator delete(void* ptr, std::size_t) noexcept {
  std::free(ptr);
}
void operator delete[](void* ptr, std::size_t) noexcept {
  std::free(ptr);
}
static void draw(lv_event_t* event) {
  auto* frame = static_cast<PageCoverArtFrame*>(lv_event_get_user_data(event));
  auto* surface = static_cast<lv_obj_t*>(lv_event_get_target(event));
  const auto snapshot = *frame;
  if (lv_event_get_code(event) == LV_EVENT_REFR_EXT_DRAW_SIZE) {
    snapshot.extend_draw_size(surface, *static_cast<int32_t*>(lv_event_get_param(event)));
    return;
  }
  if (lv_event_get_code(event) != LV_EVENT_DRAW_MAIN) return;
  if (snapshot.draw_image(surface, lv_event_get_layer(event)))
    snapshot.draw_tint(surface, lv_event_get_layer(event));
}
static void reuse_and_retirement(lv_display_t* display) {
  auto* root = lv_obj_create(lv_screen_active());
  lv_obj_set_pos(root, 8, 8);
  lv_obj_set_size(root, 72, 68);
  lv_obj_set_style_pad_all(root, 0, 0);
  lv_obj_set_style_border_width(root, 0, 0);
  lv_obj_set_style_radius(root, 11, 0);
  lv_obj_set_style_clip_corner(root, true, 0);
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  auto* surface = lv_obj_create(root);
  lv_obj_remove_style_all(surface);
  lv_obj_set_pos(surface, 0, 0);
  lv_obj_set_size(surface, 72, 68);
  lv_obj_set_style_radius(surface, 11, 0);
  lv_obj_set_style_clip_corner(surface, true, 0);
  lv_obj_clear_flag(surface, LV_OBJ_FLAG_SCROLLABLE);
  auto* first = static_cast<uint16_t*>(std::malloc(64 * 64 * 2));
  auto* second = static_cast<uint16_t*>(std::malloc(64 * 64 * 2));
  assert(first && second);
  for (size_t i = 0; i < 64 * 64; ++i) {
    first[i] = i * 73;
    second[i] = i * 13;
  }
  lv_image_dsc_t a{};
  a.header.magic = LV_IMAGE_HEADER_MAGIC;
  a.header.cf = LV_COLOR_FORMAT_RGB565;
  a.header.w = a.header.h = 64;
  a.header.stride = 128;
  a.data_size = 64 * 64 * 2;
  a.data = reinterpret_cast<const uint8_t*>(first);
  auto b = a;
  b.data = reinterpret_cast<const uint8_t*>(second);
  PageCoverArtFrame frame{&a, 0x134521, 72, 68, false};
  lv_obj_add_event_cb(surface, draw, LV_EVENT_ALL, &frame);
  auto update = [&](size_t cycle) {
    frame.source = cycle % 2 ? &a : &b;
    frame.tint = (cycle / 2) % 2;
    frame.tint_color = cycle % 2 ? 0x134521 : 0x371215;
    lv_obj_refresh_ext_draw_size(surface);
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(display);
  };
  for (size_t i = 0; i < 8; ++i) update(i);
  const auto retained = live_bytes, cpp = cpp_allocations;
  for (size_t i = 0; i < 1000; ++i) {
    update(i);
    assert(live_bytes == retained && cpp_allocations == cpp);
  }
  // Retirement clears the frame before storage can be released. A late draw or
  // extent query cannot read either freed pixel buffer after that point.
  frame = {};
  lv_obj_refresh_ext_draw_size(surface);
  lv_draw_wait_for_finish();
  lv_image_cache_drop(&a);
  lv_image_cache_drop(&b);
  std::free(first);
  std::free(second);
  lv_obj_invalidate(lv_screen_active());
  lv_refr_now(display);
  assert(!frame.draw_image(surface, nullptr) && frame.scale(nullptr) == 0);
  lv_obj_delete(root);
  std::printf(
      "Cover frame: 1000 changes, stable retained LVGL bytes, zero C++ allocations; retirement PASS\n");
}
int main() {
  lv_init();
  auto* display = lv_display_create(96, 96);
  static std::array<uint16_t, 96 * 32> buffer{};
  lv_display_set_buffers(display, buffer.data(), nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t* a, uint8_t* bytes) {
    auto* data = reinterpret_cast<uint16_t*>(bytes);
    size_t i = 0;
    for (int y = a->y1; y <= a->y2; ++y)
      for (int x = a->x1; x <= a->x2; ++x) {
        if (x >= 0 && x < 96 && y >= 0 && y < 96) pixels[y * 96 + x] = data[i];
        ++i;
      }
    lv_display_flush_ready(d);
  });
  static std::array<uint16_t, 64 * 64> source{};
  for (size_t i = 0; i < source.size(); ++i) source[i] = static_cast<uint16_t>((i * 73) ^ ((i / 32) * 19));
  size_t cases = 0;
  for (int width : {67, 78})
    for (int height : {59, 74})
      for (int sw : {32, 49, 64})
        for (int sh : {27, 51, 64})
          for (int radius : {0, 9, 25, LV_RADIUS_CIRCLE})
            for (int border : {0, 1, 3})
              for (bool tint : {false, true})
                for (int overscan : {0, 1})
                  for (int opacity_mode : {0, 1, 2, 3})
                    for (bool antialias : {false, true})
                      for (bool fit : {false, true}) {
                        lv_display_set_antialiasing(display, antialias);
                        lv_image_dsc_t image{};
                        image.header.magic = LV_IMAGE_HEADER_MAGIC;
                        image.header.cf = LV_COLOR_FORMAT_RGB565;
                        image.header.w = sw;
                        image.header.h = sh;
                        image.header.stride = sw * 2;
                        image.data_size = sw * sh * 2;
                        image.data = reinterpret_cast<const uint8_t*>(source.data());
                        auto* root = lv_obj_create(lv_screen_active());
                        lv_obj_set_pos(root, 7, 8);
                        lv_obj_set_size(root, width, height);
                        lv_obj_set_style_bg_color(root, lv_color_hex(0x212121), 0);
                        lv_obj_set_style_pad_all(root, 6, 0);
                        lv_obj_set_style_border_width(root, border, 0);
                        lv_obj_set_style_radius(root, radius, 0);
                        lv_obj_set_style_clip_corner(root, true, 0);
                        lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
                        if (opacity_mode == 1 || opacity_mode == 3) lv_obj_set_style_opa(root, LV_OPA_70, 0);
                        if (opacity_mode == 2 || opacity_mode == 3)
                          lv_obj_set_style_opa_layered(root, LV_OPA_70, 0);
                        auto* reference = lv_image_create(root);
                        lv_obj_set_size(reference, width + overscan, height);
                        lv_obj_set_pos(reference, -6, -6);
                        lv_obj_set_style_pad_all(reference, 0, 0);
                        lv_obj_set_style_border_width(reference, 0, 0);
                        lv_obj_set_style_bg_opa(reference, LV_OPA_TRANSP, 0);
                        lv_image_set_inner_align(reference,
                                                 fit ? LV_IMAGE_ALIGN_CONTAIN : LV_IMAGE_ALIGN_COVER);
                        lv_image_set_src(reference, &image);
                        lv_obj_set_style_radius(reference, radius, 0);
                        lv_obj_set_style_clip_corner(reference, true, 0);
                        auto* overlay = lv_obj_create(root);
                        lv_obj_remove_style_all(overlay);
                        lv_obj_set_size(overlay, width, height);
                        lv_obj_set_pos(overlay, -6, -6);
                        lv_obj_set_style_bg_color(overlay, lv_color_hex(0x134521), 0);
                        lv_obj_set_style_bg_opa(overlay, LV_OPA_60, 0);
                        if (!tint) lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
                        auto* actual = lv_obj_create(root);
                        lv_obj_remove_style_all(actual);
                        lv_obj_set_pos(actual, -6, -6);
                        lv_obj_set_size(actual, width + overscan, height);
                        lv_obj_set_style_radius(actual, radius, 0);
                        lv_obj_set_style_clip_corner(actual, true, 0);
                        lv_obj_clear_flag(actual, LV_OBJ_FLAG_SCROLLABLE);
                        PageCoverArtFrame frame{&image, 0x134521, width, height, tint, fit};
                        lv_obj_add_event_cb(actual, draw, LV_EVENT_ALL, &frame);
                        lv_obj_refresh_ext_draw_size(actual);
                        lv_obj_add_flag(actual, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_update_layout(root);
                        lv_obj_invalidate(lv_screen_active());
                        lv_refr_now(display);
                        const auto expected = pixels;
                        lv_obj_add_flag(reference, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(overlay, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_clear_flag(actual, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_invalidate(lv_screen_active());
                        lv_refr_now(display);
                        size_t diff = 0;
                        for (size_t i = 0; i < pixels.size(); ++i) diff += pixels[i] != expected[i];
                        if (diff) {
                          for (size_t i = 0, n = 0; i < pixels.size() && n < 12; ++i)
                            if (pixels[i] != expected[i]) {
                              std::fprintf(stderr, " (%zu,%zu):%04x/%04x", i % 96, i / 96, pixels[i],
                                           expected[i]);
                              ++n;
                            }
                          std::fprintf(stderr, "\n");
                          std::fprintf(stderr,
                                       "diff=%zu w%d h%d src%dx%d r%d b%d tint%d overscan%d opacityMode%d "
                                       "aa%d fit%d\n",
                                       diff, width, height, sw, sh, radius, border, tint, overscan,
                                       opacity_mode, antialias, fit);
                          return 1;
                        }
                        lv_obj_delete(root);
                        lv_image_cache_drop(&image);
                        ++cases;
                      }
  std::printf("Cover image/tint frame: %zu pixel cases PASS\n", cases);
  reuse_and_retirement(display);
}
