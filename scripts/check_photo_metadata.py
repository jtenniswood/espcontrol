#!/usr/bin/env python3
"""Exercise bounded photo metadata and adaptive image overlay placement."""
from pathlib import Path
import subprocess
import tempfile
import textwrap

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix="photo-metadata-") as directory:
    binary = str(Path(directory) / "test")
    subprocess.run([
        "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "components/espcontrol"),
        str(ROOT / "tests/firmware/photo_metadata_test.cpp"), "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
    source = (ROOT / "common/device/screen_clock.yaml").read_text()
    start = source.index("          const bool active =")
    end = source.index("\ninterval:", start)
    (Path(directory) / "metadata_subscription.inc").write_text(textwrap.dedent(source[start:end]))
    display_source = (ROOT / "common/config/display.yaml").read_text()
    start = display_source.index("            if (!id(metadata_overlay_toggle_migrated))")
    end = display_source.index("            if (!id(media_player_sleep_prevention", start)
    (Path(directory) / "metadata_migration.inc").write_text(textwrap.dedent(display_source[start:end]))
    subprocess.run([
        "c++", "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "components/espcontrol"), "-I", directory,
        str(ROOT / "tests/firmware/photo_metadata_subscription_test.cpp"), "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
print("Photo metadata filtering and layout: ok")
