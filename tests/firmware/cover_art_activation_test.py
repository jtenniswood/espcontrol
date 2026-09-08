#!/usr/bin/env python3
"""Execute the production activation YAML with ESPHome's real action chaining.

LVGL, network I/O and the clock are doubles. Script calls, restart/stop, delays,
conditions and the activation guard execute; they are not blanket no-ops.
The vendored Action/ActionList excerpt can be checked against a firmware build
with --esphome-source /path/to/generated/src/esphome.
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import yaml

ROOT = Path(__file__).resolve().parents[2]


class Loader(yaml.SafeLoader):
    pass


Loader.add_constructor("!lambda", lambda loader, node: loader.construct_scalar(node))


def generate(root):
    text = (root / "common/device/screen_cover_art.yaml").read_text()
    # Only script blocks are needed; other sections contain ESPHome-only tags.
    scripts = yaml.load(text[text.index("script:\n"):], Loader)["script"]
    scripts = {s["id"]: s for s in scripts}
    selected = {"display_mode_effect_cover_art", "cover_art_prepare_activation",
                "cover_art_request_artwork", "cover_art_use_cached_artwork",
                "cover_art_prepare_download", "cover_art_deferred_download", "cover_art_hide_effect"}
    declarations = set(scripts)
    setups = []
    serial = 0

    def cpp(value):
        return (str(value).replace("${cover_art_live_image_updates}", "false")
                .replace("${cover_art_decode_size}", "320"))

    def actions(items, owner):
        nonlocal serial
        output = []
        for item in items:
            serial += 1
            name = f"action_{serial}"
            kind, value = next(iter(item.items()))
            context = ""
            for index, parameter in enumerate(scripts[owner].get("parameters", {})):
                context += f"auto {parameter} = std::get<{index}>({owner}.args); (void){parameter}; "
            body = ""
            if kind == "delay":
                duration = ("1" if value == "1ms" else f"([]() {{ {cpp(value)} }})()")
                setups.append(f"auto *{name} = make_delay({owner}, {duration});")
                output.append(name)
                continue
            if kind == "if":
                # The effect's final widget visibility branches only exercise UI doubles.
                if owner == "display_mode_effect_cover_art" and "transition_is_current" not in str(value):
                    body = ""
                else:
                    cond = value["condition"]["lambda"]
                    yes = actions(value.get("then", []), owner)
                    no = actions(value.get("else", []), owner)
                    setups.append(f"auto *{name} = make_if({owner}, [&]() {{ {context}{cpp(cond)} }}, "
                                  f"{{{', '.join(yes)}}}, {{{', '.join(no)}}});")
                    output.append(name)
                    continue
            elif kind == "lambda":
                # Preserve the new scheduler call and generation guard in the effect.
                # Drawing/navigation bodies are I/O doubles; their action nodes remain.
                if owner != "display_mode_effect_cover_art" or any(
                    s in value for s in ("cover_art_prepare_activation", "transition_is_current")
                ):
                    body = cpp(value)
            elif kind == "script.execute":
                if isinstance(value, dict):
                    target = value["id"]
                    body = f"{target}.execute();"
                else:
                    target = value
                    body = f"{target}.execute();"
                declarations.add(target)
            elif kind == "script.stop":
                declarations.add(value)
                body = f"{value}.stop();"
            elif kind == "script.wait":
                declarations.add(value)
                setups.append(f"auto *{name} = make_wait({owner}, {value});")
                output.append(name)
                continue
            # Other LVGL/widget actions remain real chained action nodes with fake I/O.
            setups.append(f"auto *{name} = make_action({owner}, [&]() {{ {context}{body} }});")
            output.append(name)
        return output

    for name in sorted(selected):
        if name in scripts:
            body = scripts[name]["then"]
            if name == "cover_art_hide_effect":
                # Exercise the real guard/cancel prefix; remaining visual teardown is I/O.
                end = next((i + 1 for i, a in enumerate(body)
                            if a.get("script.stop") == "cover_art_prepare_activation"), 1)
                body = body[:end]
            nodes = actions(body, name)
            setups.append(f"{name}.actions.add_actions({{{', '.join(nodes)}}});")
    declarations.update(selected)
    prefix = "\n".join(f"Script {name};" for name in sorted(declarations))
    return prefix, "\n".join(setups)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--compiler", default="c++")
    parser.add_argument("--esphome-source", type=Path)
    parser.add_argument("--baseline", action="store_true")
    parser.add_argument("--mutations", action="store_true")
    args = parser.parse_args()
    if args.esphome_source:
        upstream = (args.esphome_source / "core/automation.h").read_text()
        start = upstream.index("template<typename... Ts> class ActionList;")
        end = upstream.index("template<typename... Ts> class Automation {", start)
        assert upstream[start:end] in (ROOT / "tests/firmware/stubs/esphome_automation_chain.h").read_text()
    declarations, setup = generate(args.root)
    harness = (ROOT / "tests/firmware/cover_art_activation_test.cpp").read_text()
    harness = harness.replace("// GENERATED_SCRIPT_DECLARATIONS", declarations)
    harness = harness.replace("// GENERATED_SCRIPT_SETUP", setup)
    with tempfile.TemporaryDirectory(prefix="cover-art-activation-") as temp:
        source = Path(temp) / "activation.cpp"
        binary = Path(temp) / "activation"
        source.write_text(harness)
        subprocess.run([args.compiler, "-std=c++17", "-O0", "-g", "-Wall", "-Wextra", "-Werror",
                        "-I", str(ROOT / "tests/firmware/stubs"), "-I", str(args.root / "components/espcontrol"),
                        "-I", str(args.root / "components/artwork_image"), str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary), "baseline" if args.baseline else "fixed"], check=True)
    if args.mutations:
        original = (args.root / "common/device/screen_cover_art.yaml").read_text()
        cases = {
            "effect-delay": ("display_mode_effect_cover_art", "      - delay: 1ms\n", ""),
            "artwork-delay": ("cover_art_prepare_activation", "      - delay: 1ms\n", ""),
            "transition-guard": ("cover_art_prepare_activation",
                "!id(espcontrol_app).display().transition_is_current(\n"
                "                  static_cast<uint32_t>(generation), espcontrol::DisplayMode::COVER_ART)", "false"),
            "subscription-guard": ("cover_art_prepare_activation",
                "subscription_generation != id(cover_art_subscription_generation)", "false"),
            "entity-guard": ("cover_art_prepare_activation",
                "cover_entity != id(cover_art_active_media_player_entity)", "false"),
            "stale-hide": ("cover_art_hide_effect", "    then:\n      - if:\n",
                "    then:\n      - script.stop: cover_art_prepare_activation\n      - if:\n"),
        }
        for label, (script, before, after) in cases.items():
            with tempfile.TemporaryDirectory(prefix=f"cover-art-mutation-{label}-") as temp:
                root = Path(temp)
                target = root / "common/device/screen_cover_art.yaml"
                target.parent.mkdir(parents=True)
                (root / "components").symlink_to(args.root / "components", target_is_directory=True)
                match = re.search(rf"  - id: {script}\n.*?(?=\n  - id: |\Z)", original, re.S)
                assert match and match[0].count(before) == 1, label
                target.write_text(original[:match.start()] + match[0].replace(before, after) + original[match.end():])
                result = subprocess.run([sys.executable, __file__,
                                         "--compiler", args.compiler, "--root", str(root)],
                                        capture_output=True, text=True)
                assert result.returncode != 0 and "Assertion" in result.stderr, (label, result.stderr)
                print(f"Detected mutation: {label}")


if __name__ == "__main__":
    main()
