"""Exercise the production wallpaper styles for main and subpage card states."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/wallpaper_ui.h').read_text()
start = source.index('inline void wallpaper_apply_card(')
end = source.index('inline void wallpaper_apply_cards()', start)
fixture = r'''
#include <cassert>
#include <map>
#include <vector>
#include "wallpaper_format.h"
using namespace espcontrol;
using lv_opa_t = uint8_t;
using lv_style_selector_t = unsigned;
constexpr unsigned LV_PART_MAIN = 0, LV_STATE_CHECKED = 1, LV_STATE_PRESSED = 32;
constexpr uint8_t LV_OPA_COVER = 255;
struct lv_obj_t { std::map<unsigned, uint8_t> opacity; };
void lv_obj_set_style_bg_opa(lv_obj_t *card, uint8_t opacity, unsigned selector) { card->opacity[selector] = opacity; }
struct WallpaperUi { uint8_t *pixels = nullptr; WallpaperOpacity opacities; WallpaperBackgroundSetting background; } ui;
WallpaperUi &wallpaper_ui() { return ui; }
struct Targets { int count = 2; lv_obj_t *buttons[2]; } targets;
Targets &theme_grid_targets() { return targets; }
struct Card { lv_obj_t *button; };
struct Page { lv_obj_t *back_button; std::vector<Card> cards; };
std::vector<Page> pages;
std::vector<Page> &navigation_subpages() { return pages; }
/* PRODUCTION */
int main() {
  lv_obj_t first, second, back, sub, unrelated;
  targets.buttons[0] = &first; targets.buttons[1] = &second;
  pages.push_back({&back, {{&sub}}});
  ui.pixels = reinterpret_cast<uint8_t *>(1);
  ui.opacities = {50, 90};
  wallpaper_apply_card_opacity();
  for (auto *card : {&first, &second, &back, &sub}) {
    assert(card->opacity.at(LV_PART_MAIN) == 128);
    assert(card->opacity.at(LV_STATE_CHECKED) == 230);
    assert(card->opacity.at(LV_STATE_PRESSED) == 230);
  }
  assert(unrelated.opacity.empty());
  ui.opacities.active = 0;
  wallpaper_apply_card_opacity();
  assert(first.opacity.at(LV_PART_MAIN) == 128);
  assert(first.opacity.at(LV_STATE_CHECKED) == 0);
  ui.pixels = nullptr;
  wallpaper_apply_card_opacity();
  for (auto *card : {&first, &second, &back, &sub})
    for (unsigned state : {LV_PART_MAIN, LV_STATE_CHECKED, LV_STATE_PRESSED}) assert(card->opacity.at(state) == 255);
  ui.background.colour = true;
  wallpaper_apply_card_opacity();
  assert(sub.opacity.at(LV_PART_MAIN) == 128 && sub.opacity.at(LV_STATE_PRESSED) == 0);
  ui.background.colour = false;
  ui.pixels = reinterpret_cast<uint8_t *>(1);
  wallpaper_apply_card_opacity();
  assert(sub.opacity.at(LV_PART_MAIN) == 128 && sub.opacity.at(LV_STATE_PRESSED) == 0);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture.replace('/* PRODUCTION */', source[start:end]))
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Independent off/active card opacity checks passed.')
