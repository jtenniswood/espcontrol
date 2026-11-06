#!/usr/bin/env python3
"""Validate a running EspControl device's HTTP and event-stream recovery."""

from __future__ import annotations

import argparse
import asyncio
import http.client
import json
import math
import os
from pathlib import Path
import re
import socket
import statistics
import threading
import time
from datetime import datetime, timezone
import urllib.request


MAX_BODY = 2 * 1024 * 1024


class CheckFailure(RuntimeError):
    pass


class SmallWindowConnection(http.client.HTTPConnection):
    def connect(self):
        super().connect()
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)


class SmallWindowHandler(urllib.request.HTTPHandler):
    def http_open(self, request):
        return self.do_open(SmallWindowConnection, request)


class DeviceHTTP:
    def __init__(self, host, port, timeout, username=None, password=None):
        self.base = f"http://{host}:{port}"
        self.timeout = timeout
        self.latencies = []
        self.http_checks = 0
        self.reconnects = 0
        self.uptimes = []
        self.openers = []
        for slow in (False, True):
            handlers = [SmallWindowHandler()] if slow else []
            # Device requests stay on the LAN even when the shell has a proxy.
            handlers.append(urllib.request.ProxyHandler({}))
            if username is not None:
                manager = urllib.request.HTTPPasswordMgrWithDefaultRealm()
                manager.add_password(None, self.base, username, password)
                handlers += [urllib.request.HTTPDigestAuthHandler(manager),
                             urllib.request.HTTPBasicAuthHandler(manager)]
            self.openers.append(urllib.request.build_opener(*handlers))

    def open(self, path, slow=False):
        headers = {"Accept": "text/event-stream"} if path == "/events" else {}
        response = self.openers[int(slow)].open(
            urllib.request.Request(self.base + path, headers=headers), timeout=self.timeout)
        if response.status != 200:
            response.close()
            raise CheckFailure(f"{path}: expected HTTP 200, got {response.status}")
        return response

    def get(self, path):
        started = time.monotonic()
        with self.open(path) as response:
            body = response.read(MAX_BODY + 1)
        if len(body) > MAX_BODY:
            raise CheckFailure(f"{path}: response exceeds {MAX_BODY} bytes")
        self.http_checks += 1
        self.latencies.append(time.monotonic() - started)
        return body

    def stream(self, slow=False):
        response = self.open("/events", slow=slow)
        try:
            if not response.headers.get("Content-Type", "").startswith("text/event-stream"):
                raise CheckFailure("/events did not return an event stream")
            # Consume only the initial ping, not the entity dump that follows.
            deadline = time.monotonic() + self.timeout
            for _ in range(32):
                if time.monotonic() >= deadline:
                    break
                line = response.readline(65537)
                if not line or len(line) > 65536:
                    break
                if not line.startswith(b"data:"):
                    continue
                data = json.loads(line[5:])
                uptime = data.get("uptime") if isinstance(data, dict) else None
                if isinstance(uptime, bool) or not isinstance(uptime, (int, float)):
                    continue
                if not math.isfinite(uptime) or uptime < 0:
                    raise CheckFailure("Invalid device uptime")
                if self.uptimes and uptime < self.uptimes[-1]:
                    raise CheckFailure("Device uptime decreased: possible reboot")
                self.uptimes.append(uptime)
                self.reconnects += 1
                return response
            raise CheckFailure("No valid event-stream ping/uptime received")
        except BaseException:
            response.close()
            raise

    def reconnect(self):
        self.stream().close()


class NativeMonitor:
    """Keep API checks and log delivery active while HTTP tests run."""

    def __init__(self, host, port, timeout, noise_psk=None, password=None):
        # ESPHome installs aioesphomeapi; HTTP-only runs need no extra packages.
        from aioesphomeapi import APIClient, LogLevel
        self.client_class, self.levels = APIClient, LogLevel
        self.host, self.port, self.timeout = host, port, timeout
        self.noise_psk, self.password = noise_psk, password
        self.ready, self.stop_event = threading.Event(), threading.Event()
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.result = {"status": "running", "checks": 0, "disconnects": 0,
                       "error_logs": 0, "warnings": 0, "stalled_closures": 0}

    def start(self):
        self.thread.start()
        if not self.ready.wait(self.timeout + 2):
            raise CheckFailure("Native API did not connect within the timeout")
        if self.result.get("failure"):
            raise CheckFailure("Native API connection failed: " + self.result["failure"])

    def stop(self):
        self.stop_event.set()
        if self.thread.ident is not None:
            self.thread.join(self.timeout + 2)
        if self.thread.is_alive():
            self.result["failure"] = "Native monitor did not stop within the timeout"
        self.result["status"] = "failed" if (
            self.result.get("failure") or self.result["disconnects"] or self.result["error_logs"]
        ) else "passed"

    def run(self):
        asyncio.run(self.monitor())

    async def monitor(self):
        client = self.client_class(self.host, self.port, password=self.password,
                                   noise_psk=self.noise_psk, client_info="EspControl device test")

        async def stopped(expected):
            if not expected and not self.stop_event.is_set():
                self.result["disconnects"] += 1

        def received(message):
            text = message.message
            if isinstance(text, bytes):
                text = text.decode("utf-8", errors="replace")
            self.result["stalled_closures"] += int("Closing stuck EventSource connection" in text)
            self.result["warnings"] += int(message.level == self.levels.LOG_LEVEL_WARN)
            self.result["error_logs"] += int(
                message.level == self.levels.LOG_LEVEL_ERROR or
                bool(re.search(r"Guru Meditation|panic|assert failed|stack smashing|watchdog", text, re.I)))

        try:
            await asyncio.wait_for(client.connect(on_stop=stopped, login=True), self.timeout)
            info = await asyncio.wait_for(client.device_info(), self.timeout)
            self.result["firmware"] = {key: getattr(info, key, "") for key in
                                       ("esphome_version", "project_version", "compilation_time")}
            client.subscribe_logs(received, log_level=self.levels.LOG_LEVEL_DEBUG, dump_config=False)
            self.ready.set()
            next_check = 0.0
            while not self.stop_event.is_set():
                if time.monotonic() >= next_check:
                    await asyncio.wait_for(client.device_info(), self.timeout)
                    self.result["checks"] += 1
                    next_check = time.monotonic() + 5
                await asyncio.sleep(0.1)
        except Exception as error:
            # Record the category, not library messages that may contain keys.
            self.result["failure"] = type(error).__name__
        finally:
            self.ready.set()
            try:
                await asyncio.wait_for(client.disconnect(), self.timeout)
            except Exception:
                pass


def run_suite(args, client=None, monitor=None):
    client = client or DeviceHTTP(args.host, args.port, args.timeout,
                                 args.web_username, args.web_password)
    report = {"schema_version": 1, "target": f"{args.host}:{args.port}",
              "firmware_label": args.firmware_label,
              "started_at": datetime.now(timezone.utc).isoformat(), "checks": [],
              "parameters": {key: getattr(args, key) for key in
                             ("reconnects", "stall_seconds", "stalled_clients", "interval", "timeout")},
              "manual_checks": ["Observe the physical screen for blue flashes and verify touch.",
                                "Repeat with the affected configuration and camera/media workload."]}
    before = None
    started = time.monotonic()

    def check(name, action):
        begin = time.monotonic()
        try:
            detail = action()
            report["checks"].append({"name": name, "status": "passed", "detail": detail,
                                     "seconds": round(time.monotonic() - begin, 3)})
            print(f"PASS {name}", flush=True)
            return detail
        except Exception as error:
            report["checks"].append({"name": name, "status": "failed",
                                     "detail": f"{type(error).__name__}: {error}"})
            print(f"FAIL {name}: {type(error).__name__}: {error}", flush=True)
            raise CheckFailure(name) from error

    def preflight():
        nonlocal before
        version = json.loads(client.get("/espcontrol/version"))
        if not isinstance(version, dict) or not isinstance(version.get("device_slug"), str) or not version["device_slug"]:
            raise CheckFailure("Target did not identify an EspControl device profile")
        report["device"] = {key: version.get(key) for key in
                            ("project_name", "project_version", "firmware_version", "device_slug")}
        before = client.get("/api/v1/config")
        if not before:
            raise CheckFailure("Native configuration is empty")
        if not client.get("/"):
            raise CheckFailure("Editor response is empty")
        client.reconnect()
        return "Version, native configuration, editor HTML and event-stream ping are available"

    def burst():
        for _ in range(args.reconnects):
            client.reconnect()
            client.get("/espcontrol/version")
        return f"{args.reconnects} reconnects with an HTTP health check after each"

    def stalls():
        streams = []
        try:
            for _ in range(args.stalled_clients):
                streams.append(client.stream(slow=True))
            deadline = time.monotonic() + args.stall_seconds
            while time.monotonic() < deadline:
                client.get("/espcontrol/version")
                client.reconnect()
                time.sleep(min(args.interval, max(0, deadline - time.monotonic())))
        finally:
            for stream in streams:
                stream.close()
        return f"{args.stalled_clients} non-reading streams held for {args.stall_seconds:g}s"

    def recovery():
        for _ in range(10):
            client.get("/espcontrol/version")
            client.reconnect()
        client.get("/")
        return "Ten reconnects and editor HTML after the stalled clients close"

    def unchanged():
        after = client.get("/api/v1/config")
        if after != before:
            raise CheckFailure("Native configuration changed during the test")
        return "Configuration is byte-for-byte unchanged; no settings were written"

    try:
        if monitor:
            check("native_api_connect", monitor.start)
        check("preflight", preflight)
        check("rapid_reconnects", burst)
        check("stalled_clients", stalls)
        check("post_stall_recovery", recovery)
    except CheckFailure:
        pass
    except KeyboardInterrupt:
        report["interrupted"] = True
    finally:
        if before is not None:
            try:
                check("configuration_preserved", unchanged)
            except CheckFailure:
                pass
        if monitor:
            monitor.stop()
            report["native_api"] = monitor.result.copy()
        else:
            report["native_api"] = {"status": "not_requested"}

    if monitor:
        if monitor.result["status"] != "passed" or monitor.result["checks"] == 0:
            report["checks"].append({"name": "native_api_health", "status": "failed",
                                     "detail": "Native API disconnected, timed out, or logged firmware errors"})
        else:
            report["checks"].append({"name": "native_api_health", "status": "passed",
                                     "detail": f"{monitor.result['checks']} API checks; no disconnects or error logs"})
    closures = monitor.result["stalled_closures"] if monitor else None
    report["stall_cleanup_observed"] = closures if monitor else "not_monitored"
    if args.require_stall_cleanup:
        report["checks"].append({"name": "stall_cleanup_coverage",
                                 "status": "passed" if closures >= 1 else "failed",
                                 "detail": f"Observed {closures} stalled-connection timeout cleanup messages; expected at least one"})
    for item in report["checks"]:
        if item["name"] in ("native_api_health", "stall_cleanup_coverage"):
            print(f"{item['status'].upper()} {item['name']}: {item['detail']}", flush=True)
    report["metrics"] = {"http_checks": client.http_checks, "event_reconnects": client.reconnects,
                          "uptime_start": client.uptimes[0] if client.uptimes else None,
                          "uptime_end": client.uptimes[-1] if client.uptimes else None,
                          "mean_http_ms": round(statistics.mean(client.latencies) * 1000, 1) if client.latencies else None,
                          "max_http_ms": round(max(client.latencies) * 1000, 1) if client.latencies else None}
    report["seconds"] = round(time.monotonic() - started, 3)
    report["status"] = "interrupted" if report.get("interrupted") else (
        "failed" if any(item["status"] == "failed" for item in report["checks"]) else "passed")
    return report


def write_report(report, directory):
    directory.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    path = directory / f"device-web-{stamp}.json"
    path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    device = report.get("device", {})
    native = report["native_api"]
    rows = ["# Device web transport test", "", f"Result: **{report['status'].upper()}**", "",
            f"Target: `{report['target']}`", "",
            f"Device profile: `{device.get('device_slug', 'unknown')}`; firmware: `{device.get('firmware_version', 'unknown')}`.",
            f"Firmware label: {report['firmware_label'] or '(not supplied)'}. "
            f"Compilation time: {native.get('firmware', {}).get('compilation_time', '(not monitored)')}.", "",
            f"Started: {report['started_at']}; duration: {report['seconds']} seconds.",
            f"Native API: {native['status']}; timeout cleanup events: {report['stall_cleanup_observed']}.", "",
            "| Check | Result | Details |", "| --- | --- | --- |"]
    for item in report["checks"]:
        detail = str(item.get("detail") or "").replace("|", "\\|").replace("\n", " ")
        rows.append(f"| {item['name']} | {item['status']} | {detail} |")
    rows += ["", "```json", json.dumps(report["metrics"], indent=2), "```", "", "Manual checks still required:", ""]
    rows += [f"- {value}" for value in report["manual_checks"]]
    path.with_suffix(".md").write_text("\n".join(rows) + "\n", encoding="utf-8")
    return path


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host", required=True, help="Device IP address or hostname (no URL)")
    parser.add_argument("--port", type=int, default=80)
    parser.add_argument("--timeout", type=float, default=5)
    parser.add_argument("--reconnects", type=int, default=80)
    parser.add_argument("--stall-seconds", type=float, default=45)
    parser.add_argument("--stalled-clients", type=int, choices=(1, 2), default=2)
    parser.add_argument("--interval", type=float, default=0.5)
    parser.add_argument("--native-api", action="store_true", help="Monitor native API and firmware logs (requires ESPHome's Python environment)")
    parser.add_argument("--api-port", type=int, default=6053)
    parser.add_argument("--api-noise-psk-env", help="Environment variable holding the existing API encryption key")
    parser.add_argument("--api-password-env", help="Environment variable holding an existing legacy API password")
    parser.add_argument("--web-username-env", help="Environment variable holding the existing web username")
    parser.add_argument("--web-password-env", help="Environment variable holding the existing web password")
    parser.add_argument("--require-stall-cleanup", action="store_true", help="Fail unless native logs prove the stalled-client timeout cleanup path ran")
    parser.add_argument("--firmware-label", default="", help="Label for comparing reports; not a verified device revision")
    parser.add_argument("--report-dir", type=Path, default=Path(".cache/device-web-transport"))
    args = parser.parse_args(argv)
    if any(c in args.host for c in "/:@?#") or not args.host:
        parser.error("--host must be a hostname or IPv4 address, without credentials or a URL")
    for name in ("timeout", "stall_seconds", "interval"):
        if not math.isfinite(getattr(args, name)) or getattr(args, name) <= 0:
            parser.error(f"--{name.replace('_', '-')} must be a finite positive number")
    if not 1 <= args.reconnects <= 10000 or not 1 <= args.port <= 65535 or not 1 <= args.api_port <= 65535:
        parser.error("Use 1–10000 reconnects and ports in 1–65535")
    if args.require_stall_cleanup and not args.native_api:
        parser.error("--require-stall-cleanup needs --native-api")
    if (args.api_noise_psk_env or args.api_password_env) and not args.native_api:
        parser.error("API credentials need --native-api")
    if bool(args.web_username_env) != bool(args.web_password_env):
        parser.error("Supply both web credential environment-variable names")
    for name in ("api_noise_psk", "api_password", "web_username", "web_password"):
        variable = getattr(args, name + "_env")
        if variable and not os.environ.get(variable):
            parser.error(f"Environment variable {variable} is missing or empty")
        setattr(args, name, os.environ[variable] if variable else None)
    return args


def main(argv=None):
    args = parse_args(argv)
    monitor = None
    if args.native_api:
        try:
            monitor = NativeMonitor(args.host, args.api_port, args.timeout, args.api_noise_psk, args.api_password)
        except ImportError:
            print("Native API checks need aioesphomeapi. Run this command in the pinned ESPHome Python environment.")
            return 2
    report = run_suite(args, monitor=monitor)
    path = write_report(report, args.report_dir)
    print(f"{report['status'].upper()}: {report['metrics']['http_checks']} HTTP checks, "
          f"{report['metrics']['event_reconnects']} event-stream reconnects")
    print(f"Reports: {path} and {path.with_suffix('.md')}")
    return 130 if report["status"] == "interrupted" else int(report["status"] != "passed")


if __name__ == "__main__":
    raise SystemExit(main())
