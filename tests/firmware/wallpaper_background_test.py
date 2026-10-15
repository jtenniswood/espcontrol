"""Exercise shared wallpaper binding, late screens, resizing and deletion."""
from pathlib import Path
import os
import subprocess
import tempfile
import yaml

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/wallpaper_ui.h').read_text()
binding = source[source.index('inline void wallpaper_widget_deleted('):source.index('inline void wallpaper_apply_card(')]
refresh = source[source.index('inline void wallpaper_apply_cards('):source.index('inline void wallpaper_tick(')]
loading_hooks = []
for name in ('screen_loading.yaml', 'screen_loading_ethernet.yaml'):
    config = yaml.safe_load((root / 'common/device' / name).read_text())
    boot = next(action for action in config['esphome']['on_boot'] if action['priority'] == -210)
    loading_hooks.append(boot['then'][0]['lambda'].replace('id(loading_page)->obj', 'loading_page->obj'))
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
  int recolor_opacity = 0;
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
int lv_color_hex(int value) { return value; }
void lv_obj_set_style_image_recolor(lv_obj_t *, int color, int) { assert(color == 0); }
void lv_obj_set_style_image_recolor_opa(lv_obj_t *obj, int opacity, int) { obj->recolor_opacity = opacity; }
void lv_obj_set_style_image_opa(lv_obj_t *obj, int opacity, int) { obj->opacity = opacity; }
void lv_obj_set_style_clip_corner(lv_obj_t *obj, bool clip, int) { obj->clip_corner = clip; }
void lv_obj_align(lv_obj_t *obj, int, int x, int y) { obj->x = x; obj->y = y; }
void lv_obj_move_background(lv_obj_t *obj) {
  auto &children = obj->parent->children;
  children.erase(std::remove(children.begin(), children.end(), obj), children.end());
  children.insert(children.begin(), obj);
}
#include "wallpaper_background.h"
#include "wallpaper_format.h"
namespace espcontrol {
struct Image { struct { int w = 1280, h = 800; } header; };
struct WallpaperUi { uint8_t *pixels = nullptr; Image image, colour_image; WallpaperBackgroundSetting background; std::vector<lv_obj_t *> widgets; } ui;
WallpaperUi &wallpaper_ui() { return ui; }
struct Grid { lv_obj_t *main_page = nullptr; } grid;
Grid &theme_grid_targets() { return grid; }
struct Page { lv_obj_t *screen; };
std::vector<Page> subpages;
std::vector<Page> &navigation_subpages() { return subpages; }
struct StaticPage { lv_obj_t *page; };
std::vector<StaticPage> setup;
std::vector<StaticPage> &theme_static_pages() { return setup; }
void register_theme_static_page(lv_obj_t *page) { setup.push_back({page}); }
void wallpaper_apply_card_opacity() {}
/* PRODUCTION */
}
int main() {
  using namespace espcontrol;
  auto *home = new lv_obj_t, *initial_subpage = new lv_obj_t, *setup_page = new lv_obj_t;
  auto *clock = new lv_obj_t, *camera = new lv_obj_t, *cover_art = new lv_obj_t;
  auto *modal = new lv_obj_t, *nested = new lv_obj_t;
  auto *loading = new lv_obj_t, *ethernet_loading = new lv_obj_t;
  struct PageRef { lv_obj_t *obj; } ref{loading};
  auto *loading_page = &ref;
  /* WIFI LOADING */
  loading_page->obj = ethernet_loading;
  /* ETHERNET LOADING */
  subpages.push_back({initial_subpage}); setup.push_back({setup_page});
  home->pads[2] = 60;
  for (auto *screen : {camera, cover_art}) wallpaper_register_background(screen);
  wallpaper_register_background(clock, false, false);
  wallpaper_register_background(modal, true); wallpaper_register_background(nested, true);
  wallpaper_register_background(modal, true);
  assert(ui.widgets.empty());
  wallpaper_background_refresh() = wallpaper_bind_screen;
  // A restored disabled clock remains black when the image first loads.
  ui.pixels = reinterpret_cast<uint8_t *>(1);
  wallpaper_apply_cards();
  assert(loading->children.empty() && ethernet_loading->children.empty());
  assert(clock->children.empty() && ui.widgets.size() == 4);
  emit(clock, LV_EVENT_SIZE_CHANGED); emit(clock, LV_EVENT_SCREEN_LOAD_START);
  assert(clock->children.empty());
  wallpaper_set_background_enabled(clock, true);
  assert(ui.widgets.size() == 5);  // Setup/backgrounds work before a home grid exists.
  grid.main_page = home; wallpaper_apply_cards();
  assert(ui.widgets.size() == 6);
  assert(modal->children.empty() && nested->children.empty());
  assert(!modal->clip_corner && !nested->clip_corner);
  auto *foreground = new lv_obj_t; foreground->parent = modal; modal->children.push_back(foreground);
  for (auto *screen : {home, initial_subpage, setup_page, clock, camera, cover_art}) {
    auto *image = screen->children.front();
    assert(image->source == &ui.image && image->zoom == 231);
    assert((image->flags & (LV_OBJ_FLAG_IGNORE_LAYOUT | LV_OBJ_FLAG_FLOATING)) == 3);
    assert(!(image->flags & (LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE)));
    assert(image->opacity == LV_OPA_COVER);
  }
  assert(home->children.front()->y == -30);
  // Regular wallpaper dims independently; the clock uses its own setting.
  ui.background.dimming = 50;
  wallpaper_set_background_dimming(clock, 0);
  wallpaper_apply_cards();
  assert(home->children.front()->recolor_opacity == 128);
  assert(initial_subpage->children.front()->recolor_opacity == 128);
  assert(clock->children.front()->recolor_opacity == 0);
  ui.background.dimming = 100; wallpaper_apply_cards();
  assert(home->children.front()->recolor_opacity == 255);
  // Clock-only dimming fades the wallpaper against black without touching text.
  auto *clock_text = new lv_obj_t; clock_text->parent = clock; clock->children.push_back(clock_text);
  wallpaper_set_background_dimming(clock, 40);
  assert(clock->children.front()->opacity == 153 && clock_text->opacity == 255);
  assert(home->children.front()->opacity == 255 && camera->children.front()->opacity == 255);
  emit(clock, LV_EVENT_SIZE_CHANGED); emit(clock, LV_EVENT_SCREEN_LOAD_START);
  assert(clock->children.front()->opacity == 153);
  wallpaper_set_background_dimming(clock, 200);
  assert(clock->children.front()->opacity == 0 && clock_text->opacity == 255);
  wallpaper_set_background_dimming(clock, -10);
  assert(clock->children.front()->opacity == 255);
  wallpaper_set_background_dimming(clock, 40);
  lv_obj_del(clock_text);
  auto *late = new lv_obj_t;
  wallpaper_register_background(late);
  assert(late->children.size() == 1 && ui.widgets.size() == 7);
  late->width = 200; late->height = 400; emit(late, LV_EVENT_SIZE_CHANGED);
  assert(late->children.front()->zoom == 128);
  wallpaper_apply_cards();
  assert(ui.widgets.size() == 7 && modal->children.size() == 1 && modal->children.front() == foreground);
  assert(nested->children.empty());
  wallpaper_set_background_enabled(clock, false);
  wallpaper_apply_cards();
  emit(clock, LV_EVENT_SIZE_CHANGED); emit(clock, LV_EVENT_SCREEN_LOAD_START);
  assert(clock->children.empty() && ui.widgets.size() == 6);
  assert(home->children.front()->source == &ui.image && camera->children.front()->source == &ui.image);
  wallpaper_set_background_enabled(modal, true);
  assert(modal->children.size() == 1 && modal->children.front() == foreground);
  // Colour replaces the shared source without allocating another image widget.
  auto *same_widget = home->children.front();
  ui.colour_image.header = {8, 8}; ui.background.colour = true;
  wallpaper_apply_cards();
  assert(clock->children.empty() && ui.widgets.size() == 6);
  wallpaper_set_background_enabled(clock, true);
  assert(clock->children.front()->source == &ui.colour_image);
  assert(clock->children.front()->opacity == 153);
  assert(ui.widgets.size() == 7 && home->children.front() == same_widget);
  assert(same_widget->source == &ui.colour_image && same_widget->zoom == 23040);
  assert(same_widget->recolor_opacity == 0);  // Colour is unaffected by wallpaper dimming.
  assert(initial_subpage->children.front()->source == &ui.colour_image);
  for (auto *screen : {loading, ethernet_loading}) {
    emit(screen, LV_EVENT_SIZE_CHANGED); emit(screen, LV_EVENT_SCREEN_LOAD_START);
    assert(screen->children.empty());
  }
  assert(modal->children.front() == foreground && nested->children.empty());
  ui.pixels = nullptr; wallpaper_apply_cards();
  assert(ui.widgets.size() == 7);  // Colour needs no uploaded wallpaper.
  ui.background.colour = false;
  ui.pixels = nullptr; wallpaper_apply_cards();
  assert(ui.widgets.empty() && modal->children.size() == 1 && modal->children.front() == foreground);
  assert(!modal->clip_corner && !nested->clip_corner);
  ui.pixels = reinterpret_cast<uint8_t *>(2); wallpaper_apply_cards();
  assert(ui.widgets.size() == 7);
  lv_obj_del(late); lv_obj_del(nested);
  assert(ui.widgets.size() == 6 && wallpaper_backgrounds().size() == 9);
  for (auto *screen : {home, initial_subpage, setup_page, clock, camera, cover_art, modal, loading, ethernet_loading}) lv_obj_del(screen);
  assert(ui.widgets.empty() && wallpaper_backgrounds().empty());
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture.replace('/* PRODUCTION */', binding + refresh).replace('/* WIFI LOADING */', loading_hooks[0]).replace('/* ETHERNET LOADING */', loading_hooks[1]))
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Shared wallpaper backgrounds, solid startup/loading pages and modals, late screens, resizing and cleanup checks passed.')
