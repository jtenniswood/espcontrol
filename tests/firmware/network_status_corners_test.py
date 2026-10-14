"""Exercise the production network-card construction with real mask bindings."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--network-header", type=Path, default=ROOT / "components/espcontrol/network_status.h")
args = parser.parse_args()
# Reuse the existing LVGL host adapter and production corner implementation.
adapter = (ROOT / "tests/firmware/continuous_corners_test.cpp").read_text().split("int main() {")[0]
subpages = (ROOT / "components/espcontrol/button_grid_subpages.h").read_text()
start = subpages.index("inline lv_obj_t *create_grid_card_button(")
factory = subpages[start:subpages.index("\n}", start) + 2]
network = args.network_header.read_text()
start = network.index("inline void network_status_open_modal(")
network = network[start:network.index("\n}", start) + 2]
radius = re.search(r"const lv_coord_t radius =[^;]+;", network)
construction = re.search(r"auto \*button = create_grid_card_button\([^;]+;", network)
assert radius and construction
# Compile the actual construction statements, including any corner-policy lookup,
# rather than reproducing the arguments or mask decision in the test fixture.
policy = "\n".join(line.strip() for line in network.splitlines()
                   if "card_uses_continuous_corners(reference)" in line)
fixture = r'''
using lv_coord_t = int;
using lv_color_t = uint32_t;
struct lv_font_t {};
lv_obj_t *lv_btn_create(lv_obj_t *) { return new lv_obj_t; }
void lv_obj_set_style_pad_all(lv_obj_t *, int, int) {}
void lv_obj_set_style_text_font(lv_obj_t *, const lv_font_t *, int) {}
void lv_obj_set_style_text_color(lv_obj_t *, lv_color_t, int) {}
void lv_obj_set_style_shadow_width(lv_obj_t *, int, int) {}
int control_modal_card_radius(lv_obj_t *reference) { return reference ? reference->radius : 8; }
'''
fixture += factory + r'''
#include "../../components/espcontrol/network_status_layout.h"
lv_obj_t *network_card(lv_obj_t *reference) {
  struct { lv_obj_t *overlay = nullptr; } ui;
  int card_pad = 0;
  const lv_font_t *label_font = nullptr;
  lv_color_t text_color = 0;
'''
fixture += radius.group(0) + "\n" + policy + "\n" + construction.group(0) + "\nreturn button;\n}\n"
fixture += r'''
int main() {
  using namespace espcontrol::corners;
  for (int radius : {14, 20, 26}) {
    lv_obj_t reference;
    reference.width = 145; reference.height = 140;
    apply_grid_card_corners(&reference, radius, true);
    for (int index = 0; index < NETWORK_STATUS_CARD_COUNT; ++index) {
      auto *card = network_card(&reference);
      // Grid layout gives newly created cards their actual footprint.
      card->width = 145; card->height = 140;
      send(*card, LV_EVENT_SIZE_CHANGED);
      assert(card->radius == radius && card->pressed_radius == radius);
      assert(card_uses_continuous_corners(card) && card->mask);
      assert(binding_for(card)->mask->radius == radius);
      send(*card, LV_EVENT_DELETE);
      delete card;
    }
    send(reference, LV_EVENT_DELETE);
    assert(mask_cache() == nullptr);
  }
  lv_obj_t regular_reference;
  apply_grid_card_corners(&regular_reference, 8, false);
  for (auto *reference : {&regular_reference, static_cast<lv_obj_t *>(nullptr)}) {
    auto *card = network_card(reference);
    assert(card->radius == 8 && !card_uses_continuous_corners(card) && !card->mask);
    send(*card, LV_EVENT_DELETE);
    delete card;
  }
  assert(mask_cache() == nullptr);
}
'''
with tempfile.TemporaryDirectory(prefix="espcontrol-network-corners-") as directory:
    source = Path(directory) / "network_cards.cpp"
    executable = Path(directory) / "network_cards"
    source.write_text(adapter + fixture)
    subprocess.run([os.environ.get("CXX", "g++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                    "-I", str(ROOT / "tests/firmware"), str(source), "-o", str(executable)], check=True)
    subprocess.run([str(executable)], check=True)
print("All network-status cards inherit the selected radius and continuous corners.")
