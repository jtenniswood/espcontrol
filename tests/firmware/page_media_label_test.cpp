#include "page_media_label.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "button_grid_string.h"
#include "page_label_refresh.h"
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
struct Arena : PageArenaAllocator {
  bool fail{false};
  size_t bytes{0};
  void* allocate(size_t size) override {
    if (fail) return nullptr;
    bytes += size;
    return std::malloc(size);
  }
  void release(void* p, size_t size) override {
    bytes -= size;
    std::free(p);
  }
};

static std::array<uint16_t, 96 * 96> pixels{};
int main() {
  lv_init();
  auto* display = lv_display_create(96, 96);
  static std::array<uint16_t, 96 * 32> buffer{};
  lv_display_set_buffers(display, buffer.data(), nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, [](lv_display_t* d, const lv_area_t* a, uint8_t* bytes) {
    auto* data = reinterpret_cast<uint16_t*>(bytes);
    size_t index = 0;
    for (int y = a->y1; y <= a->y2; ++y)
      for (int x = a->x1; x <= a->x2; ++x) {
        if (x >= 0 && x < 96 && y >= 0 && y < 96) pixels[y * 96 + x] = data[index];
        ++index;
      }
    lv_display_flush_ready(d);
  });
  auto* root = lv_screen_active();
  auto* actual = lv_label_create(root);
  auto* reference = lv_label_create(root);
  char text[260]{};
  lv_label_set_text_static(actual, text);
  lv_label_set_text(reference, "");
  for (auto* label : {actual, reference}) {
    lv_obj_set_pos(label, 4, 4);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
  }
  Arena arena;
  {
    PageLabelRefresh refresh(arena);
    lv_obj_t* labels[] = {actual};
    assert(refresh.initialize(labels, 1));
    const std::array<std::string, 10> titles = {"Short",
                                                "A very long title with many words in it to test truncation",
                                                u8"Beyoncé — Αθήνα Москва 日本語",
                                                "ABCDEFGHIJKabcdefghijklmnopqrstuvwxyz0123456789",
                                                "123456789012345678901234567890",
                                                "First\nSecond\nThird",
                                                "a b c d e f g h i j k l m n o p",
                                                "",
                                                std::string(256, 'A'),
                                                u8"Ṣade ṣ has a very long Unicode title to display"};
    lv_font_t taller = lv_font_montserrat_14;
    taller.line_height = 20;
    for (const auto* font : {&lv_font_montserrat_14, static_cast<const lv_font_t*>(&taller)})
      for (auto align : {LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_CENTER, LV_TEXT_ALIGN_RIGHT})
        for (int scale : {256, 280, 320})
          for (int spacing : {-1, 0, 2})
            for (int line_limit : {0, 1, 2})
              for (int width : {12, 34, 64, 87})
                for (const auto& title : titles) {
                  for (auto* label : {actual, reference}) {
                    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
                    lv_obj_set_style_text_align(label, align, LV_PART_MAIN);
                    lv_obj_set_style_transform_scale_x(label, scale == 280 ? 256 : scale, LV_PART_MAIN);
                    lv_obj_set_style_transform_scale_y(label, scale == 280 ? scale : 256, LV_PART_MAIN);
                    lv_obj_set_width(label, width);
                    lv_obj_set_style_text_line_space(label, spacing, LV_PART_MAIN);
                    lv_obj_set_style_max_height(
                        label,
                        line_limit ? font->line_height * line_limit + spacing * (line_limit - 1)
                                   : LV_COORD_MAX,
                        LV_PART_MAIN);
                  }
                  lv_obj_set_height(reference, line_limit == 1 ? font->line_height : LV_SIZE_CONTENT);
                  lv_label_set_long_mode(reference, line_limit ? LV_LABEL_LONG_DOT : LV_LABEL_LONG_WRAP);
                  lv_label_set_text(reference, normalize_display_text(title).c_str());
                  lv_obj_add_flag(actual, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_clear_flag(reference, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_update_layout(root);
                  lv_obj_invalidate(root);
                  lv_refr_now(display);
                  const auto expected = pixels;
                  lv_obj_add_flag(reference, LV_OBJ_FLAG_HIDDEN);
                  lv_obj_clear_flag(actual, LV_OBJ_FLAG_HIDDEN);
                  write_media_label(actual, text, sizeof(text), title, line_limit);
                  lv_obj_update_layout(root);
                  lv_obj_invalidate(root);
                  lv_refr_now(display);
                  if (pixels != expected) {
                    std::fprintf(stderr,
                                 "ellipsis mismatch width=%d lines=%d align=%d scale=%d spacing=%d font=%d "
                                 "title=%s actual=%s reference=%s\n",
                                 width, line_limit, int(align), scale, spacing, font->line_height,
                                 title.c_str(), lv_label_get_text(actual), lv_label_get_text(reference));
                    std::abort();
                  }
                  assert(lv_label_get_text(actual) == text &&
                         lv_label_get_long_mode(actual) == LV_LABEL_LONG_WRAP);
                }
    lv_obj_delete(reference);
    const auto before = allocations, steady = live_bytes, descriptors = refresh.memory().descriptor_adds;
    for (size_t i = 0; i < 1000; ++i) {
      write_media_label(actual, text, sizeof(text), titles[i % 2], 2);
      lv_obj_update_layout(root);
      assert(allocations == before && live_bytes == steady &&
             refresh.memory().descriptor_adds == descriptors);
    }
    assert(refresh.shutdown());
  }
  lv_display_delete(display);
  lv_deinit();
  assert(live_bytes == 0 && arena.bytes == 0);
}
