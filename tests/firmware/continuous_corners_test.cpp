#include <array>
#include <cassert>
#include <cstdint>
#include <vector>
#include <cmath>

#define LVGL_VERSION_MAJOR 9
constexpr int LV_PART_MAIN = 0;
constexpr int LV_STATE_PRESSED = 1;
using lv_style_selector_t = unsigned;
constexpr int LV_IMAGE_HEADER_MAGIC = 0x19;
constexpr int LV_COLOR_FORMAT_A8 = 1;
enum lv_event_code_t { LV_EVENT_ALL, LV_EVENT_SIZE_CHANGED, LV_EVENT_STYLE_CHANGED, LV_EVENT_DELETE };
struct lv_event_t;
using lv_event_cb_t = void (*)(lv_event_t *);
struct lv_event_dsc_t { lv_event_cb_t cb; void *data; };
struct lv_image_dsc_t {
  struct { unsigned magic = 0, cf = 0, w = 0, h = 0, stride = 0; } header;
  uint32_t data_size = 0;
  const uint8_t *data = nullptr;
};
struct lv_obj_t {
  int width = 100, height = 80, radius = 16;
  int pressed_radius = 16;
  const void *mask = nullptr;
  std::vector<lv_event_dsc_t> events;
};
struct lv_event_t { lv_obj_t *target; lv_obj_t *current; lv_event_code_t code; void *data; };
void *lv_event_get_user_data(lv_event_t *e) { return e->data; }
void *lv_event_get_target(lv_event_t *e) { return e->target; }
void *lv_event_get_current_target(lv_event_t *e) { return e->current; }
lv_event_code_t lv_event_get_code(lv_event_t *e) { return e->code; }
int lv_obj_get_width(lv_obj_t *o) { return o->width; }
int lv_obj_get_height(lv_obj_t *o) { return o->height; }
int lv_obj_get_style_radius(lv_obj_t *o, int) { return o->radius; }
void lv_obj_set_style_radius(lv_obj_t *o, int radius, lv_style_selector_t selector) {
  // Match LVGL 9: changing radius alone does not emit STYLE_CHANGED.
  if (selector & LV_STATE_PRESSED) o->pressed_radius = radius;
  else o->radius = radius;
}
uint32_t lv_obj_get_event_count(lv_obj_t *o) { return o->events.size(); }
lv_event_dsc_t *lv_obj_get_event_dsc(lv_obj_t *o, unsigned i) { return &o->events[i]; }
lv_event_cb_t lv_event_dsc_get_cb(lv_event_dsc_t *d) { return d->cb; }
void *lv_event_dsc_get_user_data(lv_event_dsc_t *d) { return d->data; }
lv_event_dsc_t *lv_obj_add_event_cb(lv_obj_t *o, lv_event_cb_t cb, lv_event_code_t, void *data) {
  o->events.push_back({cb, data});
  return &o->events.back();
}
void send(lv_obj_t &o, lv_event_code_t code, lv_obj_t *target = nullptr) {
  for (auto &d : o.events) {
    lv_event_t event{target ? target : &o, &o, code, d.data};
    d.cb(&event);
  }
  if (code == LV_EVENT_DELETE && !target) o.events.clear();
}
void lv_obj_set_style_bitmap_mask_src(lv_obj_t *o, const void *mask, int) {
  o->mask = mask;
  send(*o, LV_EVENT_STYLE_CHANGED); // Exercise recursive style notifications.
}
unsigned lv_draw_buf_width_to_stride(unsigned width, int) { return (width + 3) & ~3U; }
int dropped = 0;
void lv_image_cache_drop(const void *) { ++dropped; }
void lv_image_header_cache_drop(const void *) { ++dropped; }
#include "../../components/espcontrol/button_grid_continuous_corners.h"
#include "../../components/espcontrol/card_curve.h"

int main() {
  using namespace espcontrol::corners;
  // Four corners must mirror each other, with a solid interior and partial
  // edge coverage. Check non-aligned dimensions as well as wide/tall cards.
  for (const auto &size : {std::pair{101, 81}, std::pair{400, 80}, std::pair{100, 320}}) {
    const int stride = lv_draw_buf_width_to_stride(size.first, LV_COLOR_FORMAT_A8);
    std::vector<uint8_t> pixels(stride * size.second);
    fill_mask(pixels.data(), size.first, size.second, stride, 16);
    assert(pixels[0] == 0);
    assert(pixels[(size.second / 2) * stride + size.first / 2] == 255);
    bool antialias = false;
    for (int y = 0; y < size.second; ++y) for (int x = 0; x < size.first; ++x) {
      const auto alpha = pixels[y * stride + x];
      assert(alpha == pixels[y * stride + size.first - 1 - x]);
      assert(alpha == pixels[(size.second - 1 - y) * stride + x]);
      antialias |= alpha > 0 && alpha < 255;
    }
    assert(antialias);
  }
  // The continuous outline lies inside the retained native 16px circular
  // clipping, so native image/slider rounding cannot cut off this new shape.
  for (int y = 0; y < 32; ++y) for (int x = 0; x < 32; ++x) {
    for (int sy = 0; sy < 4; ++sy) for (int sx = 0; sx < 4; ++sx) {
      const double px = x + (sx + 0.5) / 4, py = y + (sy + 0.5) / 4;
      const double dx = 1 - px / 32, dy = 1 - py / 32;
      if (std::pow(dx, 4) + std::pow(dy, 4) <= 1 && px < 16 && py < 16)
        assert((16 - px) * (16 - px) + (16 - py) * (16 - py) <= 16 * 16);
    }
  }
  lv_obj_t first, second;
  attach(&first); attach(&second); attach(&first);
  assert(first.events.size() == 1);
  assert(first.mask && first.mask == second.mask);
  assert(mask_cache()->owners == 2);
  assert(mask_cache()->next == nullptr);
  first.width = 240;
  send(first, LV_EVENT_SIZE_CHANGED);
  assert(first.mask != second.mask);
  assert(mask_cache()->next && !mask_cache()->next->next);
  // A bubbled child delete must not free the parent's mask/binding.
  lv_obj_t child;
  send(first, LV_EVENT_DELETE, &child);
  assert(first.mask && first.events.size() == 1);
  send(first, LV_EVENT_DELETE);
  assert(mask_cache()->owners == 1 && !mask_cache()->next);
  second.width = 20; // Thin controls fall back instead of being cropped.
  send(second, LV_EVENT_SIZE_CHANGED);
  assert(second.mask == nullptr && mask_cache() == nullptr);
  second.width = 100;
  send(second, LV_EVENT_SIZE_CHANGED);
  assert(second.mask && mask_cache());
  send(second, LV_EVENT_DELETE);
  assert(mask_cache() == nullptr && dropped == 6);
  // Switching saved presets must replace the outline and release the old
  // mask. Exercise the S3's rounded whole-pixel sizes on a real card footprint.
  for (const auto &radii : {std::array{20, 12, 16}, std::array{22, 13, 18},
                            std::array{35, 21, 28}, std::array{26, 16, 21}}) {
    assert(card_curve_radius(radii[0], "Small") == radii[1]);
    assert(card_curve_radius(radii[0], "Medium") == radii[2]);
    assert(card_curve_radius(radii[0], "Large") == radii[0]);
  }
  assert(card_curve_radius(25, "invalid") == 20);
  lv_obj_t adjustable;
  adjustable.width = 145; adjustable.height = 140; adjustable.radius = 26;
  attach(&adjustable);
  for (const auto &preset : {std::pair{"Small", 16}, std::pair{"Medium", 21}, std::pair{"Large", 26}}) {
    apply_grid_card_corners(&adjustable, card_curve_radius(26, preset.first, 16, 21), true);
    assert(adjustable.radius == preset.second);
    assert(adjustable.pressed_radius == preset.second);
    assert(mask_cache() && mask_cache()->radius == preset.second);
    assert(mask_cache()->owners == 1 && !mask_cache()->next);
  }
  send(adjustable, LV_EVENT_DELETE);
  assert(mask_cache() == nullptr && dropped == 14);
  // A radius-only update used to retain a Large mask over a Small card.
  // Home navigation, nested cards and Back must all use the same policy.
  lv_obj_t home, nested, back;
  for (auto *card : {&home, &nested, &back}) {
    card->width = 145; card->height = 140;
    apply_grid_card_corners(card, 26, true);
    lv_obj_set_style_radius(card, 14, LV_PART_MAIN);
    assert(binding_for(card)->mask->radius == 26); // Reproduce the old mismatch.
    apply_grid_card_corners(card, 14, true);
    assert(card_uses_continuous_corners(card));
    assert(binding_for(card)->mask->radius == 14);
    assert(card->pressed_radius == 14 && card->events.size() == 1);
  }
  assert(home.mask == nested.mask && nested.mask == back.mask);
  assert(mask_cache()->owners == 3 && !mask_cache()->next);
  for (auto *card : {&home, &nested, &back}) send(*card, LV_EVENT_DELETE);
  assert(mask_cache() == nullptr);
  // Hidden subpage cards start without usable dimensions; their first layout
  // creates the same Small outline, without needing a radius style event.
  lv_obj_t hidden;
  hidden.width = hidden.height = 0;
  apply_grid_card_corners(&hidden, 14, true);
  assert(card_uses_continuous_corners(&hidden) && !hidden.mask);
  hidden.width = 145; hidden.height = 140;
  send(hidden, LV_EVENT_SIZE_CHANGED);
  assert(binding_for(&hidden)->mask->radius == 14);
  send(hidden, LV_EVENT_DELETE);
  assert(mask_cache() == nullptr);
}
