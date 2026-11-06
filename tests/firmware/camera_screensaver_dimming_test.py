#!/usr/bin/env python3
"""Run the production image-dimming lambda with both supported LVGL APIs."""
from pathlib import Path
import subprocess
import tempfile
import yaml

root = Path(__file__).resolve().parents[2]
class Loader(yaml.SafeLoader):
    pass
Loader.add_constructor('!lambda', lambda loader, node: loader.construct_scalar(node))
Loader.add_constructor('!include', lambda loader, node: loader.construct_scalar(node))
camera = yaml.load((root / 'common/device/screen_camera_screensaver.yaml').read_text(), Loader)
display = yaml.load((root / 'common/config/display.yaml').read_text(), Loader)
setting = next(n for n in display['number'] if n['id'] == 'screensaver_camera_dimming')
assert (setting['min_value'], setting['max_value'], setting['step'], setting['initial_value']) == (0, 100, 1, 0)
assert setting['restore_value'] and setting['optimistic']
assert {'script.execute': 'camera_screensaver_apply_dimming'} in setting['on_value']['then']
assert any('global_preferences->sync()' in a.get('lambda', '') for a in setting['on_value']['then'])
scripts = {s['id']: s for s in camera['script']}
entry = scripts['display_mode_effect_camera']['then']
assert entry.index({'script.execute': 'camera_screensaver_apply_dimming'}) < entry.index({'lvgl.widget.show': 'camera_screensaver'})
parent = camera['lvgl']['top_layer']['widgets'][0]['obj']
assert parent['bg_color'] == 0 and parent['bg_opa'] == '100%'
render = scripts['camera_screensaver_apply_dimming']['then'][0]['lambda']
source = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#define VERSION_CODE(a,b,c) ((a)*10000+(b)*100+(c))
#define id(x) x
using lv_opa_t = unsigned char;
constexpr int LV_PART_MAIN = 0;
struct Widget { int opacity = 255; } image, clock_label, metadata, dashboard, cover_art;
Widget *camera_screensaver_image = &image;
struct Number { float state = 0; } screensaver_camera_dimming;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026,4,0)
void lv_obj_set_style_image_opa(Widget *widget, lv_opa_t opacity, int) { widget->opacity = opacity; }
#else
void lv_obj_set_style_img_opa(Widget *widget, lv_opa_t opacity, int) { widget->opacity = opacity; }
#endif
void apply() {
''' + render + r'''
}
int main() {
  for (const auto &test : {std::pair<float, int>{0,255}, {25,191}, {50,128}, {100,0}, {-20,255}, {120,0}}) {
    screensaver_camera_dimming.state = test.first;
    apply();
    assert(image.opacity == test.second);
    assert(clock_label.opacity == 255 && metadata.opacity == 255 && dashboard.opacity == 255 && cover_art.opacity == 255);
  }
  screensaver_camera_dimming.state = std::numeric_limits<float>::quiet_NaN();
  apply(); assert(image.opacity == 255);
  screensaver_camera_dimming.state = std::numeric_limits<float>::infinity();
  apply(); assert(image.opacity == 255);
  camera_screensaver_image = nullptr; apply(); // Restore may run before widgets are ready.
  camera_screensaver_image = &image;
  screensaver_camera_dimming.state = 50; apply(); assert(image.opacity == 128); // Entry reapplies the restored value.
}
'''
with tempfile.TemporaryDirectory(prefix='camera-dimming-') as directory:
    cpp = Path(directory) / 'test.cpp'
    cpp.write_text(source)
    for version in (20250900, 20260901):
        binary = Path(directory) / str(version)
        subprocess.run(['c++', '-std=c++17', '-Wall', '-Wextra', '-Werror', f'-DESPHOME_VERSION_CODE={version}', str(cpp), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print('Camera dimming endpoints, restored state, overlay isolation and both LVGL APIs passed.')
