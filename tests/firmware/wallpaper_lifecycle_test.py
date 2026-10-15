"""Run production wallpaper loading with allocation failures and mode changes."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/wallpaper_ui.h').read_text()
state = source[source.index('struct WallpaperUi {'):source.index('inline void wallpaper_widget_deleted')]
tick = source[source.index('inline void wallpaper_tick()'):source.rindex('}  // namespace')]
fixture = r'''
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include "wallpaper_format.h"
#include "wallpaper_decode.h"
struct lv_obj_t {};
struct lv_image_dsc_t {
  struct { unsigned magic, cf, w, h, stride; } header{};
  size_t data_size = 0;
  const uint8_t *data = nullptr;
};
constexpr int LV_IMAGE_HEADER_MAGIC = 1, LV_COLOR_FORMAT_RGB565 = 2, LV_COLOR_FORMAT_RGB888 = 3;
uint32_t now = 0;
namespace esphome { uint32_t millis() { return now; } }
bool lv_is_initialized() { return true; }
bool lv_display_get_default() { return true; }
void lv_image_cache_drop(const void *) {}
uint32_t correct_display_color(uint32_t rgb) { return rgb; }
namespace espcontrol {
/* STATE */
using Refresh = void (*)();
using BackgroundRefresh = void (*)(lv_obj_t *, bool);
Refresh &theme_grid_wallpaper_refresh() { static Refresh callback; return callback; }
BackgroundRefresh &wallpaper_background_refresh() { static BackgroundRefresh callback; return callback; }
void wallpaper_bind_screen(lv_obj_t *, bool) {}
void wallpaper_apply_cards() {}
void wallpaper_apply_card_opacity() {}
struct Store {
  WallpaperBackgroundSetting setting;
  uint32_t version = 1;
  int loads = 0;
  bool fail_load = false;
  auto background() { return setting; }
  WallpaperOpacity opacities() { return {}; }
  bool available() { return true; }
  uint32_t revision() { return version; }
  uint8_t *load(size_t &size, uint32_t &revision) {
    ++loads; size = 1; revision = version;
    return fail_load ? nullptr : static_cast<uint8_t *>(malloc(1));
  }
} store;
Store &wallpaper_store() { return store; }
bool fail_decode = false;
int decodes = 0;
bool wallpaper_decode(const uint8_t *, size_t, DecodedWallpaper &decoded) {
  ++decodes;
  if (fail_decode) return false;
  decoded.pixels = static_cast<uint8_t *>(malloc(4));
  decoded.width = decoded.height = 1; decoded.size = 4;
  return true;
}
/* TICK */
}
int main() {
  using namespace espcontrol;
  auto &ui = wallpaper_ui();
  wallpaper_tick();
  assert(ui.pixels && ui.loaded && store.loads == 1);
  auto *original = ui.pixels;
  store.setting.colour = true;
  wallpaper_tick();
  assert(!ui.pixels && !ui.loaded && ui.retired_pixels == original);
  assert(ui.image.data == nullptr && store.loads == 1);
  now = 999; wallpaper_tick(); assert(ui.retired_pixels == original);
  now = 1000; wallpaper_tick(); assert(!ui.retired_pixels);
  store.setting.colour = false;
  wallpaper_tick(); assert(ui.pixels && ui.loaded && store.loads == 2);
  auto *preserved = ui.pixels;
  ++store.version; store.fail_load = true;
  wallpaper_tick(); assert(store.loads == 3 && ui.pixels == preserved);
  for (int i = 0; i < 100; ++i) wallpaper_tick();
  now = 5999; wallpaper_tick(); assert(store.loads == 3);
  now = 6000; wallpaper_tick(); assert(store.loads == 4);
  // A changed revision bypasses the delay; decoding failures also back off.
  ++store.version; store.fail_load = false; fail_decode = true;
  wallpaper_tick(); assert(store.loads == 5 && ui.pixels == preserved);
  now = 10999; wallpaper_tick(); assert(store.loads == 5);
  now = 11000; fail_decode = false; wallpaper_tick();
  assert(store.loads == 6 && ui.loaded && !ui.retry_pending);
  now = 12000; wallpaper_tick(); assert(store.loads == 6 && !ui.retired_pixels);
  // Unsigned elapsed time remains valid across the millis rollover.
  now = UINT32_MAX - 1000; ++store.version; store.fail_load = true;
  wallpaper_tick(); assert(store.loads == 7);
  now = 100; wallpaper_tick(); assert(store.loads == 7);
  now = 4000; wallpaper_tick(); assert(store.loads == 8);
  free(ui.pixels);
}
'''
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(fixture.replace('/* STATE */', state).replace('/* TICK */', tick))
    subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Wallpaper retirement, reload, allocation/decode backoff and timer rollover passed.')
