"""Exercise each audited SDK version and reject the next unaudited patch."""

from pathlib import Path
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
compiler = sys.argv[1] if len(sys.argv) > 1 else "c++"
template = (HERE / "owned_ha_stubs/esphome/core/version.h.in").read_text()

with tempfile.TemporaryDirectory(prefix="owned-ha-sdk-guard-") as temporary:
    directory = Path(temporary)
    for patch in (1, 2, 3):
        include = directory / str(patch)
        header = include / "esphome/core/version.h"
        header.parent.mkdir(parents=True)
        header.write_text(template.replace("@ESPCONTROL_SDK_MAJOR@", "2026")
                          .replace("@ESPCONTROL_SDK_MINOR@", "10")
                          .replace("@ESPCONTROL_SDK_PATCH@", str(patch)))
        binary = directory / f"transport-{patch}"
        command = [compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                   "-fno-rtti", "-UNDEBUG", "-DUSE_API_HOMEASSISTANT_STATES",
                   "-I", str(include), "-I", str(HERE / "owned_ha_stubs"),
                   "-I", str(HERE / "stubs"),
                   "-I", str(ROOT / "components/espcontrol"),
                   str(HERE / "home_assistant_transport_runtime_test.cpp"),
                   str(ROOT / "components/espcontrol/home_assistant_transport_runtime.cpp"),
                   "-o", str(binary)]
        result = subprocess.run(command, capture_output=True, text=True)
        source_position = command.index(str(HERE / "home_assistant_transport_runtime_test.cpp"))
        dispatch_command = command[:source_position] + [
            "-fsyntax-only", str(ROOT / "components/espcontrol/native_ha_state_dispatch.cpp")]
        dispatch_result = subprocess.run(dispatch_command, capture_output=True, text=True)
        if patch == 3:
            assert result.returncode != 0, "Unaudited SDK version must fail compilation"
            assert "Audit owned HA subscription removal" in result.stderr, result.stderr
            assert dispatch_result.returncode != 0, "Unaudited native dispatch must fail compilation"
            assert "Audit native HA state dispatch" in dispatch_result.stderr, dispatch_result.stderr
        else:
            assert result.returncode == 0, f"Audited SDK 2026.10.{patch}:\n{result.stderr}"
            assert dispatch_result.returncode == 0, dispatch_result.stderr
            subprocess.run([str(binary)], check=True)

print("PASS audited 2026.10.1/2 transport behaviour and unaudited-version rejection")
