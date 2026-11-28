#!/usr/bin/env python3
"""Summarize one continuous 10-inch V3 display benchmark log as JSON."""

import argparse
import json
import math
import re
from pathlib import Path

LOG_RECORD = re.compile(r"\[display_benchmark(?::\d+)?\]:\s*(sample|task|events) (.*)")
PAIR = {"R": ("D", "render"), "C": ("F", "flush_callback"),
        "A": ("W", "flush_wait"), "V": ("V", "refresh_interval")}
MEMORY_FIELDS = ("internal_free", "internal_min", "internal_largest",
                 "psram_free", "psram_min", "psram_largest")


def summary(values):
    ordered = sorted(values)
    return {
        "count": len(ordered),
        "p50": ordered[math.ceil(len(ordered) * 0.50) - 1],
        "p95": ordered[math.ceil(len(ordered) * 0.95) - 1],
        "max": ordered[-1],
    }


def read_samples(lines):
    samples = []

    for line in lines:
        record = LOG_RECORD.search(line)
        if record is None:
            continue
        kind, data = record.groups()
        data = data.split("\033", 1)[0]
        if kind in ("sample", "task"):
            fields = {key: int(value) for key, value in re.findall(r"(\w+)=(-?\d+)", data)}
            if kind == "sample":
                if samples and fields["time_us"] <= samples[-1]["time_us"]:
                    raise ValueError("Expected one uninterrupted capture in boot timestamp order.")
                samples.append({**fields, "tasks": [], "events": []})
            elif samples:
                samples[-1]["tasks"].append(fields)
            continue

        if samples:
            samples[-1]["events"].extend(re.findall(r"([TUDRFCWAV]):(\d+)", data))

    # The receiver can stop midway through the last report. A legacy final
    # report has no event count, so only a following report establishes its end.
    discarded_terminal_sample = False
    if samples:
        last = samples[-1]
        expected = last.get("event_count")
        if expected is None or len(last["tasks"]) < last["task_count"] or len(last["events"]) < expected:
            samples.pop()
            discarded_terminal_sample = True

    if len(samples) < 2:
        raise ValueError("Capture at least two complete benchmark samples.")
    for sample in samples:
        if sample["lost"]:
            raise ValueError("Firmware dropped trace events. Shorten each reporting interval.")
        if sample["task_count"] == 0 or len(sample["tasks"]) != sample["task_count"]:
            raise ValueError("At least one CPU snapshot is missing or has unexpected task records.")
        if sample.get("event_count", len(sample["events"])) != len(sample["events"]):
            raise ValueError("At least one benchmark report is missing or has unexpected trace events.")
    return samples, discarded_terminal_sample


def analyze(lines):
    samples, discarded_terminal_sample = read_samples(lines)
    timings = {}
    starts = {}
    previous = 0
    flushed_this_refresh = False
    touch_counts = {"T": 0, "U": 0}

    # The first report contains events from before the first CPU snapshot.
    # Measure CPU and display activity over the same window.
    for sample in samples[1:]:
        for code, timestamp in sample["events"]:
            now = int(timestamp)
            if now < previous:
                raise ValueError("Trace events are out of order or span multiple boots.")
            previous = now
            if code == "F":
                flushed_this_refresh = True
            if code == "V":
                # LVGL emits REFR_READY even when no pixels were redrawn.
                if not flushed_this_refresh:
                    continue
                flushed_this_refresh = False
            if code in touch_counts:
                touch_counts[code] += 1
            if code in PAIR:
                start, label = PAIR[code]
                if start in starts:
                    timings.setdefault(label, []).append(now - starts.pop(start))
            if code in "DFWV":
                starts[code] = now

    def idle_counters(sample):
        counters = {task["idle_core"]: task["runtime"] for task in sample["tasks"]
                    if task["idle_core"] >= 0}
        if counters.keys() != {0, 1}:
            raise ValueError("Expected two idle task counters on the ESP32-P4.")
        return counters

    first, last = samples[0], samples[-1]
    elapsed = last["total_runtime"] - first["total_runtime"]
    first_idle, last_idle = idle_counters(first), idle_counters(last)
    idle = [last_idle[core] - first_idle[core] for core in first_idle]
    if elapsed <= 0 or any(delta < 0 or delta > elapsed for delta in idle):
        raise ValueError("CPU counters wrapped or disagree. Capture a shorter interval.")
    busy = 2 * elapsed - sum(idle)
    def counter_delta(key):
        if key in first and key in last:
            return (last[key] - first[key]) % (1 << 32)
        return None

    first_tasks = {task["id"]: task for task in first["tasks"]}
    role_names = {1: "esphome_loop", 2: "display_worker"}
    role_ticks = {
        role_names[task["role"]]: task["runtime"] - first_tasks[task["id"]]["runtime"]
        for task in last["tasks"]
        if task.get("role") in role_names and task["id"] in first_tasks
    }
    return {
        "seconds_between_samples": (last["time_us"] - first["time_us"]) / 1_000_000,
        "discarded_terminal_sample": discarded_terminal_sample,
        "cpu_busy_counter_ticks": busy,
        "cpu_busy_percent_both_cores": 100 * busy / (2 * elapsed),
        "cpu_task_ticks": role_ticks,
        "memory_min_bytes": {key: min(sample[key] for sample in samples) for key in MEMORY_FIELDS},
        "lvgl_draw_buffer_bytes": last.get("draw_buffer_bytes"),
        "dsi_staging_bytes": last.get("dsi_staging_bytes"),
        "dsi_staged_submissions": counter_delta("dsi_staged_submissions"),
        "native_flush_bytes": last.get("native_flush_bytes"),
        "native_flush_regions": counter_delta("native_flush_regions"),
        "render_timings_us": {name: summary(values) for name, values in sorted(timings.items())},
        "touches": {"presses": touch_counts["T"], "releases": touch_counts["U"]},
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, help="ESPHome or serial log for one workload on one boot")
    args = parser.parse_args()
    try:
        with args.log.open(encoding="utf-8", errors="replace") as source:
            result = analyze(source)
    except (OSError, ValueError, KeyError) as error:
        parser.error(str(error))
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
