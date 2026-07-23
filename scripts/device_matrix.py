#!/usr/bin/env python3
"""Generate GitHub Actions device build matrices from devices/manifest.json."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

from device_profiles import (
    DEVICE_MANIFEST,
    ROOT,
    VALID_CHIP_FAMILIES,
    DeviceProfileError,
    load_device_profiles,
    load_manifest_data,
    validate_manifest_data,
)

class DeviceMatrixError(RuntimeError):
    pass


BUILD_FILE_SUFFIXES = ("yaml", "factory.yaml")
RECOVERY_CHIP_FAMILY = "ESP32-P4"


def load_manifest(path: Path = DEVICE_MANIFEST) -> dict[str, Any]:
    data = load_manifest_data(path)
    errors = validate_manifest_data(data)
    if errors:
        raise DeviceMatrixError("\n".join(errors))
    return data


def root_for_manifest(path: Path) -> Path:
    resolved = path.resolve()
    if resolved.name == "manifest.json" and resolved.parent.name == "devices":
        return resolved.parent.parent
    return ROOT


def validate_matrix_build_files(
    profiles: dict[str, dict[str, Any]],
    manifest_path: Path = DEVICE_MANIFEST,
) -> None:
    root = root_for_manifest(manifest_path)
    missing: list[str] = []
    for slug, profile in profiles.items():
        for suffix in BUILD_FILE_SUFFIXES:
            path = root / "builds" / f"{slug}.{suffix}"
            if not path.is_file():
                missing.append(str(path.relative_to(root)))
        chip = profile.get("firmware", {}).get("build", {}).get("chip")
        if chip == RECOVERY_CHIP_FAMILY:
            path = root / "builds" / f"{slug}.recovery.yaml"
            if not path.is_file():
                missing.append(str(path.relative_to(root)))
    if missing:
        raise DeviceMatrixError("missing firmware build input(s):\n" + "\n".join(missing))


def release_matrix(profiles: dict[str, dict[str, Any]]) -> dict[str, list[dict[str, str]]]:
    return {
        "include": [
            {
                "device": slug,
                "slug": slug,
                "chip": profile["firmware"]["build"]["chip"],
                "recovery": profile["firmware"]["build"]["chip"] == RECOVERY_CHIP_FAMILY,
            }
            for slug, profile in profiles.items()
        ]
    }


def nightly_matrix(profiles: dict[str, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    return {
        "include": [
            {
                "slug": slug,
                "recovery": profile["firmware"]["build"]["chip"] == RECOVERY_CHIP_FAMILY,
            }
            for slug, profile in profiles.items()
        ]
    }


def pr_matrix(profiles: dict[str, dict[str, Any]]) -> dict[str, list[dict[str, Any]]]:
    return nightly_matrix(profiles)


def write_json(data: Any) -> None:
    json.dump(data, sys.stdout, separators=(",", ":"))
    sys.stdout.write("\n")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEVICE_MANIFEST,
        help="Path to devices/manifest.json",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    release = sub.add_parser("release", help="Print the release workflow matrix JSON")
    release.set_defaults(matrix=release_matrix)

    nightly = sub.add_parser("nightly", help="Print the nightly workflow matrix JSON")
    nightly.set_defaults(matrix=nightly_matrix)

    pr = sub.add_parser("pr", help="Print the pull request firmware compile matrix JSON")
    pr.set_defaults(matrix=pr_matrix)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        profiles = load_device_profiles(args.manifest)
        validate_matrix_build_files(profiles, args.manifest)
        write_json(args.matrix(profiles))
    except (DeviceMatrixError, DeviceProfileError) as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
