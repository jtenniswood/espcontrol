#!/usr/bin/env python3
"""Report native ESP-IDF application size against the compiled OTA partitions.

Run after ESPHome compile (including failures), before deleting its build cache.
The repository pins ESPHome's native ESP-IDF backend for every released target.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import struct

PARTITION = struct.Struct("<HBBII16sI")
WARNING_BYTES = 128 * 1024
HEADER = "## Firmware size and headroom\n\n"
COLUMNS = "| Target | Application bytes | OTA partition bytes | Used | Headroom | Build |\n| --- | ---: | ---: | ---: | ---: | --- |\n"


def ota_capacity(table: bytes) -> int:
    sizes = []
    for offset in range(0, len(table), PARTITION.size):
        entry = table[offset:offset + PARTITION.size]
        if len(entry) != PARTITION.size:
            raise ValueError("Truncated partition table")
        magic, kind, subtype, _, size, _, _ = PARTITION.unpack(entry)
        if magic in (0xEBEB, 0xFFFF):  # MD5 trailer or erased padding
            break
        if magic != 0x50AA:
            raise ValueError("Invalid partition table entry")
        if kind == 0 and 0x10 <= subtype <= 0x1F:
            sizes.append(size)
    if not sizes or min(sizes) <= 0:
        raise ValueError("No usable OTA application partition found")
    # An update must fit either OTA slot, even if their sizes differ.
    return min(sizes)


def measure(build_root: Path) -> tuple[int, int]:
    descriptions = list(build_root.rglob("project_description.json"))
    # Bootloader projects have no application partition table.
    descriptions = [p for p in descriptions if (p.parent / "partition_table/partition-table.bin").is_file()]
    if len(descriptions) != 1:
        raise ValueError(f"Expected one application build, found {len(descriptions)}")
    description = descriptions[0]
    project = json.loads(description.read_text())
    # ESP-IDF paths may refer to /config inside a now-stopped Docker container.
    app = description.parent / Path(project["app_bin"]).name
    used = app.stat().st_size
    if used <= 0:
        raise ValueError("Application binary is empty")
    capacity = ota_capacity((description.parent / "partition_table/partition-table.bin").read_bytes())
    return used, capacity


def report(target: str, build_root: Path, compile_status: int, summary: Path | None) -> int:
    status = "passed" if compile_status == 0 else "failed"
    result = int(compile_status != 0)
    try:
        used, capacity = measure(build_root)
        free = capacity - used
        print(f"{target}: application {used:,} bytes / {capacity:,} bytes "
              f"({100 * used / capacity:.2f}%); headroom {free:,} bytes ({free / 1024:.1f} KiB); build {status}")
        if free < 0:
            print(f"::error::{target}: application exceeds OTA partition by {-free:,} bytes")
            result = 1
            status = "failed (oversize)"
        elif free < WARNING_BYTES:
            print(f"::warning::{target}: only {free / 1024:.1f} KiB application headroom (warning below 128 KiB)")
        row = f"| {target} | {used:,} | {capacity:,} | {100 * used / capacity:.2f}% | {free / 1024:.1f} KiB | {status} |\n"
    except (OSError, ValueError, KeyError, TypeError) as exc:
        print(f"::error::{target}: firmware size unavailable: {exc}")
        row = f"| {target} | unavailable | unavailable | — | — | failed (size unavailable; compile {status}) |\n"
        result = 1
    if summary:
        existing = summary.read_text() if summary.exists() else ""
        with summary.open("a") as handle:
            if HEADER not in existing:
                handle.write(HEADER + COLUMNS)
            handle.write(row)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--compile-status", type=int, required=True)
    parser.add_argument("--summary", type=Path, default=os.environ.get("GITHUB_STEP_SUMMARY"))
    args = parser.parse_args()
    return report(args.target, args.build_root, args.compile_status, args.summary)


if __name__ == "__main__":
    raise SystemExit(main())
