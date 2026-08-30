#!/usr/bin/env python3
"""Check the startup boundary for backlight mode persistence."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "common" / "addon" / "backlight_schedule.yaml"


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


if __name__ == "__main__":
    main()
