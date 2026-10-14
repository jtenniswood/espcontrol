"""Exercise shared wallpaper binding, late screens, resizing and deletion."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/wallpaper_ui.h').read_text()
binding = source[source.index('inline void wallpaper_widget_deleted('):source.index('inline void wallpaper_apply_card(')]
refresh = source[source.index('inline void wallpaper_apply_cards('):source.index('inline void wallpaper_tick(')]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <vector>
#include <cstdint>
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; };
using Callback = void (*)(lv_event_t *);
struct EventBinding { Callback callback; int code; };
struct lv_obj_t {
  lv_obj_t *parent = nullptr;
  std::vector<lv_obj_t *> children;
  std::vector<EventBinding> events;
  int width = 720, height = 720, flags = 0, zoom = 0, opacity = 255;
  int x = 0, y = 0, pads[4] = {};
  const void *source = nullptr;
  bool clip_corner = false;
};
constexpr int LV_EVENT_DELETE = 1, LV_EVENT_SIZE_CHANGED = 2, LV_EVENT_SCREEN_LOAD_START = 3;
constexpr int LV_OBJ_FLAG_IGNORE_LAYOUT = 1, LV_OBJ_FLAG_FLOATING = 2;
constexpr int LV_OBJ_FLAG_CLICKABLE = 4, LV_OBJ_FLAG_SCROLLABLE = 8;
constexpr int LV_ALIGN_CENTER = 0, LV_PART_MAIN = 0, LV_OPA_COVER = 255, LV_OPA_30 = 76;
lv_obj_t *lv_event_get_target(lv_event_t *event) { return event->target; }
void lv_obj_add_event_cb(lv_obj_t *obj, Callback cb, int code, void *) { obj->events.push_back({cb, code}); }
void emit(lv_obj_t *obj, int code) {
  auto events = obj->events;
  lv_event_t event{obj};
  for (auto binding : events) if (binding.code == code) binding.callback(&event);
}
lv_obj_t *lv_image_create(lv_obj_t *parent) {
  auto *obj = new lv_obj_t;
  obj->parent = parent; parent->children.push_back(obj); return obj;
}
lv_obj_t *lv_obj_get_parent(lv_obj_t *obj) { return obj->parent; }
void lv_obj_add_flag(lv_obj_t *obj, int flag) { obj->flags |= flag; }
void lv_obj_clear_flag(lv_obj_t *obj, int flag) { obj->flags &= ~flag; }
void lv_obj_del(lv_obj_t *obj) {
  emit(obj, LV_EVENT_DELETE);
  while (!obj->children.empty()) lv_obj_del(obj->children.back());
  if (obj->parent) {
    auto &children = obj->parent->children;
    children.erase(std::remove(children.begin(), children.end(), obj), children.end());
  }
  delete obj;
}
int lv_obj_get_width(lv_obj_t *obj) { return obj->width; }
int lv_obj_get_height(lv_obj_t *obj) { return obj->height; }
int lv_obj_get_style_pad_left(lv_obj_t *obj, int) { return obj->pads[0]; }
int lv_obj_get_style_pad_right(lv_obj_t *obj, int) { return obj->pads[1]; }
int lv_obj_get_style_pad_top(lv_obj_t *obj, int) { return obj->pads[2]; }
int lv_obj_get_style_pad_bottom(lv_obj_t *obj, int) { return obj->pads[3]; }
void lv_image_set_src(lv_obj_t *obj, const void *source) { obj->source = source; }
void lv_image_set_scale(lv_obj_t *obj, int zoom) { obj->zoom = zoom; }
void lv_obj_set_style_image_opa(lv_obj_t *obj, int opacity, int) { obj->opacity = opacity; }
void lv_obj_set_style_clip_corner(lv_obj_t *obj, bool clip, int) { obj->clip_corner = clip; }
void lv_obj_align(lv_obj_t *obj, int, int x, int y) { obj->x = x; obj->y = y; }
void lv_obj_move_background(lv_obj_t *obj) {
  auto &children = obj->parent->children;
  children.erase(std::remove(children.begin(), children.end(), obj), children.end());
  children.insert(children.begin(), obj);
}
#include "wallpaper_background.h"
namespace espcontrol {
struct Image { struct { int w = 1280, h = 800; } header; };
struct WallpaperUi { uint8_t *pixels = nullptr; Image image; std::vector<lv_obj_t *> widgets; } ui;
WallpaperUi &wallpaper_ui() { return ui; }
struct Grid { lv_obj_t *main_page = nullptr; } grid;
Grid &theme_grid_targets() { return grid; }
struct Page { lv_obj_t *screen; };
std::vector<Page> subpages;
std::vector<Page> &navigation_subpages() { return subpages; }
struct StaticPage { lv_obj_t *page; };
std::vector<StaticPage> setup;
std::vector<StaticPage> &theme_static_pages() { return setup; }
void wallpaper_apply_card_opacity() {}
/* PRODUCTION */
}
int main() {
  using namespace espcontrol;
  auto *home = new lv_obj_t, *initial_subpage = new lv_obj_t, *setup_page = new lv_obj_t;
  auto *clock = new lv_obj_t, *camera = new lv_obj_t, *cover_art = new lv_obj_t;
  auto *modal = new lv_obj_t, *nested = new lv_obj_t;
  subpages.push_back({initial_subpage}); setup.push_back({setup_page});
  home->pads[2] = 60;
  for (auto *screen : {clock, camera, cover_art}) wallpaper_register_background(screen);
  wallpaper_register_background(modal, true); wallpaper_register_background(nested, true);
  wallpaper_register_background(modal, true);
  assert(wallpaper_backgrounds().size() == 5 && ui.widgets.empty());
  wallpaper_background_refresh() = wallpaper_bind_screen;
  ui.pixels = reinterpret_cast<uint8_t *>(1);
  wallpaper_apply_cards();
  assert(ui.widgets.size() == 7);  // Setup/backgrounds work before a home grid exists.
  grid.main_page = home; wallpaper_apply_cards();
  assert(ui.widgets.size() == 8);
  assert(modal->clip_corner && nested->clip_corner);
  auto *foreground = new lv_obj_t; foreground->parent = modal; modal->children.push_back(foreground);
  for (auto *screen : {home, initial_subpage, setup_page, clock, camera, cover_art, modal, nested}) {
    auto *image = screen->children.front();
    assert(image->source == &ui.image && image->zoom == 231);
    assert((image->flags & (LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING)) == 3);
    assert(!(image->flags & (LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)));
    assert(image->opacity == ((screen == modal || screen == nested) ? LV_OPA_30 : LV_OPA_COVER));
  }
  assert(home->children.front()->y == -30);
  auto *late = new lv_obj_t;
  wallpaper_register_background(late);
  assert(late->children.size() == 1 && ui.widgets.size() == 9);
  late->width = 200; late->height = 400; emit(late, LV_EVENT_SIZE_CHANGED);
  assert(late->children.front()->zoom == 128);
  wallpaper_apply_cards();
  assert(ui.widgets.size() == 9 && modal->children.back() == foreground);
  ui.pixels = nullptr; wallpaper_apply_cards();
  assert(ui.widgets.empty() && modal->children.size() == 1 && modal->children.front() == foreground);
  assert(!modal->clip_corner && !nested->clip_corner);
  ui.pixels = reinterpret_cast<uint8_t *>(2); wallpaper_apply_cards();
  assert(ui.widgets.size() == 9);
  lv_obj_del(late); lv_obj_del(nested);
  assert(ui.widgets.size() == 7 && wallpaper_backgrounds().size() == 7);
  for (auto *screen : {home, initial_subpage, setup_page, clock, camera, cover_art, modal}) lv_obj_del(screen);
  assert(ui.widgets.empty() && wallpaper_backgrounds().empty());
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture.replace('/* PRODUCTION */', binding + refresh))
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Shared wallpaper backgrounds, late screens, resizing and cleanup checks passed.')
