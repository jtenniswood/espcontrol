#!/usr/bin/env python3
"""Run local ESPHome commands with the EspControl dev firmware version."""

from __future__ import annotations

import os
import shlex
import subprocess
import sys
import tempfile
import unittest
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]


class LocalEsphomeError(RuntimeError):
    pass


def pinned_esphome_version() -> str:
    env_path = ROOT / ".github" / "esphome.env"
    try:
        lines = env_path.read_text(encoding="utf-8").splitlines()
    except OSError as exc:
        raise LocalEsphomeError(f"Could not read {env_path}: {exc}") from exc

    versions = [
        line.removeprefix("ESPHOME_VERSION=")
        for line in lines
        if line.startswith("ESPHOME_VERSION=")
    ]
    if len(versions) != 1 or not versions[0]:
        raise LocalEsphomeError(f"{env_path} must declare exactly one ESPHOME_VERSION")
    return versions[0]


def installed_esphome_version(esphome_bin: str) -> str:
    try:
        result = subprocess.run(
            [esphome_bin, "version"],
            capture_output=True,
            check=False,
            text=True,
        )
    except OSError as exc:
        raise LocalEsphomeError(f"Could not run ESPHome at {esphome_bin}: {exc}") from exc

    output = f"{result.stdout}\n{result.stderr}"
    for line in output.splitlines():
        if line.startswith("Version:"):
            return line.removeprefix("Version:").strip()
    raise LocalEsphomeError(f"Could not determine the ESPHome version from {esphome_bin}")


def ensure_pinned_esphome(esphome_bin: str) -> None:
    expected = pinned_esphome_version()
    installed = installed_esphome_version(esphome_bin)
    if installed != expected:
        raise LocalEsphomeError(
            f"ESPHome {expected} is required by .github/esphome.env, but {esphome_bin} is {installed}. "
            "Install or select the pinned version before compiling or flashing."
        )


def resolve_yaml_path(yaml_arg: str, cwd: Path | None = None) -> Path:
    base = cwd or Path.cwd()
    path = Path(yaml_arg)
    if not path.is_absolute():
        path = base / path
    resolved = path.resolve()
    if not resolved.is_file():
        raise LocalEsphomeError(f"ESPHome YAML not found: {yaml_arg}")
    return resolved


def local_version_for(_yaml_path: Path) -> str:
    return "dev"


def build_esphome_command(
    yaml_path: Path,
    command: str,
    command_args: list[str],
    firmware_version: str,
    esphome_bin: str,
) -> list[str]:
    return [
        esphome_bin,
        "-s",
        "firmware_version",
        firmware_version,
        "-s",
        "espcontrol_component_url",
        ROOT.resolve().as_uri(),
        command,
        str(yaml_path),
        *command_args,
    ]


def resolve_esphome_bin(esphome_bin: str, cwd: Path | None = None) -> str:
    if os.sep not in esphome_bin and (not os.altsep or os.altsep not in esphome_bin):
        return esphome_bin

    path = Path(esphome_bin).expanduser()
    if path.is_absolute():
        return str(path)
    return str(((cwd or Path.cwd()) / path).resolve())


def esphome_working_dir(yaml_path: Path) -> Path:
    return yaml_path.parent


def parse_args(argv: list[str]) -> tuple[bool, Path, str, list[str]]:
    dry_run = False
    filtered_args: list[str] = []
    for arg in argv:
        if arg == "--dry-run":
            dry_run = True
        else:
            filtered_args.append(arg)

    if len(filtered_args) < 2:
        raise LocalEsphomeError(
            "Usage: python3 scripts/local_esphome.py <devices/<slug>/dev.yaml> <esphome-command> [args...]\n"
            "Example: python3 scripts/local_esphome.py devices/esp32-p4-86/dev.yaml run --device 192.168.x.x"
        )

    yaml_path = resolve_yaml_path(filtered_args[0])
    command = filtered_args[1]
    command_args = filtered_args[2:]
    return dry_run, yaml_path, command, command_args


def run(argv: list[str]) -> int:
    dry_run, yaml_path, command, command_args = parse_args(argv)
    version = local_version_for(yaml_path)
    esphome_bin = resolve_esphome_bin(os.environ.get("ESPHOME_BIN", "esphome"))

    if dry_run:
        esphome_command = build_esphome_command(yaml_path, command, command_args, version, esphome_bin)
        print(shlex.join(esphome_command))
        return 0

    ensure_pinned_esphome(esphome_bin)
    esphome_command = build_esphome_command(yaml_path, command, command_args, version, esphome_bin)
    return subprocess.run(esphome_command, cwd=esphome_working_dir(yaml_path)).returncode


class LocalEsphomeTests(unittest.TestCase):
    def test_reads_the_repository_pinned_esphome_version(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            root = Path(temporary_directory)
            env_path = root / ".github" / "esphome.env"
            env_path.parent.mkdir()
            env_path.write_text("ESPHOME_VERSION=test-version\n", encoding="utf-8")

            with mock.patch(__name__ + ".ROOT", root):
                self.assertEqual(pinned_esphome_version(), "test-version")

    def test_reads_installed_esphome_version(self) -> None:
        with mock.patch(__name__ + ".subprocess.run") as run_mock:
            run_mock.return_value = subprocess.CompletedProcess(
                args=["esphome", "version"],
                returncode=0,
                stdout="Version: 2026.8.1\n",
                stderr="",
            )
            self.assertEqual(installed_esphome_version("esphome"), "2026.8.1")

    def test_rejects_an_unpinned_esphome_version(self) -> None:
        with mock.patch(__name__ + ".pinned_esphome_version", return_value="2026.8.1"), \
            mock.patch(__name__ + ".installed_esphome_version", return_value="2026.8.0"):
            with self.assertRaisesRegex(LocalEsphomeError, "2026.8.1.*2026.8.0"):
                ensure_pinned_esphome("esphome")

    def test_uses_dev_firmware_version(self) -> None:
        path = ROOT / "devices" / "esp32-p4-86" / "dev.yaml"
        self.assertEqual(local_version_for(path), "dev")

    def test_builds_esphome_command_with_firmware_version_substitution(self) -> None:
        command = build_esphome_command(
            Path("/tmp/dev.yaml"),
            "run",
            ["--device", "192.168.1.10"],
            "dev",
            "esphome",
        )
        self.assertEqual(
            command,
            [
                "esphome",
                "-s",
                "firmware_version",
                "dev",
                "-s",
                "espcontrol_component_url",
                ROOT.resolve().as_uri(),
                "run",
                "/tmp/dev.yaml",
                "--device",
                "192.168.1.10",
            ],
        )

    def test_keeps_esphome_command_name_for_path_lookup(self) -> None:
        self.assertEqual(resolve_esphome_bin("esphome"), "esphome")

    def test_resolves_relative_esphome_bin_before_changing_directory(self) -> None:
        self.assertEqual(
            resolve_esphome_bin(".venv/bin/esphome", cwd=ROOT),
            str(ROOT / ".venv" / "bin" / "esphome"),
        )

    def test_runs_esphome_from_yaml_directory(self) -> None:
        path = ROOT / "devices" / "esp32-p4-86" / "dev.yaml"
        self.assertEqual(esphome_working_dir(path), ROOT / "devices" / "esp32-p4-86")

    def test_invokes_esphome_from_yaml_directory(self) -> None:
        path = ROOT / "devices" / "esp32-p4-86" / "dev.yaml"
        with mock.patch(__name__ + ".parse_args", return_value=(False, path, "run", [])), \
            mock.patch(__name__ + ".local_version_for", return_value="local-version"), \
            mock.patch(__name__ + ".ensure_pinned_esphome"), \
            mock.patch(__name__ + ".subprocess.run") as run_mock:
            run_mock.return_value.returncode = 0
            self.assertEqual(run(["devices/esp32-p4-86/dev.yaml", "run"]), 0)
            self.assertEqual(run_mock.call_args.kwargs["cwd"], path.parent)

    def test_dry_run_prints_command_without_invoking_esphome(self) -> None:
        path = ROOT / "devices" / "esp32-p4-86" / "dev.yaml"
        output = StringIO()
        with mock.patch(__name__ + ".local_version_for", return_value="local-version"), \
            mock.patch(__name__ + ".subprocess.run") as run_mock, \
            redirect_stdout(output):
            self.assertEqual(run(["--dry-run", str(path), "run", "--device", "192.0.2.10"]), 0)
            run_mock.assert_not_called()
        self.assertIn("-s firmware_version local-version", output.getvalue())
        self.assertIn(f"-s espcontrol_component_url {ROOT.resolve().as_uri()}", output.getvalue())
        self.assertIn("--device 192.0.2.10", output.getvalue())


def self_test() -> int:
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(LocalEsphomeTests)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    return 0 if result.wasSuccessful() else 1


def main(argv: list[str]) -> int:
    if argv == ["--self-test"]:
        return self_test()

    try:
        return run(argv)
    except LocalEsphomeError as exc:
        print(f"local_esphome.py: {exc}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"local_esphome.py: command failed: {shlex.join(exc.cmd)}", file=sys.stderr)
        return exc.returncode


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
