#!/usr/bin/env python3
"""Live EspControl health/crash checks; no firmware writes or HA commands."""
from __future__ import annotations

import argparse
import asyncio
from collections import Counter
from contextlib import suppress
from dataclasses import dataclass, field
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import re
import sys
import time
import urllib.error
import urllib.request

CRASH = re.compile(
    r"Guru Meditation|CORRUPT HEAP|assert(?:ion)? failed|abort\(\).*called|"
    r"stack (?:canary|overflow)|Stack smashing|task_wdt.*trigger|"
    r"watchdog.*(?:trigger|reset)|LoadProhibited|StoreProhibited|"
    r"IllegalInstruction|Backtrace:", re.I)
METRICS = {"memory__heap_free": "heap_free_percent",
           "memory__heap_largest_block": "heap_largest_bytes",
           "memory__psram_free": "psram_free_percent",
           "memory__psram_largest_block": "psram_largest_bytes"}
MAX_BODY = 65536


def positive(value):
    number = float(value)
    if not math.isfinite(number) or number <= 0:
        raise argparse.ArgumentTypeError("must be a finite positive number")
    return number


def safe_log(text):
    text = re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", text)
    text = re.sub(r"https?://\S+", "<url>", text)
    return re.sub(r"(?i)(password|token|api_key|authorization)(\s*[:=]\s*)\S+",
                  r"\1\2<redacted>", text)[:1000]


def read_boot_clock(response):
    """Read only the initial ping event. Never retain entity/config payloads."""
    event, identifier = "", None
    consumed = 0
    while consumed < MAX_BODY:
        line = response.readline(4097)
        consumed += len(line)
        if not line or len(line) > 4096:
            break
        line = line.decode("utf-8").strip()
        if not line:
            if event == "ping" and identifier is not None:
                if not 0 <= identifier <= 0xFFFFFFFF:
                    raise ValueError("invalid boot clock")
                return identifier
            event, identifier = "", None
        elif line.startswith("event:"):
            event = line[6:].strip()
        elif line.startswith("id:"):
            identifier = int(line[3:].strip())
    raise ValueError("event stream did not supply a boot clock")


class HttpProbe:
    def __init__(self, host, port, timeout):
        host = f"[{host}]" if ":" in host and not host.startswith("[") else host
        self.base = f"http://{host}:{port}"
        self.timeout = timeout
        manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
        username = os.environ.get("ESPCONTROL_HTTP_USERNAME")
        if username:
            manager.add_password(None, self.base, username,
                                 os.environ.get("ESPCONTROL_HTTP_PASSWORD", ""))
        self.opener = urllib.request.build_opener(
            urllib.request.ProxyHandler({}),
            urllib.request.HTTPDigestAuthHandler(manager),
            urllib.request.HTTPBasicAuthHandler(manager))

    def open(self, path):
        return self.opener.open(urllib.request.Request(
            self.base + path, headers={"User-Agent": "EspControl-device-test"}),
            timeout=self.timeout)

    def json(self, path):
        with self.open(path) as response:
            data = response.read(MAX_BODY + 1)
        if len(data) > MAX_BODY:
            raise ValueError("oversized JSON response")
        result = json.loads(data)
        if not isinstance(result, dict):
            raise ValueError("expected a JSON object")
        return result

    def sample(self):
        identity = self.json("/api/v1/identity")
        capabilities = self.json("/api/v1/capabilities")
        if not isinstance(identity.get("hostname"), str) or not identity["hostname"]:
            raise ValueError("missing device hostname")
        if identity.get("restart_required") is not False:
            raise ValueError("device requires a restart")
        if capabilities.get("api", {}).get("version") != 1:
            raise ValueError("unsupported device API")
        with self.open("/events") as response:
            if "text/event-stream" not in response.headers.get("Content-Type", ""):
                raise ValueError("expected an event stream")
            clock = read_boot_clock(response)
        return {"hostname": identity["hostname"], "clock_ms": clock}


@dataclass
class Evidence:
    output: Path
    started: float = field(default_factory=time.monotonic)
    failures: Counter = field(default_factory=Counter)
    counts: Counter = field(default_factory=Counter)
    metrics: dict = field(default_factory=dict)
    metric_updates: Counter = field(default_factory=Counter)
    firmware: dict = field(default_factory=dict)
    previous_clock: tuple | None = None
    hostname: str | None = None
    memory_minimum: float = 5.0

    def event(self, kind, failure=False, **details):
        if failure:
            self.failures[kind] += 1
        self.counts[kind] += 1
        record = {"elapsed_seconds": round(time.monotonic() - self.started, 3),
                  "event": kind, "failure": failure, **details}
        with (self.output / "events.jsonl").open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, allow_nan=False) + "\n")
        if failure:
            print(f"FAIL {kind}: {json.dumps(details)}", flush=True)

    def sample(self, data, now=None):
        now = time.monotonic() if now is None else now
        clock = data["clock_ms"]
        if self.previous_clock:
            old, previous_time = self.previous_clock
            elapsed_ms = max(0, (now - previous_time) * 1000)
            advance = (clock - old) & 0xFFFFFFFF
            # A normal 32-bit millis wrap advances by roughly wall time.
            if clock < old and advance > elapsed_ms + 5000:
                self.event("reboot", failure=True, previous_ms=old, current_ms=clock)
        self.previous_clock = clock, now
        if self.hostname and self.hostname != data["hostname"]:
            self.event("identity_changed", failure=True)
        self.hostname = data["hostname"]
        self.event("http_sample", **data)

    def memory(self, metric, value):
        if not math.isfinite(value):
            return
        self.metric_updates[metric] += 1
        current = self.metrics.setdefault(metric, {"first": value, "last": value,
                                                   "minimum": value, "maximum": value})
        current.update(last=value, minimum=min(current["minimum"], value),
                       maximum=max(current["maximum"], value))
        if metric == "heap_free_percent" and value < self.memory_minimum:
            self.event("memory_pressure", failure=True, metric=metric, value=value)
        self.event("memory", metric=metric, value=value)

    def firmware_info(self, info, expected_version=None, expected_build=None):
        current = {key: getattr(info, key) for key in
                   ("name", "mac_address", "model", "esphome_version", "compilation_time", "project_version")}
        if self.firmware and current != self.firmware:
            self.event("firmware_or_device_changed", failure=True)
        self.firmware = current
        if expected_version and current["esphome_version"] != expected_version:
            self.event("wrong_firmware_version", failure=True)
        if expected_build and current["compilation_time"] != expected_build:
            self.event("wrong_build_timestamp", failure=True)
        self.event("native_connected", firmware=current)


def final_report(evidence, args, interrupted=False):
    if evidence.counts["http_sample"] < 2:
        evidence.event("incomplete_http_coverage", failure=True)
    if not evidence.counts["native_connected"]:
        evidence.event("incomplete_native_coverage", failure=True)
    if not evidence.metric_updates["heap_free_percent"]:
        evidence.event("missing_memory_diagnostics", failure=True)
    if args.modal_entity and evidence.counts["modal_cycle"] < args.modal_cycles:
        evidence.event("incomplete_modal_coverage", failure=True)
    if interrupted:
        evidence.event("interrupted", failure=True)
    report = {"status": "failed" if evidence.failures else "passed",
              "host": args.host, "duration_seconds": round(time.monotonic() - evidence.started, 2),
              "requested_duration_seconds": args.duration,
              "firmware": evidence.firmware, "failures": dict(evidence.failures),
              "counts": dict(evidence.counts), "memory": evidence.metrics,
              "metric_updates": dict(evidence.metric_updates),
              "modal_entity": args.modal_entity, "modal_cycles_requested": args.modal_cycles if args.modal_entity else 0,
              "limits": ["Passing describes only the observed test window.",
                         "Reboot evidence does not establish the reset cause.",
                         "Network/API outages are failures but are not classified as confirmed crashes.",
                         "Panics before WiFi/log delivery may need USB serial capture.",
                         "Modal acknowledgement proves opening; this does not exercise speaker joins or volume commands.",
                         "The runner attempts to close the modal; the prior screen/sleep state is not restored."]}
    (evidence.output / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    summary = (f"# Device runtime test: {report['status'].upper()}\n\n"
               f"Target: `{args.host}`\n\n"
               f"Observed: {report['duration_seconds']} seconds; "
               f"HTTP samples: {evidence.counts['http_sample']}; "
               f"completed modal cycles: {evidence.counts['modal_cycle']}; "
               f"other device error logs: {evidence.counts['other_error_log']}.\n\n"
               f"Firmware: `{evidence.firmware.get('esphome_version', 'unavailable')}`; "
               f"built `{evidence.firmware.get('compilation_time', 'unavailable')}`.\n\n"
               "## Failures\n\n" +
               ("\n".join(f"- {kind}: {count}" for kind, count in evidence.failures.items()) or "None detected.") +
               "\n\n## Limits\n\n" + "\n".join("- " + line for line in report["limits"]) + "\n")
    (evidence.output / "report.md").write_text(summary)
    return report


async def run_suite(args, client_factory=None):
    # Import only for live runs: deterministic CI tests need no device or SDK.
    if client_factory is None:
        from aioesphomeapi import APIClient
        client_factory = APIClient
    from aioesphomeapi import LogLevel
    args.output.mkdir(parents=True, exist_ok=False)
    evidence = Evidence(args.output, memory_minimum=args.min_heap_percent)
    probe = HttpProbe(args.host, args.http_port, args.timeout)
    stop = asyncio.Event()
    ready = asyncio.Event()
    opened = asyncio.Event()
    rejected = asyncio.Event()
    client = None
    services = {}
    intentional_close = False
    connection_lost = asyncio.Event()
    interrupted = False
    active_tasks = []

    async def pause(seconds):
        with suppress(asyncio.TimeoutError):
            await asyncio.wait_for(stop.wait(), seconds)

    async def on_stop(expected):
        if not intentional_close and not stop.is_set():
            evidence.event("native_disconnect", failure=True, expected=expected)
            ready.clear()
            connection_lost.set()

    def on_log(message):
        text = message.message.decode("utf-8", errors="replace") if isinstance(message.message, bytes) else str(message.message)
        evidence.counts["native_log"] += 1
        if CRASH.search(re.sub(r"\x1b\[[0-9;]*[A-Za-z]", "", text)):
            evidence.event("crash_signature", failure=True, message=safe_log(text))
        elif message.level <= 1:
            evidence.event("other_error_log", message=safe_log(text))
        if args.modal_entity and "[open_modal:" in text:
            if f"Opened {args.modal_entity} " in text:
                opened.set()
            elif "Rejected request" in text or (args.modal_entity in text and "Opened " not in text and message.level <= 2):
                rejected.set()
                evidence.event("modal_rejected", failure=True, message=safe_log(text))

    async def native_loop():
        nonlocal client, services, intentional_close
        while not stop.is_set():
            connection_lost.clear()
            client = client_factory(args.host, args.api_port,
                                    os.environ.get("ESPCONTROL_API_PASSWORD"),
                                    noise_psk=os.environ.get("ESPCONTROL_API_NOISE_PSK"),
                                    client_info="EspControl Device Test", provide_time=False,
                                    keepalive=5.0)
            try:
                await asyncio.wait_for(client.connect(on_stop=on_stop, login=True), args.timeout)
                info = await asyncio.wait_for(client.device_info(), args.timeout)
                evidence.firmware_info(info, args.expect_version, args.expect_build)
                entities, offered_services = await asyncio.wait_for(client.list_entities_services(), args.timeout)
                keys = {entity.key: METRICS[entity.object_id] for entity in entities
                        if entity.object_id in METRICS}
                services = {service.name: service for service in offered_services}
                def on_state(state):
                    if state.key in keys and not getattr(state, "missing_state", False):
                        evidence.memory(keys[state.key], float(state.state))
                client.subscribe_states(on_state)
                client.subscribe_logs(on_log, log_level=LogLevel.LOG_LEVEL_INFO if args.modal_entity else LogLevel.LOG_LEVEL_WARN,
                                      dump_config=False)
                ready.set()
                while not stop.is_set() and not connection_lost.is_set():
                    await pause(min(args.interval, 1.0))
            except asyncio.CancelledError:
                raise
            except Exception as exc:
                evidence.event("native_error", failure=True, error=type(exc).__name__)
            finally:
                ready.clear()
                intentional_close = True
                with suppress(Exception):
                    await asyncio.wait_for(client.disconnect(), args.timeout)
                intentional_close = False
            if not stop.is_set():
                await pause(args.interval)

    async def http_loop():
        while not stop.is_set():
            try:
                data = await asyncio.to_thread(probe.sample)
                evidence.sample(data)
            except Exception as exc:
                evidence.event("http_error", failure=True, error=type(exc).__name__,
                               status=getattr(exc, "code", None))
            await pause(args.interval)

    async def action(name, data):
        if name not in ("open_modal", "close_modal"):
            raise ValueError("unsupported test action")
        service = services[name]
        response = await asyncio.wait_for(
            client.execute_service(
                service, data,
                return_response=True if getattr(service, "supports_response", None) else None,
                timeout=args.timeout), args.timeout)
        if response is not None and not response.success:
            raise RuntimeError("device rejected API action")

    async def stress_modals():
        if not args.modal_entity:
            return
        try:
            await asyncio.wait_for(ready.wait(), args.timeout * 2)
            if not {"open_modal", "close_modal"}.issubset(services):
                raise RuntimeError("modal actions missing")
            for _ in range(args.modal_cycles):
                if stop.is_set() or not ready.is_set():
                    break
                opened.clear()
                rejected.clear()
                try:
                    await action("open_modal", {"entity_id": args.modal_entity})
                    # Do not count an accepted API script as an opened modal.
                    await asyncio.wait_for(opened.wait(), args.timeout)
                    if rejected.is_set():
                        raise RuntimeError("modal rejected")
                    evidence.event("modal_opened")
                    await pause(args.modal_dwell)
                finally:
                    if ready.is_set():
                        await action("close_modal", {})
                evidence.event("modal_cycle")
                await pause(args.modal_dwell)
        except asyncio.CancelledError:
            raise
        except Exception as exc:
            evidence.event("modal_error", failure=True, error=type(exc).__name__,
                           reason=str(exc) if type(exc) is RuntimeError else "request or acknowledgement failed")

    try:
        active_tasks = [asyncio.create_task(native_loop()), asyncio.create_task(http_loop()),
                        asyncio.create_task(stress_modals())]
        print(f"Testing {args.host} for {args.duration:g}s; reports: {args.output}", flush=True)
        await asyncio.sleep(args.duration)
    except asyncio.CancelledError:
        interrupted = True
    finally:
        # Keep the native connection alive until the modal's finally block has
        # closed it. Stopping both workers first races against UI cleanup.
        if active_tasks:
            active_tasks[2].cancel()
            try:
                cleanup = await asyncio.wait_for(
                    asyncio.gather(active_tasks[2], return_exceptions=True), args.timeout * 2 + 1)
                if any(isinstance(result, Exception) for result in cleanup):
                    evidence.event("modal_cleanup_incomplete", failure=True)
            except (Exception, asyncio.CancelledError):
                evidence.event("modal_cleanup_incomplete", failure=True)
            stop.set()
            for task in active_tasks[:2]:
                task.cancel()
            results = await asyncio.gather(*active_tasks[:2], return_exceptions=True)
            for result in results:
                if isinstance(result, Exception):
                    evidence.event("test_task_error", failure=True, error=type(result).__name__)
        report = final_report(evidence, args, interrupted)
    print(f"{report['status'].upper()}: {dict(evidence.failures)}; report: {args.output / 'report.md'}", flush=True)
    return 0 if report["status"] == "passed" else 1


def arguments(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="explicit device hostname or IP (no URL)")
    parser.add_argument("--duration", type=positive, default=120, help="observation seconds (default 120)")
    parser.add_argument("--interval", type=positive, default=10, help="seconds between HTTP checks")
    parser.add_argument("--timeout", type=positive, default=5, help="per-request timeout seconds")
    parser.add_argument("--http-port", type=int, default=80)
    parser.add_argument("--api-port", type=int, default=6053)
    parser.add_argument("--expect-version")
    parser.add_argument("--expect-build", help="exact ESPHome compilation timestamp")
    parser.add_argument("--modal-entity", help="optional existing card entity; enables visible open/close stress")
    parser.add_argument("--modal-cycles", type=int, default=20)
    parser.add_argument("--modal-dwell", type=positive, default=1, help="seconds with modal open and closed")
    parser.add_argument("--min-heap-percent", type=positive, default=5)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args(argv)
    if re.search(r"[/@\s]", args.host) or not args.host:
        parser.error("--host must be a hostname/IP, not a URL")
    if not all(1 <= port <= 65535 for port in (args.http_port, args.api_port)):
        parser.error("ports must be between 1 and 65535")
    if args.modal_cycles < 1 or args.min_heap_percent > 100:
        parser.error("cycles must be positive and heap threshold must not exceed 100")
    if args.modal_entity and not re.fullmatch(r"[a-z_]+\.[a-z0-9_]+", args.modal_entity):
        parser.error("--modal-entity must be a Home Assistant entity ID")
    if args.output is None:
        stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
        args.output = Path("build/device-tests") / stamp
    args.output = args.output.resolve()
    if args.output.exists():
        parser.error("--output must name a new directory, to preserve existing evidence")
    return args


def main(argv=None):
    args = arguments(argv)
    try:
        return asyncio.run(run_suite(args))
    except ImportError:
        print("Install the pinned ESPHome environment from docs/reference/contributing.md first.", file=sys.stderr)
        return 2
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())
