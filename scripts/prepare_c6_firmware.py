#!/usr/bin/env python3
"""Download and verify the ESP32-C6 recovery firmware build dependency."""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import tempfile
import urllib.request


ROOT = Path(__file__).resolve().parent.parent
C6_VERSION = "2.12.9"
C6_SHA256 = "9d8cec9dcca632e9ded31de555a5b49c70316d30b87ae06bd933fd35a5a11513"
C6_URL = (
    "https://esphome.github.io/esp-hosted-firmware/"
    f"v{C6_VERSION}/network_adapter_esp32c6.bin"
)
C6_RELATIVE_PATH = Path(
    f".firmware-deps/esp32-c6/{C6_VERSION}/network_adapter_esp32c6.bin"
)
C6_PATH = ROOT / C6_RELATIVE_PATH


class C6FirmwareError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def verify(path: Path) -> None:
    if not path.is_file():
        raise C6FirmwareError(f"C6 firmware not found: {path}")
    actual = sha256(path)
    if actual != C6_SHA256:
        raise C6FirmwareError(
            f"C6 firmware checksum mismatch: expected {C6_SHA256}, received {actual}"
        )


def download(url: str, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(
        prefix=f".{destination.name}.", suffix=".tmp", dir=destination.parent
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(fd, "wb") as output:
            request = urllib.request.Request(
                url, headers={"User-Agent": "espcontrol-c6-recovery-build"}
            )
            with urllib.request.urlopen(request, timeout=60) as response:
                while chunk := response.read(1024 * 1024):
                    output.write(chunk)
            output.flush()
            os.fsync(output.fileno())
        verify(temporary)
        temporary.replace(destination)
    except Exception:
        temporary.unlink(missing_ok=True)
        raise


def prepare(path: Path = C6_PATH, url: str = C6_URL) -> Path:
    if path.is_file():
        try:
            verify(path)
            print(f"Using verified ESP32-C6 firmware {C6_VERSION}: {path}")
            return path
        except C6FirmwareError:
            path.unlink()

    try:
        download(url, path)
    except Exception as exc:
        raise C6FirmwareError(f"Could not prepare ESP32-C6 firmware: {exc}") from exc
    print(f"Downloaded and verified ESP32-C6 firmware {C6_VERSION}: {path}")
    return path


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check",
        action="store_true",
        help="Only verify the cached dependency; do not download it.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=C6_PATH,
        help="Override the dependency output path (primarily for tests).",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.check:
            verify(args.output)
            print(f"Verified ESP32-C6 firmware {C6_VERSION}: {args.output}")
        else:
            prepare(args.output)
    except C6FirmwareError as exc:
        print(f"::error::{exc}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
