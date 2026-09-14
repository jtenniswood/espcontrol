#!/usr/bin/env python3
"""Check deferred presence screensaver scheduling and latest-state behavior."""
from pathlib import Path
import yaml

ROOT = Path(__file__).resolve().parents[2]


class Loader(yaml.SafeLoader):
    pass


Loader.add_constructor("!lambda", lambda loader, node: loader.construct_scalar(node))


def load_scripts(path):
    text = path.read_text()
    return {item["id"]: item for item in yaml.load(text[text.index("script:\n"):], Loader)["script"]}


def main():
    scripts = load_scripts(ROOT / "common/addon/backlight.yaml")
    assert scripts["screensaver_presence_wake"]["then"] == [{"script.execute": "screensaver_presence_update"}]
    assert scripts["screensaver_presence_sleep"]["then"] == [{"script.execute": "screensaver_presence_update"}]
    update = scripts["screensaver_presence_update"]["then"]
    assert update[0] == {"delay": "1ms"}
    assert all(name in str(update) for name in ("screensaver_mode", "presence_detected", "screensaver_wake", "screensaver_sleep_sensor"))

    pending, detected, mode, applied = False, False, "sensor", []
    def callback(value):
        nonlocal pending, detected
        detected, pending = value, True
    def loop_pass():
        nonlocal pending
        if pending:
            pending = False
            if mode == "sensor":
                applied.append("wake" if detected else "sleep")
    callback(False); callback(True)
    assert applied == []
    loop_pass(); assert applied == ["wake"]
    callback(True); callback(False); loop_pass()
    assert applied == ["wake", "sleep"]
    mode = "timer"; callback(False); loop_pass()
    assert applied == ["wake", "sleep"]


if __name__ == "__main__":
    main()
