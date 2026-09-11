"""Run the production camera picture handler and maintenance loop with simulated I/O."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/espcontrol/button_grid_image.h").read_text()
functions = []
for name in ("image_card_handle_picture", "image_card_refresh_due"):
    match = re.search(rf"^inline void {name}\([^;{{]*\) \{{\n.*?^\}}", source, re.M | re.S)
    if match is None:
        raise AssertionError(f"Missing production function: {name}")
    functions.append(match.group())
with tempfile.TemporaryDirectory(prefix="camera-refresh-runtime-") as temp:
    directory = Path(temp)
    (directory / "camera_refresh_runtime_functions.h").write_text("\n\n".join(functions))
    executable = directory / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(directory),
        "-I", str(ROOT / "components/espcontrol"),
        str(ROOT / "tests/firmware/camera_refresh_runtime_test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
