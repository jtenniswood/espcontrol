#!/usr/bin/env python3
"""Validate timezone selector options against firmware rules."""
from __future__ import annotations

import argparse
from datetime import datetime, timedelta, timezone
from pathlib import Path
import os
import re
import sys
import time


ROOT = Path(__file__).resolve().parent.parent
TIME_YAML = ROOT / "common" / "addon" / "time.yaml"
SUN_CALC_H = ROOT / "components" / "espcontrol" / "sun_calc.h"
AUTO_TIMEZONE_OPTION = "Auto (Home Assistant)"

ZONEINFO_ALIASES = {
    "Asia/Rangoon": "Asia/Yangon",
}


def load_timezone_select_options() -> list[tuple[int, str]]:
    options: list[tuple[int, str]] = []
    in_timezone_select = False
    in_options = False
    for line_no, line in enumerate(TIME_YAML.read_text().splitlines(), 1):
        if re.match(r"^\s+id:\s*timezone_select\s*$", line):
            in_timezone_select = True
            continue
        if in_timezone_select and not in_options:
            if re.match(r"^\s+options:\s*$", line):
                in_options = True
            continue
        if not in_options:
            continue
        match = re.match(r'^\s+- "([^"]+)"$', line)
        if match:
            options.append((line_no, match.group(1)))
            continue
        if line.strip():
            break
    return options


def timezone_option_id(option: str) -> str | None:
    if " (GMT" not in option or ("/" not in option and not option.startswith("UTC ")):
        return None
    return option.split(" (", 1)[0]


def load_timezone_options() -> dict[str, str]:
    options: dict[str, str] = {}
    for _line_no, option in load_timezone_select_options():
        if option == AUTO_TIMEZONE_OPTION:
            continue
        tz_id = timezone_option_id(option)
        if tz_id is None:
            continue
        options[tz_id] = option
    return options


def validate_timezone_select_options(rows: list[tuple[int, str]]) -> list[str]:
    errors: list[str] = []
    if not rows:
        return [f"{TIME_YAML.relative_to(ROOT)}: timezone_select options were not found"]

    options_seen: dict[str, int] = {}
    tz_ids_seen: dict[str, int] = {}
    auto_lines = []
    for line_no, option in rows:
        if option in options_seen:
            errors.append(
                f"{TIME_YAML.relative_to(ROOT)}:{line_no}: duplicate timezone option "
                f"{option!r} (first defined on line {options_seen[option]})"
            )
        options_seen[option] = line_no

        if option == AUTO_TIMEZONE_OPTION:
            auto_lines.append(line_no)
            continue

        tz_id = timezone_option_id(option)
        if tz_id is None:
            errors.append(f"{TIME_YAML.relative_to(ROOT)}:{line_no}: invalid timezone option {option!r}")
            continue
        if tz_id in tz_ids_seen:
            errors.append(
                f"{TIME_YAML.relative_to(ROOT)}:{line_no}: duplicate timezone id "
                f"{tz_id!r} (first defined on line {tz_ids_seen[tz_id]})"
            )
        tz_ids_seen[tz_id] = line_no

    if len(auto_lines) != 1:
        errors.append(
            f"{TIME_YAML.relative_to(ROOT)}: expected exactly one {AUTO_TIMEZONE_OPTION!r} option, "
            f"found {len(auto_lines)}"
        )
    elif rows[-1][1] != AUTO_TIMEZONE_OPTION:
        errors.append(
            f"{TIME_YAML.relative_to(ROOT)}:{auto_lines[0]}: {AUTO_TIMEZONE_OPTION!r} must stay last "
            "so restored option indexes remain stable"
        )

    return errors


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


def local_offset_minutes(dt: datetime) -> int:
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


def posix_offset_minutes(posix: str, dt: datetime) -> int:
    old_tz = os.environ.get("TZ")
    try:
        os.environ["TZ"] = posix
        time.tzset()
        return local_offset_minutes(dt)
    finally:
        if old_tz is None:
            os.environ.pop("TZ", None)
        else:
            os.environ["TZ"] = old_tz
        time.tzset()


def iana_offset_minutes(tz_id: str, dt: datetime) -> int:
    return posix_offset_minutes(ZONEINFO_ALIASES.get(tz_id, tz_id), dt)


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
    start = datetime(2024, 1, 1, tzinfo=timezone.utc)
    end = datetime(2051, 1, 1, tzinfo=timezone.utc)
    pauses: list[tuple[tuple[int, ...], tuple[int, ...]]] = []
    pause_start: datetime | None = None
    old_tz = os.environ.get("TZ")
    try:
        os.environ["TZ"] = "Africa/Casablanca"
        time.tzset()
        prev = timedelta(minutes=local_offset_minutes(start))
        if prev == timedelta(0):
            pause_start = start
        dt = start + timedelta(hours=1)
        while dt <= end:
            offset = timedelta(minutes=local_offset_minutes(dt))
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
    finally:
        if old_tz is None:
            os.environ.pop("TZ", None)
        else:
            os.environ["TZ"] = old_tz
        time.tzset()
    return pauses


def validate_timezones(
    options: dict[str, str],
    posix_table: dict[str, str],
    casablanca_pauses: list[tuple[tuple[int, ...], tuple[int, ...]]],
    *,
    check_casablanca_table: bool = True,
) -> list[str]:
    errors: list[str] = []
    errors.extend(validate_timezone_select_options(load_timezone_select_options()))

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

    if check_casablanca_table:
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
        if tz_id not in options:
            continue
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
        if tz_id not in posix_table:
            continue
        got = firmware_offset_minutes(tz_id, posix_table[tz_id], april_2026, casablanca_pauses)
        if got != expected:
            errors.append(f"{tz_id} expected {expected} min on 2026-04-22, got {got} min")

    if "Africa/Casablanca" in posix_table:
        casablanca_ramadan = datetime(2026, 3, 1, 12, tzinfo=timezone.utc)
        got = firmware_offset_minutes(
            "Africa/Casablanca",
            posix_table["Africa/Casablanca"],
            casablanca_ramadan,
            casablanca_pauses,
        )
        if got != 0:
            errors.append(f"Africa/Casablanca expected 0 min during Ramadan pause, got {got} min")

    return errors


def expect_error(errors: list[str], expected: str) -> None:
    if not any(expected in error for error in errors):
        raise AssertionError(f"expected error containing {expected!r}, got {errors!r}")


def run_self_test() -> int:
    options = load_timezone_options()
    posix_table = load_posix_table()
    casablanca_pauses = load_casablanca_pauses()

    errors = validate_timezones(options, posix_table, casablanca_pauses, check_casablanca_table=False)
    assert errors == []

    known = {"Europe/London", "America/New_York", "America/Los_Angeles", "Asia/Almaty", "Africa/Casablanca"}
    missing_id = next(tz_id for tz_id in options if tz_id not in known)
    missing_posix = dict(posix_table)
    del missing_posix[missing_id]
    expect_error(
        validate_timezones(options, missing_posix, casablanca_pauses, check_casablanca_table=False),
        f"Missing firmware timezone rows: {missing_id}",
    )

    extra_id = "Etc/UTC" if "Etc/UTC" not in options else "UTC"
    extra_posix = dict(posix_table)
    extra_posix[extra_id] = "UTC0"
    expect_error(
        validate_timezones(options, extra_posix, casablanca_pauses, check_casablanca_table=False),
        f"Unused firmware timezone rows: {extra_id}",
    )

    quoted_posix = dict(posix_table)
    quoted_posix["Europe/London"] = "<GMT0>0"
    expect_error(
        validate_timezones(options, quoted_posix, casablanca_pauses, check_casablanca_table=False),
        "Firmware POSIX strings must avoid quoted numeric timezone names: Europe/London",
    )

    wrong_offset_posix = dict(posix_table)
    wrong_offset_posix["Europe/London"] = "UTC0"
    expect_error(
        validate_timezones(options, wrong_offset_posix, casablanca_pauses, check_casablanca_table=False),
        "Europe/London offset mismatch",
    )

    expect_error(
        validate_timezones(options, posix_table, casablanca_pauses[1:], check_casablanca_table=True),
        "Africa/Casablanca pause table does not match Python zoneinfo",
    )

    print("Timezone self-test passed.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run timezone validator self-tests")
    args = parser.parse_args(argv)

    if args.self_test:
        return run_self_test()

    options = load_timezone_options()
    posix_table = load_posix_table()
    casablanca_pauses = load_casablanca_pauses()
    errors = validate_timezones(options, posix_table, casablanca_pauses)

    if errors:
        print("Timezone validation failed:")
        for error in errors:
            print(f"  - {error}")
        return 1

    print(f"Timezone validation passed for {len(options)} options.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
