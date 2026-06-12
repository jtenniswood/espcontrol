#!/usr/bin/env python3
"""Decide whether changed files require pull request firmware compiles."""

from __future__ import annotations

import argparse
from pathlib import Path
import sys


EXACT_PATHS = {
    ".github/workflows/firmware-compile.yml",
    ".github/workflows/nightly-firmware.yml",
    ".github/workflows/release.yml",
    "devices/manifest.json",
    "package.json",
    "package-lock.json",
    "scripts/build.py",
    "scripts/device_matrix.py",
    "scripts/device_profiles.py",
    "scripts/firmware_compile_required.py",
    "scripts/generate_device_slots.py",
}

PREFIXES = (
    "common/assets/",
    "common/config/",
    "docs/public/webserver/",
    "src/webserver/",
)

FIRMWARE_COMPONENT_SUFFIXES = (
    ".cpp",
    ".h",
    ".json",
    ".py",
    ".txt",
    ".yaml",
    ".yml",
)


def normalize_path(path: str) -> str:
    normalized = path.strip().replace("\\", "/")
    while normalized.startswith("./"):
        normalized = normalized[2:]
    return normalized


def compile_required_for_path(path: str) -> bool:
    path = normalize_path(path)
    if not path:
        return False
    if path in EXACT_PATHS:
        return True
    if path.startswith(PREFIXES):
        return True
    if path.startswith("builds/") and path.endswith((".yaml", ".yml")):
        return True
    if path.startswith("components/") and path.endswith(FIRMWARE_COMPONENT_SUFFIXES):
        return True
    if (
        path.startswith("devices/")
        and path.endswith((".yaml", ".yml"))
        and (
            "/device/" in path
            or path.endswith("/esphome.yaml")
            or path.endswith("/packages.yaml")
            or path.endswith("/dev.yaml")
        )
    ):
        return True
    return False


def compile_required(paths: list[str]) -> bool:
    return any(compile_required_for_path(path) for path in paths)


def read_paths(args: argparse.Namespace) -> list[str]:
    paths: list[str] = []
    if args.files:
        paths.extend(args.files)
    if args.path_file:
        paths.extend(args.path_file.read_text(encoding="utf-8").splitlines())
    if args.stdin:
        paths.extend(sys.stdin.read().splitlines())
    return paths


def run_self_test() -> None:
    required = [
        ".github/workflows/firmware-compile.yml",
        "common/assets/icon_glyphs.yaml",
        "common/config/card_contract.json",
        "components/gsl3680/touchscreen.py",
        "components/espcontrol/button_grid.h",
        "devices/manifest.json",
        "devices/guition-esp32-s3-4848s040/device/fonts.yaml",
        "devices/guition-esp32-s3-4848s040/esphome.yaml",
        "devices/guition-esp32-s3-4848s040/packages.yaml",
        "builds/guition-esp32-s3-4848s040.factory.yaml",
        "src/webserver/modules/app.js",
        "docs/public/webserver/guition-esp32-s3-4848s040/www.js",
        "scripts/build.py",
        "package-lock.json",
    ]
    skipped = [
        "README.md",
        "dev-docs/checks-and-releases.md",
        "devices/guition-esp32-s3-4848s040/README.md",
        "components/gsl3680/README.md",
        "components/gsl3680/LICENSE.md",
        ".github/pull_request_template.md",
        "compatibility/README.md",
    ]
    for path in required:
        assert compile_required_for_path(path), f"{path} should require firmware compile"
    for path in skipped:
        assert not compile_required_for_path(path), f"{path} should not require firmware compile"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("files", nargs="*", help="Changed paths to check")
    parser.add_argument("--path-file", type=Path, help="Read changed paths from a newline-delimited file")
    parser.add_argument("--stdin", action="store_true", help="Read changed paths from stdin")
    parser.add_argument("--github-output", action="store_true", help="Print compile_required=<true|false>")
    parser.add_argument("--self-test", action="store_true", help="Run built-in path matching tests")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.self_test:
        run_self_test()
        print("Firmware compile path checks passed.")
        return 0

    value = "true" if compile_required(read_paths(args)) else "false"
    if args.github_output:
        print(f"compile_required={value}")
    else:
        print(value)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
