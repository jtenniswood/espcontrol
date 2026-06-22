#!/usr/bin/env python3
"""Validate timezone selector options against firmware rules."""
from __future__ import annotations

from datetime import datetime, timedelta, timezone
from pathlib import Path
import os
import re
import sys
import time
from zoneinfo import ZoneInfo, ZoneInfoNotFoundError


ROOT = Path(__file__).resolve().parent.parent
TIME_YAML = ROOT / "common" / "addon" / "time.yaml"
SUN_CALC_H = ROOT / "components" / "espcontrol" / "sun_calc.h"
AUTO_TIMEZONE_OPTION = "Auto (Home Assistant)"

ZONEINFO_ALIASES = {
    "Asia/Rangoon": "Asia/Yangon",
}


def load_timezone_options() -> dict[str, str]:
    options: dict[str, str] = {}
    for match in re.finditer(r'^\s+- "([^"]+)"$', TIME_YAML.read_text(), re.M):
        option = match.group(1)
        if option == AUTO_TIMEZONE_OPTION:
            continue
        if " (GMT" not in option or ("/" not in option and not option.startswith("UTC ")):
            continue
        tz_id = option.split(" (", 1)[0]
        options[tz_id] = option
    return options


def load_posix_table() -> dict[str, str]:
    text = SUN_CALC_H.read_text()
    return {
        match.group(1): match.group(2)
        for match in re.finditer(
            r'\{"([^"]+)",\s*[-0-9.]+f,\s*[-0-9.]+f,\s*"([^"]+)"\}', text
        )
    }


def load_casablanca_pauses() -> list[tuple[tuple[int, ...], tuple[int, ...]]]:
    text = SUN_CALC_H.read_text()
    return [
        (
            tuple(map(int, match.group(1, 2, 3, 4, 5))),
            tuple(map(int, match.group(6, 7, 8, 9, 10))),
        )
        for match in re.finditer(
            r"\{\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\},\s*"
            r"\{(\d+),\s*(\d+),\s*(\d+),\s*(\d+),\s*(\d+)\}\}",
            text,
        )
    ]


def utc_point(dt: datetime) -> tuple[int, int, int, int, int]:
    dt = dt.astimezone(timezone.utc)
    return (dt.year, dt.month, dt.day, dt.hour, dt.minute)


def point_in_ranges(
    point: tuple[int, int, int, int, int],
    ranges: list[tuple[tuple[int, ...], tuple[int, ...]]],
) -> bool:
    return any(start <= point < end for start, end in ranges)


def posix_offset_minutes(posix: str, dt: datetime) -> int:
    old_tz = os.environ.get("TZ")
    try:
        os.environ["TZ"] = posix
        time.tzset()
        ts = dt.timestamp()
        local_tm = time.localtime(ts)
        if hasattr(local_tm, "tm_gmtoff"):
            return int(local_tm.tm_gmtoff // 60)
        utc_tm = time.gmtime(ts)
        diff = (
            (local_tm.tm_yday - utc_tm.tm_yday) * 1440
            + (local_tm.tm_hour - utc_tm.tm_hour) * 60
            + local_tm.tm_min
            - utc_tm.tm_min
        )
        if diff > 720:
            diff -= 1440
        elif diff < -720:
            diff += 1440
        return diff
    finally:
        if old_tz is None:
            os.environ.pop("TZ", None)
        else:
            os.environ["TZ"] = old_tz
        time.tzset()


def iana_offset_minutes(tz_id: str, dt: datetime) -> int:
    try:
        zone = ZoneInfo(tz_id)
    except ZoneInfoNotFoundError:
        zone = ZoneInfo(ZONEINFO_ALIASES.get(tz_id, tz_id))
    return int(dt.astimezone(zone).utcoffset().total_seconds() // 60)


def firmware_offset_minutes(
    tz_id: str,
    posix: str,
    dt: datetime,
    casablanca_pauses: list[tuple[tuple[int, ...], tuple[int, ...]]],
) -> int:
    if tz_id == "Africa/Casablanca" and point_in_ranges(utc_point(dt), casablanca_pauses):
        return 0
    return posix_offset_minutes(posix, dt)


def expected_casablanca_pauses() -> list[tuple[tuple[int, ...], tuple[int, ...]]]:
    zone = ZoneInfo("Africa/Casablanca")
    start = datetime(2024, 1, 1, tzinfo=timezone.utc)
    end = datetime(2051, 1, 1, tzinfo=timezone.utc)
    pauses: list[tuple[tuple[int, ...], tuple[int, ...]]] = []
    pause_start: datetime | None = None
    prev = start.astimezone(zone).utcoffset()
    if prev == timedelta(0):
        pause_start = start
    dt = start + timedelta(hours=1)
    while dt <= end:
        offset = dt.astimezone(zone).utcoffset()
        if offset != prev:
            transition = dt
            if prev != timedelta(0) and offset == timedelta(0):
                pause_start = transition
            elif prev == timedelta(0) and offset != timedelta(0):
                if pause_start is None:
                    raise AssertionError("Casablanca pause end without start")
                pauses.append((utc_point(pause_start), utc_point(transition)))
                pause_start = None
            prev = offset
        dt += timedelta(hours=1)
    return pauses


def main() -> int:
    errors: list[str] = []
    options = load_timezone_options()
    posix_table = load_posix_table()
    casablanca_pauses = load_casablanca_pauses()

    missing = sorted(set(options) - set(posix_table))
    extra = sorted(set(posix_table) - set(options))
    if missing:
        errors.append(f"Missing firmware timezone rows: {', '.join(missing)}")
    if extra:
        errors.append(f"Unused firmware timezone rows: {', '.join(extra)}")

    quoted_numeric = sorted(
        tz_id for tz_id, posix in posix_table.items() if "<" in posix or ">" in posix
    )
    if quoted_numeric:
        errors.append(
            "Firmware POSIX strings must avoid quoted numeric timezone names: "
            + ", ".join(quoted_numeric)
        )

    expected_pauses = expected_casablanca_pauses()
    if casablanca_pauses != expected_pauses:
        errors.append("Africa/Casablanca pause table does not match Python zoneinfo")

    samples = [
        datetime(2026, 1, 15, 12, tzinfo=timezone.utc),
        datetime(2026, 3, 1, 12, tzinfo=timezone.utc),
        datetime(2026, 4, 22, 12, tzinfo=timezone.utc),
        datetime(2026, 7, 15, 12, tzinfo=timezone.utc),
        datetime(2026, 10, 15, 12, tzinfo=timezone.utc),
    ]
    for tz_id, posix in sorted(posix_table.items()):
        for sample in samples:
            want = iana_offset_minutes(tz_id, sample)
            got = firmware_offset_minutes(tz_id, posix, sample, casablanca_pauses)
            if got != want:
                errors.append(
                    f"{tz_id} offset mismatch on {sample.date()}: firmware {got} min, IANA {want} min"
                )
                break

    known = {
        "Europe/London": 60,
        "America/New_York": -240,
        "America/Los_Angeles": -420,
        "Asia/Almaty": 300,
        "Africa/Casablanca": 60,
    }
    april_2026 = datetime(2026, 4, 22, 12, tzinfo=timezone.utc)
    for tz_id, expected in known.items():
        got = firmware_offset_minutes(tz_id, posix_table[tz_id], april_2026, casablanca_pauses)
        if got != expected:
            errors.append(f"{tz_id} expected {expected} min on 2026-04-22, got {got} min")

    casablanca_ramadan = datetime(2026, 3, 1, 12, tzinfo=timezone.utc)
    got = firmware_offset_minutes(
        "Africa/Casablanca",
        posix_table["Africa/Casablanca"],
        casablanca_ramadan,
        casablanca_pauses,
    )
    if got != 0:
        errors.append(f"Africa/Casablanca expected 0 min during Ramadan pause, got {got} min")

    if errors:
        print("Timezone validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"Timezone validation passed for {len(options)} options.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
