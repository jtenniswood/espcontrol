"""Test the hardware runner against a local fake device; never contact a LAN panel."""
from contextlib import contextmanager, redirect_stdout
import base64
import http.server
import io
import json
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
import test_device_web_transport as runner


class Device(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        state = self.server.state
        state["requests"].append(self.path)
        if state.get("auth") and self.headers.get("Authorization") != "Basic " + base64.b64encode(state["auth"].encode()).decode():
            self.send_response(401)
            self.send_header("WWW-Authenticate", 'Basic realm="EspControl"')
            self.send_header("Content-Length", "0")
            self.end_headers()
            return
        if state.get("unavailable"):
            self.send_error(503)
            return
        if self.path == "/espcontrol/version":
            body = json.dumps({"device_slug": "test-panel", "version": "dev"}).encode()
        elif self.path == "/api/v1/config":
            state["config_reads"] += 1
            body = b"private configuration"
            if state.get("config_changes") and state["config_reads"] > 1:
                body += b"changed"
        elif self.path == "/":
            body = b"<html>EspControl editor</html>"
        elif self.path == "/events":
            state["streams"] += 1
            uptime = state["streams"] + 100
            if state.get("reboots") and state["streams"] > 2:
                uptime = 1
            if state.get("missing_ping"):
                body = b"event: state\ndata: {}\n\n"
            else:
                body = f'event: ping\ndata: {{"uptime":{uptime}}}\n\n'.encode()
            self.send_response(200)
            self.send_header("Content-Type", "text/event-stream")
            self.send_header("Transfer-Encoding", "chunked")
            self.end_headers()
            try:
                self.wfile.write(f"{len(body):x}\r\n".encode() + body + b"\r\n0\r\n\r\n")
                self.wfile.flush()
            except (BrokenPipeError, ConnectionResetError):
                pass
            return
        else:
            self.send_error(404)
            return
        self.send_response(200)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_POST(self):
        raise AssertionError("The suite must never write configuration or invoke controls")

    do_PUT = do_POST

    def log_message(self, *args):
        pass


@contextmanager
def device(**options):
    server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), Device)
    server.state = {"requests": [], "config_reads": 0, "streams": 0, **options}
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    try:
        yield server
    finally:
        server.shutdown()
        server.server_close()
        thread.join(2)


class Monitor:
    def __init__(self, **values):
        self.started = self.stopped = False
        self.result = {"status": "passed", "checks": 2, "disconnects": 0,
                       "error_logs": 0, "stalled_closures": 2, **values}

    def start(self):
        self.started = True

    def stop(self):
        self.stopped = True


class DeviceWebTests(unittest.TestCase):
    def args(self, server, *extra):
        return runner.parse_args(["--host", "127.0.0.1", "--port", str(server.server_port),
                                  "--reconnects", "3", "--stall-seconds", "0.02",
                                  "--interval", "0.01", "--timeout", "1", *extra])

    def run_device(self, server, monitor=None, *extra):
        with redirect_stdout(io.StringIO()):
            return runner.run_suite(self.args(server, *extra), monitor=monitor)

    def test_chunked_streams_health_and_configuration_preservation(self):
        with device() as server:
            report = self.run_device(server)
            self.assertEqual(report["status"], "passed")
            self.assertGreaterEqual(report["metrics"]["event_reconnects"], 16)
            self.assertEqual(server.state["config_reads"], 2)
            self.assertEqual(set(server.state["requests"]),
                             {"/", "/events", "/espcontrol/version", "/api/v1/config"})
            self.assertEqual(report["native_api"]["status"], "not_requested")

    def test_configuration_change_fails(self):
        with device(config_changes=True) as server:
            report = self.run_device(server)
            self.assertEqual(report["status"], "failed")
            self.assertEqual(report["checks"][-1]["name"], "configuration_preserved")

    def test_reboot_fails_and_still_checks_configuration(self):
        with device(reboots=True) as server:
            report = self.run_device(server)
            self.assertEqual(report["status"], "failed")
            self.assertIn("uptime decreased", str(report["checks"]))
            self.assertEqual(server.state["config_reads"], 2)
            self.assertNotIn("stalled_clients", [item["name"] for item in report["checks"]])

    def test_unreachable_and_missing_ping_do_not_pass_vacuously(self):
        for options in ({"unavailable": True}, {"missing_ping": True}):
            with self.subTest(options=options), device(**options) as server:
                report = self.run_device(server)
                self.assertEqual(report["status"], "failed")
                self.assertEqual(report["metrics"]["event_reconnects"], 0)

    def test_native_errors_and_missing_cleanup_coverage_fail(self):
        for values in ({"status": "failed", "error_logs": 1}, {"checks": 0}, {"stalled_closures": 0}):
            with self.subTest(values=values), device() as server:
                monitor = Monitor(**values)
                report = self.run_device(server, monitor, "--native-api", "--require-stall-cleanup")
                self.assertEqual(report["status"], "failed")
                self.assertTrue(monitor.started and monitor.stopped)

    def test_interrupt_closes_stalled_streams_and_preserves_report(self):
        with device() as server:
            args = self.args(server)
            client = runner.DeviceHTTP(args.host, args.port, args.timeout)
            original = client.stream
            held = []

            def interrupted(slow=False):
                if slow and held:
                    raise KeyboardInterrupt
                stream = original(slow)
                if slow:
                    held.append(stream)
                return stream

            monitor = Monitor()
            with patch.object(client, "stream", interrupted), redirect_stdout(io.StringIO()):
                report = runner.run_suite(args, client, monitor)
            self.assertEqual(report["status"], "interrupted")
            self.assertTrue(held and all(stream.closed for stream in held))
            self.assertTrue(monitor.stopped)
            self.assertEqual(server.state["config_reads"], 2)

    def test_one_timeout_message_proves_cleanup_path_coverage(self):
        with device() as server:
            report = self.run_device(server, Monitor(stalled_closures=1),
                                     "--native-api", "--require-stall-cleanup")
            self.assertEqual(report["status"], "passed")
            self.assertEqual(report["stall_cleanup_observed"], 1)

    def test_reports_and_exit_status_include_failures_without_configuration(self):
        with device(config_changes=True) as server, tempfile.TemporaryDirectory() as tmp:
            args = self.args(server)
            with redirect_stdout(io.StringIO()):
                code = runner.main(["--host", args.host, "--port", str(args.port),
                                    "--reconnects", "1", "--stall-seconds", "0.01",
                                    "--interval", "0.01", "--report-dir", tmp])
            self.assertEqual(code, 1)
            json_path, = Path(tmp).glob("*.json")
            self.assertEqual(json.loads(json_path.read_text())["status"], "failed")
            self.assertIn("configuration_preserved", json_path.with_suffix(".md").read_text())
            self.assertNotIn("private configuration", json_path.read_text())

    def test_invalid_targets_and_timings_are_rejected(self):
        for options in (["--host", "http://example.test"], ["--host", "user:pass@device"],
                        ["--timeout", "nan"], ["--interval", "0"], ["--stall-seconds", "inf"],
                        ["--reconnects", "0"], ["--require-stall-cleanup"], ["--port", "0"]):
            with self.subTest(options=options), redirect_stdout(io.StringIO()), patch("sys.stderr", io.StringIO()):
                with self.assertRaises(SystemExit) as error:
                    runner.parse_args(["--host", "127.0.0.1", *options])
                self.assertEqual(error.exception.code, 2)

    def test_credentials_are_not_serialized(self):
        with device() as server, patch.dict("os.environ", {"TEST_DEVICE_KEY": "private-api-key"}):
            args = self.args(server, "--native-api", "--api-noise-psk-env", "TEST_DEVICE_KEY")
            with redirect_stdout(io.StringIO()):
                report = runner.run_suite(args, monitor=Monitor())
            self.assertNotIn("private-api-key", json.dumps(report))

    def test_authenticated_requests_and_streams_use_existing_credentials(self):
        with device(auth="tester:private-password") as server, patch.dict("os.environ", {
                "TEST_DEVICE_USER": "tester", "TEST_DEVICE_PASSWORD": "private-password"}):
            report = self.run_device(server, None, "--web-username-env", "TEST_DEVICE_USER",
                                     "--web-password-env", "TEST_DEVICE_PASSWORD")
            self.assertEqual(report["status"], "passed")
            self.assertNotIn("private-password", json.dumps(report))


if __name__ == "__main__":
    unittest.main()
