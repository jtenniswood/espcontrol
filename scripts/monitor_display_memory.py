#!/usr/bin/env python3
"""Poll display memory diagnostics and summarize heap/PSRAM trends."""

from __future__ import annotations

import argparse
import csv
import json
import math
import re
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Iterable


DEFAULT_TARGETS = {
    "7inch": "192.168.6.102",
    "10inch": "192.168.6.103",
    "p4_86": "192.168.6.104",
    "4_3inch": "192.168.6.101",
    "s3": "192.168.6.105",
}

P4_TARGET_NAMES = ("7inch", "10inch", "p4_86", "4_3inch")

METRICS = {
    "heap_free": "/sensor/memory__heap_free_bytes",
    "heap_largest": "/sensor/memory__heap_largest_block",
    "psram_free": "/sensor/memory__psram_free_bytes",
    "psram_largest": "/sensor/memory__psram_largest_block",
}

REQUIRED_METRICS = {"heap_free", "psram_free"}


@dataclass(frozen=True)
class Target:
    name: str
    host: str


@dataclass(frozen=True)
class Reading:
    timestamp: float
    target: str
    values: dict[str, float]


@dataclass(frozen=True)
class Trend:
    target: str
    metric: str
    first: float
    last: float
    delta: float
    per_hour: float
    samples: int


class MonitorError(RuntimeError):
    pass


def remove_prefix(value: str, prefix: str) -> str:
    return value[len(prefix) :] if value.startswith(prefix) else value


def parse_target(value: str) -> Target:
    if "=" not in value:
        raise argparse.ArgumentTypeError("targets must use name=host")
    name, host = value.split("=", 1)
    name = name.strip()
    host = remove_prefix(remove_prefix(host.strip(), "http://"), "https://").rstrip("/")
    if not name or not host:
        raise argparse.ArgumentTypeError("targets must use name=host")
    return Target(name=name, host=host)


def selected_targets(preset: str, explicit: Iterable[Target]) -> list[Target]:
    targets = list(explicit)
    if targets:
        return targets
    names = P4_TARGET_NAMES if preset == "p4" else tuple(DEFAULT_TARGETS)
    return [Target(name=name, host=DEFAULT_TARGETS[name]) for name in names]


def metric_url(host: str, path: str) -> str:
    return f"http://{host}{path}"


def parse_metric_payload(payload: str) -> float:
    text = payload.strip()
    if not text:
        raise MonitorError("empty response")

    try:
        data = json.loads(text)
    except json.JSONDecodeError:
        data = None

    if isinstance(data, dict):
        for key in ("value", "state", "val"):
            value = data.get(key)
            if isinstance(value, (int, float)):
                return float(value)
            if isinstance(value, str):
                return parse_metric_payload(value)

    match = re.search(r"-?\d+(?:\.\d+)?", text)
    if match:
        return float(match.group(0))
    raise MonitorError(f"cannot parse response: {text[:80]}")


def fetch_metric(host: str, metric: str, timeout: float) -> float:
    url = metric_url(host, METRICS[metric])
    request = urllib.request.Request(url, headers={"User-Agent": "espcontrol-memory-monitor"})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            payload = response.read().decode("utf-8", errors="replace")
    except (OSError, urllib.error.URLError) as exc:
        raise MonitorError(str(exc)) from exc
    return parse_metric_payload(payload)


def collect_sample(targets: list[Target], timeout: float) -> list[Reading]:
    timestamp = time.time()
    readings: list[Reading] = []
    for target in targets:
        values: dict[str, float] = {}
        for metric in METRICS:
            try:
                values[metric] = fetch_metric(target.host, metric, timeout)
            except MonitorError:
                if metric in REQUIRED_METRICS:
                    raise
                values[metric] = math.nan
        readings.append(Reading(timestamp=timestamp, target=target.name, values=values))
    return readings


def summarize(readings: list[Reading]) -> list[Trend]:
    by_target: dict[str, list[Reading]] = {}
    for reading in readings:
        by_target.setdefault(reading.target, []).append(reading)

    trends: list[Trend] = []
    for target, target_readings in sorted(by_target.items()):
        target_readings.sort(key=lambda item: item.timestamp)
        if not target_readings:
            continue
        for metric in METRICS:
            metric_readings = [
                item for item in target_readings
                if math.isfinite(item.values.get(metric, math.nan))
            ]
            if not metric_readings:
                continue
            elapsed_hours = (metric_readings[-1].timestamp - metric_readings[0].timestamp) / 3600.0
            first = metric_readings[0].values[metric]
            last = metric_readings[-1].values[metric]
            delta = last - first
            per_hour = delta / elapsed_hours if elapsed_hours > 0 else 0.0
            trends.append(Trend(
                target=target,
                metric=metric,
                first=first,
                last=last,
                delta=delta,
                per_hour=per_hour,
                samples=len(metric_readings),
            ))
    return trends


def write_csv(path: Path, readings: list[Reading]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow(["timestamp", "target", *METRICS.keys()])
        for reading in readings:
            writer.writerow([
                datetime.fromtimestamp(reading.timestamp, timezone.utc).isoformat(),
                reading.target,
                *(reading.values[metric] if math.isfinite(reading.values[metric]) else "" for metric in METRICS),
            ])


def format_bytes(value: float) -> str:
    if not math.isfinite(value):
        return "n/a"
    return f"{value:,.0f}"


def print_sample(readings: list[Reading], baseline: dict[tuple[str, str], float]) -> None:
    if not readings:
        return
    stamp = datetime.fromtimestamp(readings[0].timestamp).isoformat(timespec="seconds")
    print(stamp)
    for reading in readings:
        cells = []
        for metric in METRICS:
            value = reading.values[metric]
            key = (reading.target, metric)
            if not math.isfinite(value):
                cells.append(f"{metric}=n/a")
                continue
            if key not in baseline:
                baseline[key] = value
            delta = value - baseline[key]
            cells.append(f"{metric}={format_bytes(value)} ({delta:+,.0f})")
        print(f"  {reading.target:8s} " + " ".join(cells))
    sys.stdout.flush()


def print_summary(trends: list[Trend], heap_tolerance: float, psram_tolerance: float) -> bool:
    print()
    print("Summary")
    leaking = False
    for trend in trends:
        tolerance = psram_tolerance if trend.metric.startswith("psram_") else heap_tolerance
        status = "ok"
        if trend.per_hour < -tolerance:
            status = "falling"
            leaking = True
        print(
            f"  {trend.target:8s} {trend.metric:13s} "
            f"{format_bytes(trend.first)} -> {format_bytes(trend.last)} "
            f"delta={trend.delta:+,.0f} per_hour={trend.per_hour:+,.0f} "
            f"samples={trend.samples} {status}"
        )
    return leaking


def run_monitor(args: argparse.Namespace) -> int:
    targets = selected_targets(args.preset, args.target)
    readings: list[Reading] = []
    baseline: dict[tuple[str, str], float] = {}
    for sample_index in range(args.samples):
        sample = collect_sample(targets, args.timeout)
        readings.extend(sample)
        print_sample(sample, baseline)
        if sample_index + 1 < args.samples:
            time.sleep(args.interval)

    if args.csv:
        write_csv(args.csv, readings)
        print(f"\nWrote {args.csv}")

    leaking = print_summary(readings and summarize(readings) or [], args.heap_tolerance, args.psram_tolerance)
    return 2 if args.fail_on_leak and leaking else 0


def run_self_test() -> int:
    now = 1_800_000_000.0
    readings = [
        Reading(now, "test", {
            "heap_free": 100_000,
            "heap_largest": 50_000,
            "psram_free": 10_000_000,
            "psram_largest": 8_000_000,
        }),
        Reading(now + 1800, "test", {
            "heap_free": 98_000,
            "heap_largest": 49_000,
            "psram_free": 9_999_000,
            "psram_largest": 8_000_000,
        }),
        Reading(now, "missing", {
            "heap_free": 100_000,
            "heap_largest": math.nan,
            "psram_free": 10_000_000,
            "psram_largest": math.nan,
        }),
        Reading(now + 1800, "missing", {
            "heap_free": 100_000,
            "heap_largest": math.nan,
            "psram_free": 10_000_000,
            "psram_largest": math.nan,
        }),
    ]
    trends = {(item.target, item.metric): item for item in summarize(readings)}
    heap = trends[("test", "heap_free")]
    psram = trends[("test", "psram_free")]
    assert heap.delta == -2000
    assert heap.per_hour == -4000
    assert psram.delta == -1000
    assert ("missing", "heap_free") in trends
    assert ("missing", "psram_free") in trends
    assert ("missing", "heap_largest") not in trends
    assert ("missing", "psram_largest") not in trends
    assert parse_metric_payload('{"value": "12345 B"}') == 12345
    assert parse_metric_payload("state: 67890") == 67890

    original_fetch_metric = fetch_metric
    try:
        def optional_missing_fetch(host: str, metric: str, timeout: float) -> float:
            if metric == "heap_largest":
                raise MonitorError("missing optional metric")
            return 42.0

        globals()["fetch_metric"] = optional_missing_fetch
        sample = collect_sample([Target("optional", "example.invalid")], 1.0)[0]
        assert math.isnan(sample.values["heap_largest"])
        assert sample.values["heap_free"] == 42.0

        def required_missing_fetch(host: str, metric: str, timeout: float) -> float:
            if metric == "heap_free":
                raise MonitorError("missing required metric")
            return 42.0

        globals()["fetch_metric"] = required_missing_fetch
        try:
            collect_sample([Target("required", "example.invalid")], 1.0)
        except MonitorError:
            pass
        else:
            raise AssertionError("self-test expected missing required metric to fail")
    finally:
        globals()["fetch_metric"] = original_fetch_metric

    print("Memory monitor self-test passed.")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--preset", choices=("p4", "all"), default="p4",
                        help="default target group when --target is not supplied")
    parser.add_argument("--target", action="append", type=parse_target, default=[],
                        help="display target as name=host; may be supplied more than once")
    parser.add_argument("--samples", type=int, default=12,
                        help="number of samples to collect")
    parser.add_argument("--interval", type=float, default=300.0,
                        help="seconds between samples")
    parser.add_argument("--timeout", type=float, default=5.0,
                        help="HTTP timeout per metric")
    parser.add_argument("--csv", type=Path,
                        help="optional CSV output path")
    parser.add_argument("--heap-tolerance", type=float, default=4096.0,
                        help="allowed internal-heap fall per hour before reporting falling")
    parser.add_argument("--psram-tolerance", type=float, default=1_048_576.0,
                        help="allowed PSRAM fall per hour before reporting falling")
    parser.add_argument("--fail-on-leak", action="store_true",
                        help="exit non-zero when a metric falls faster than tolerance")
    parser.add_argument("--self-test", action="store_true",
                        help="run offline parser/slope tests")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.self_test:
        return run_self_test()
    if args.samples < 1:
        parser.error("--samples must be at least 1")
    if args.interval < 0:
        parser.error("--interval must be non-negative")
    return run_monitor(args)


if __name__ == "__main__":
    raise SystemExit(main())
