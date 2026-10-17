"""Exercise production robot context creation and state rendering for chosen icons."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADERS = ROOT / "components/espcontrol"


def definition(file, name, structure=False):
    pattern = (rf"^struct {name} \{{\n.*?^\}};" if structure else
               rf"^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}")
    matches = re.findall(pattern, (HEADERS / file).read_text(), re.M | re.S)
    assert len(matches) == 1, name
    return matches[0] + "\n"


code = r'''
#include <cassert>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include "esphome/core/string_ref.h"
#include "button_grid_card_runtime.h"
struct lv_obj_t { std::string text; bool checked = false; };
struct ParsedCfg { std::string entity, type, sensor, label, unit, icon; };
struct BtnSlot { lv_obj_t *btn, *icon_lbl, *text_lbl; };
std::map<std::string, std::string> glyphs;
const char *find_icon(const char *name) { return glyphs.emplace(name, name).first->second.c_str(); }
std::string espcontrol_i18n(const std::string &text) { return text; }
void lv_label_set_display_text(lv_obj_t *label, const char *text) { label->text = text; }
void set_wrapped_button_label_text(lv_obj_t *label, const std::string &text) { label->text = text; }
void set_card_checked_state(lv_obj_t *obj, bool checked) { obj->checked = checked; }
'''
for family, context in (("vacuum", "VacuumCardCtx"), ("lawn_mower", "LawnMowerCardCtx")):
    file = f"button_grid_{family}.h"
    code += definition(file, context, True)
    for name in (f"{family}_card_mode", f"{family}_card_default_icon_name", f"{family}_card_mode_label",
                 f"{family}_state_icon_name", f"{family}_state_label", f"create_{family}_card_context",
                 f"apply_{family}_card_state"):
        code += definition(file, name)
code += r'''
int main() {
  lv_obj_t button, icon, label; BtnSlot slot{&button, &icon, &label};
  for (const char *mode : {"status", "start_stop", "start_dock", "pause_resume", "dock"}) {
    // Main pages and subpages share this context and renderer.
      ParsedCfg config{"vacuum.test", "vacuum", mode, "My vacuum", "", "Star"};
      auto *ctx = create_vacuum_card_context(slot, config);
      for (const char *state : {"docked", "cleaning", "returning", "paused", "error", "unknown", "unavailable"}) {
        apply_vacuum_card_state(ctx, esphome::StringRef(state), std::string(state) == "unavailable");
        assert(icon.text == "Star");
        assert(button.checked == (ctx->mode == "start_stop" && ctx->state == "cleaning"));
        button.checked = false;
      }
      delete ctx;
  }
  for (const char *mode : {"status", "start_mowing", "dock", "pause_resume"}) {
    ParsedCfg config{"lawn_mower.test", "lawn_mower", mode, "My mower", "", "Star"};
    auto *ctx = create_lawn_mower_card_context(slot, config);
    for (const char *state : {"docked", "mowing", "paused", "returning", "error", "unknown", "unavailable"}) {
      apply_lawn_mower_card_state(ctx, esphome::StringRef(state), std::string(state) == "unavailable");
      assert(icon.text == "Star");
    }
    delete ctx;
  }
  // Existing concrete mode defaults, empty icons and Auto retain dynamic state icons.
  for (const char *saved : {"", "Auto", "Robot Vacuum"}) {
    ParsedCfg config{"vacuum.test", "vacuum", "status", "", "", saved};
    auto *ctx = create_vacuum_card_context(slot, config);
    apply_vacuum_card_state(ctx, esphome::StringRef("error"), false);
    assert(icon.text == "Robot Vacuum Alert" && label.text == "Error"); delete ctx;
  }
  for (const char *saved : {"", "Auto", "Robot Mower", "Lawnmower"}) {
    ParsedCfg config{"lawn_mower.test", "lawn_mower", "status", "", "", saved};
    auto *ctx = create_lawn_mower_card_context(slot, config);
    for (const char *state : {"docked", "mowing", "error"}) {
      apply_lawn_mower_card_state(ctx, esphome::StringRef(state), false);
      assert(icon.text == lawn_mower_state_icon_name(state));
      assert(label.text == lawn_mower_state_label(state, ctx->label));
    }
    delete ctx;
  }
  puts("Vacuum/Mower custom icons persist across states; automatic icons and labels remain dynamic.");
}
'''
with tempfile.TemporaryDirectory(prefix="robot-card-icons-") as temporary:
    source = Path(temporary) / "test.cpp"
    binary = Path(temporary) / "test"
    source.write_text(code)
    compiler = shlex.split(os.environ.get("CXX", sys.argv[1] if len(sys.argv) > 1 else "c++"))
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(HEADERS), "-I", str(ROOT / "tests/firmware/stubs"), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
