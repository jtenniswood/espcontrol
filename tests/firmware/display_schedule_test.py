#!/usr/bin/env python3
"""Run production schedule/reconcile/ACTIVE YAML against a virtual clock.

The controller, schedule predicates, lambdas and action ordering are production
code. Hardware, rendering and unrelated scripts are doubles. The small script
runner models restart, delay and wait so both periodic callback orders execute.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile
import yaml

ROOT = Path(__file__).resolve().parents[2]

class Loader(yaml.SafeLoader):
    pass

Loader.add_constructor("!lambda", lambda loader, node: loader.construct_scalar(node))

def generate(root):
    scripts = {}
    for filename in ("backlight.yaml", "backlight_schedule.yaml"):
        text = (root / "common/addon" / filename).read_text()
        scripts.update({s["id"]: s for s in yaml.load(text[text.index("script:\n"):], Loader)["script"]})
    selected = {"display_mode_reconcile", "screen_schedule_check", "screen_schedule_wake",
                "display_mode_apply_transition", "display_mode_effect_active",
                "display_active_finalize", "screensaver_idle_check"}
    # Exercise the entire ACTIVE branch, including the guarded completion and
    # finalizer call. Other presentation modes are hardware doubles below.
    adapter = scripts["display_mode_apply_transition"]["then"][0]["if"]["then"]
    active = next(a for a in adapter if "if" in a and
                  "target_mode == static_cast<int>(espcontrol::DisplayMode::ACTIVE)" in
                  str(a["if"]["condition"]))
    active["if"].pop("else", None)
    scripts["display_mode_apply_transition"]["then"] = [active]
    declarations = set(selected)

    def actions(items, owner):
        result = []
        context = "".join(f"int {p} = {owner}.args[{i}]; (void){p}; "
                          for i, p in enumerate(scripts[owner].get("parameters", {})))
        for item in items:
            kind, value = next(iter(item.items()))
            if kind == "lambda":
                result.append(f"call([&] {{ {context}{value} }})")
            elif kind == "if":
                yes = actions(value.get("then", []), owner)
                no = actions(value.get("else", []), owner)
                result.append(f"branch({owner}, [&] {{ {context}{value['condition']['lambda']} }}, {yes}, {no})")
            elif kind == "delay":
                if isinstance(value, str) and value.endswith("ms"):
                    duration = value[:-2]
                else:
                    duration = f"([] {{ {value} }})()"
                result.append(f"pause([&] {{ return {duration}; }})")
            elif kind in ("script.execute", "script.wait", "script.stop"):
                target = value["id"] if isinstance(value, dict) else value
                declarations.add(target)
                if kind == "script.wait":
                    result.append(f"wait_for({target})")
                else:
                    args = []
                    if isinstance(value, dict):
                        args = [f"([&] {{ {expr} }})()" for key, expr in value.items() if key != "id"]
                    method = "execute" if kind == "script.execute" else "stop"
                    result.append(f"call([&] {{ {context}{target}.{method}({', '.join(args)}); }})")
            elif kind == "globals.set":
                result.append(f"call([&] {{ {value['id']} = {value['value']}; }})")
            elif kind.startswith("lvgl."):
                result.append("call([] {})")  # Render I/O only.
            else:
                raise ValueError(f"Unsupported action {owner}: {kind}")
        return "Steps{" + ",\n".join(result) + "}"

    setups = [f"{name}.steps = {actions(scripts[name]['then'], name)};" for name in sorted(selected)]
    helpers = (root / "components/espcontrol/backlight.h").read_text()
    helpers = helpers[helpers.index("inline bool screen_schedule_in_window"):helpers.index("inline bool screen_schedule_blocks_cover_art")]
    return ("\n".join(f"Script {s};" for s in sorted(declarations)), "\n".join(setups), helpers)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="c++")
    args = parser.parse_args()
    declarations, setup, helpers = generate(args.root)
    source = (ROOT / "tests/firmware/display_schedule_test.cpp.in").read_text()
    source = source.replace("// SCRIPT_DECLARATIONS", declarations).replace("// SCRIPT_SETUP", setup).replace("// SCHEDULE_HELPERS", helpers)
    with tempfile.TemporaryDirectory() as tmp:
        cpp, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
        cpp.write_text(source)
        subprocess.run([args.compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(args.root / "components/espcontrol"), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)

if __name__ == "__main__":
    main()
