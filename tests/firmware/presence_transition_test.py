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


def evaluate_condition(condition, state):
    expression = condition["lambda"]
    if "screensaver_mode" in expression:
        return state["mode"] == "sensor"
    if "presence_detected" in expression:
        return state["presence_detected"]
    if "presence_can_wake_display" in expression:
        return state["can_wake"]
    raise AssertionError(f"unhandled presence transition condition: {expression}")


def execute_actions(actions, state, applied):
    for action in actions:
        if "delay" in action:
            continue
        if "globals.set" in action:
            continue
        if "lambda" in action:
            continue
        if "if" in action:
            branch = action["if"]
            selected = branch["then"] if evaluate_condition(branch["condition"], state) else branch.get("else", [])
            execute_actions(selected, state, applied)
            continue
        target = action.get("script.execute")
        if target == "screensaver_wake":
            applied.append("wake")
        elif target == "screensaver_sleep_sensor":
            applied.append("sleep")
        elif target == "display_mode_clear_automatic":
            applied.append("clear")
        else:
            raise AssertionError(f"unhandled presence transition action: {action}")


def main():
    scripts = load_scripts(ROOT / "common/addon/backlight.yaml")
    for script_id in (
        "screensaver_presence_wake",
        "screensaver_presence_sleep",
        "screensaver_presence_update",
    ):
        assert scripts[script_id]["mode"] == "restart"
    assert scripts["screensaver_presence_wake"]["then"] == [{"script.execute": "screensaver_presence_update"}]
    assert scripts["screensaver_presence_sleep"]["then"] == [{"script.execute": "screensaver_presence_update"}]
    update = scripts["screensaver_presence_update"]["then"]
    assert update[0] == {"delay": "1ms"}
    state = {"mode": "sensor", "presence_detected": False, "can_wake": True}
    pending, applied = [], []

    def callback(script_id, value):
        state["presence_detected"] = value
        target = scripts[script_id]["then"][0]["script.execute"]
        if scripts[target]["mode"] == "restart":
            pending[:] = [target]
        else:
            pending.append(target)

    def loop_pass():
        queued = pending[:]
        pending.clear()
        for script_id in queued:
            execute_actions(scripts[script_id]["then"], state, applied)

    callback("screensaver_presence_sleep", False)
    callback("screensaver_presence_wake", True)
    assert applied == []
    loop_pass()
    assert applied == ["wake"]

    callback("screensaver_presence_wake", True)
    callback("screensaver_presence_sleep", False)
    loop_pass()
    assert applied == ["wake", "sleep"]

    state["mode"] = "timer"
    callback("screensaver_presence_sleep", False)
    loop_pass()
    assert applied == ["wake", "sleep"]


if __name__ == "__main__":
    main()
