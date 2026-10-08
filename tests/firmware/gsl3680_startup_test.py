"""Exercise the production GSL3680 driver with a simulated register bus."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
COMPONENT = Path(os.environ.get("GSL3680_SOURCE", ROOT / "components/gsl3680"))

with tempfile.TemporaryDirectory(prefix="gsl3680-startup-") as temp:
    directory = Path(temp)
    # Supply just ESPHome's hardware boundary; retain the production class,
    # methods and firmware table so the test exercises the actual bus protocol.
    header = (COMPONENT / "gsl3680.h").read_text()
    header = "\n".join(line for line in header.splitlines()
                       if not line.startswith('#include "esphome/'))
    (directory / "gsl3680_under_test.h").write_text(header)
    source = (COMPONENT / "gsl3680.cpp").read_text().replace(
        '#include "gsl3680.h"', '#include "gsl3680_under_test.h"', 1)
    (directory / "gsl3680_under_test.cpp").write_text(source)
    executable = directory / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-Wno-missing-field-initializers", "-I", str(directory),
        "-I", str(COMPONENT),
        str(ROOT / "tests/firmware/gsl3680_startup_test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
