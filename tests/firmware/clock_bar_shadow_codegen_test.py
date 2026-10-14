"""Ensure clock-bar and screensaver styles use LVGL alpha text shadows."""
from pathlib import Path
import subprocess
import sys
import tempfile

import yaml

ROOT = Path(__file__).resolve().parents[2]

class Loader(yaml.SafeLoader):
    pass

Loader.add_constructor("!lambda", lambda loader, node: loader.construct_scalar(node))
shared = yaml.load((ROOT / "common/config/display.yaml").read_text(), Loader=Loader)
labels = ["temperatures", "display_time", "network_status_icon_label", "night_mode_status_icon_label"]
screensaver = yaml.load((ROOT / "common/device/screen_clock.yaml").read_text(), Loader=Loader)
screensaver_widgets = next(widget["obj"]["widgets"] for widget in screensaver["lvgl"]["top_layer"]["widgets"]
                           if widget["obj"]["id"] == "clock_screensaver")
screensaver_labels = [{"label": {key: value for key, value in widget["label"].items()
                                 if key in ("id", "text", "styles", "hidden")}}
                      for widget in screensaver_widgets if "label" in widget]
updates = [action for boot in shared["esphome"]["on_boot"] for action in boot["then"]
           if "lvgl.label.update" in action]
fixture = {
    "esphome": {"name": "clock-shadow-test"},
    "esp32": {"board": "esp32dev", "framework": {"type": "esp-idf"}},
    "spi": {"clk_pin": 18, "mosi_pin": 23},
    "display": [{"platform": "ili9xxx", "id": "shadow_display", "model": "ILI9341",
                 "dc_pin": 4, "cs_pin": 5, "invert_colors": False,
                 "auto_clear_enabled": False, "update_interval": "never"}],
    "lvgl": {**shared.get("lvgl", {}), "displays": ["shadow_display"],
             "pages": [{"widgets": [{"label": {"id": label, "text": "13:14"}}
                                    for label in labels] + screensaver_labels}]},
}
if updates:
    fixture["esphome"]["on_boot"] = [{"priority": -100, "then": updates}]
with tempfile.TemporaryDirectory(prefix="espcontrol-clock-shadow-") as directory:
    root = Path(directory)
    config = root / "clock-shadow.yaml"
    config.write_text(yaml.safe_dump(fixture, sort_keys=False))
    result = subprocess.run([sys.executable, "-m", "esphome", "compile", str(config), "--only-generate"],
                            capture_output=True, text=True)
    if result.returncode:
        print(result.stdout)
        print(result.stderr)
        raise SystemExit(result.returncode)
    source = root / ".esphome/build/clock-shadow-test/src"
    defines = (source / "lv_conf.h").read_text()
    assert "#define LV_DRAW_SW_SUPPORT_A8 1" in defines, \
        "Clock shadows need the alpha-only draw buffer, not just runtime style setters"
    generated = (source / "main.cpp").read_text()
    for label in labels + ["clock_label", "clock_date_label"]:
        assert f"lv_obj_add_style({label}, clock_bar_shadow," in generated, \
            f"{label} must receive the shared shadow style"
print("ESPHome enables alpha shadows for every clock-bar label and the screensaver clock/date.")
