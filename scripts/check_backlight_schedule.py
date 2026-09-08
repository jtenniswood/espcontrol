#!/usr/bin/env python3
"""Check backlight startup persistence and interrupted-fade recovery."""

from pathlib import Path
import subprocess
import tempfile
import textwrap


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "common" / "addon" / "backlight_schedule.yaml"
FADE_SOURCE = ROOT / "common" / "addon" / "backlight.yaml"


def check_recovery(source: str) -> None:
    # Exercise the actual YAML write/skip logic with the real display controller
    # and fade sampler, while replacing ESPHome's light with a host test double.
    script = source.index("  - id: backlight_apply_brightness")
    start = source.index("          float target = pct / 100.0f;", script)
    end = source.index("\n\n", start)
    fade_source = FADE_SOURCE.read_text(encoding="utf-8")
    fade_script = fade_source.index("  - id: backlight_fade_current_ui_to_black")
    fade_start = fade_source.index("          float brightness =", fade_script)
    fade_end = fade_source.index("      - while:", fade_start)
    with tempfile.TemporaryDirectory(prefix="backlight-recovery-") as directory:
        output = Path(directory)
        (output / "backlight_brightness_adapter.inc").write_text(
            textwrap.dedent(source[start:end]), encoding="utf-8"
        )
        (output / "backlight_fade_start.inc").write_text(
            textwrap.dedent(fade_source[fade_start:fade_end]), encoding="utf-8"
        )
        binary = output / "backlight_recovery_test"
        subprocess.run([
            "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
            "-I", str(ROOT / "components" / "espcontrol"), "-I", str(output),
            str(ROOT / "tests" / "firmware" / "backlight_recovery_test.cpp"),
            "-o", str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)
    print("backlight interrupted-fade recovery: ok")


def check_clock_switches_without_fade() -> None:
    source = FADE_SOURCE.read_text(encoding="utf-8")
    start = source.index("  - id: clock_screensaver_refresh_brightness")
    end = source.index("\n  # Hide the clock screensaver overlay", start)
    clock_view = source[start:end]
    assert "backlight_fade_current_ui_to_black" not in clock_view, (
        "clock screensaver must not fade the active UI to black"
    )
    assert "screensaver_fade" not in clock_view, (
        "clock screensaver must not run a brightness fade"
    )
    assert "transition_length: 400ms" not in clock_view, (
        "clock screensaver brightness refresh must not fade"
    )
    assert clock_view.count("transition_length: 0s") == 2, (
        "all clock screensaver brightness writes must be immediate"
    )
    direct_brightness = clock_view.index("espcontrol::apply_backlight_fade_level(")
    reveal_overlay = clock_view.index(
        "lv_obj_clear_flag(id(clock_screensaver), LV_OBJ_FLAG_HIDDEN);"
    )
    assert direct_brightness < reveal_overlay, (
        "clock brightness must reach the PWM before the overlay is revealed"
    )
    print("clock screensaver direct transition: ok")


def main() -> None:
    text = SOURCE.read_text(encoding="utf-8")
    handler = text.index("  - id: display_backlight_handle_state")
    handler_end = text.index("\n  # ---------------------------------------------------------------------------", handler)
    handler_text = text[handler:handler_end]

    guard = "if (!App.is_setup_complete() || !id(brightness_mode_runtime_ready)) {"
    assert guard in handler_text, "backlight state handler lacks the startup guard"
    assert "id(backlight_expected_internal_level_valid) = false;" in handler_text, (
        "startup guard must clear pending internal brightness state"
    )
    assert handler_text.index(guard) < handler_text.index(
        "if (!id(display_backlight).remote_values.is_on()) return;"
    ), "startup guard must run before restored light-state handling"

    boot = text.index("priority: -190")
    boot_end = text.index("\n      - lambda: |-", text.index("id(brightness_mode_runtime_ready) = true;", boot)) + 1
    boot_text = text[boot:boot_end]
    assert "id(brightness_mode_runtime_ready) = true;" in boot_text, (
        "brightness mode must become runtime-ready during boot initialization"
    )

    assert "mode_call.set_option(\"Manual\");" in handler_text, (
        "runtime external brightness changes must still select Manual mode"
    )

    print("backlight schedule startup guard: ok")
    check_recovery(text)
    check_clock_switches_without_fade()


if __name__ == "__main__":
    main()
