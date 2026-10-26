#!/usr/bin/env python3
"""Offline tests for the live runner. No hardware or ESPHome install required."""
from contextlib import redirect_stdout
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import threading
import asyncio
import io
import json
from pathlib import Path
from types import SimpleNamespace
import tempfile
import time
import unittest
from unittest.mock import patch

import test_device_runtime as runtime


def info(version="2026.9.1", compiled="test-build"):
    return SimpleNamespace(name="test", mac_address="AA:BB:CC:DD:EE:FF", model="fake",
                           esphome_version=version, compilation_time=compiled, project_version="dev")


class EvidenceTests(unittest.TestCase):
    def setUp(self):
        output = redirect_stdout(io.StringIO())
        output.__enter__()
        self.addCleanup(output.__exit__, None, None, None)
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.evidence = runtime.Evidence(Path(self.tmp.name))

    def test_reboot_evidence_is_distinct_from_network_failure(self):
        self.evidence.sample({"hostname": "test", "mac_suffix": "ddeeff", "clock_ms": 100000}, 100)
        self.evidence.sample({"hostname": "test", "mac_suffix": "ddeeff", "clock_ms": 200}, 110)
        self.assertEqual(self.evidence.failures["reboot"], 1)
        self.evidence.event("http_error", failure=True, error="TimeoutError")
        self.assertEqual(self.evidence.failures["reboot"], 1)
        self.assertNotIn("crash_signature", self.evidence.failures)

    def test_clock_wrap_is_not_a_reboot(self):
        self.evidence.sample({"hostname": "test", "mac_suffix": "ddeeff", "clock_ms": 0xFFFFFF00}, 100)
        self.evidence.sample({"hostname": "test", "mac_suffix": "ddeeff", "clock_ms": 9744}, 110)
        self.assertFalse(self.evidence.failures)

    def test_identity_and_firmware_changes_fail(self):
        self.evidence.sample({"hostname": "test", "mac_suffix": "ddeeff", "clock_ms": 10}, 100)
        self.evidence.sample({"hostname": "other", "mac_suffix": "ddeeff", "clock_ms": 20}, 101)
        self.evidence.firmware_info(info())
        self.evidence.firmware_info(info(compiled="different"))
        self.assertEqual(self.evidence.failures["identity_changed"], 1)
        self.assertEqual(self.evidence.failures["firmware_or_device_changed"], 1)

    def test_protocol_identities_match_by_mac_not_hostname(self):
        self.evidence.firmware_info(info())
        for suffix in ("eeff", "ddeeff"):
            self.evidence.sample({"hostname": "http-alias", "mac_suffix": suffix, "clock_ms": 10})
            self.assertTrue(self.evidence.verify_identity())
        self.evidence.sample({"hostname": "test", "mac_suffix": "aabbcc", "clock_ms": 20})
        self.assertFalse(self.evidence.verify_identity())
        self.assertIn("protocol_identity_mismatch", self.evidence.failures)

    def test_missing_or_invalid_mac_identity_cannot_verify(self):
        self.evidence.firmware_info(info())
        for suffix in (None, "", "ff", "not-a-mac"):
            self.evidence.mac_suffix = suffix
            self.assertFalse(self.evidence.verify_identity())
        self.evidence.mac_suffix = "ddeeff"
        self.evidence.firmware["mac_address"] = "invalid"
        self.assertFalse(self.evidence.verify_identity())

    def test_requested_firmware_version_and_build_must_match(self):
        self.evidence.firmware_info(info(), expected_version='wrong', expected_build='wrong')
        self.assertEqual(self.evidence.failures['wrong_firmware_version'],1)
        self.assertEqual(self.evidence.failures['wrong_build_timestamp'],1)

    def test_nonfinite_memory_and_pressure(self):
        self.evidence.memory("heap_free_percent", float("nan"))
        self.assertFalse(self.evidence.metrics)
        self.evidence.memory("heap_free_percent", 4)
        self.assertEqual(self.evidence.failures["memory_pressure"], 1)

    def test_crash_patterns_do_not_match_normal_watchdog_configuration(self):
        for text in ("Guru Meditation Error", "CORRUPT HEAP", "assert failed: x",
                     "[E][test:10]: Backtrace: 0x123", "Stack canary watchpoint triggered",
                     "task_wdt: Task watchdog got triggered"):
            self.assertRegex(text, runtime.CRASH)
        self.assertIsNone(runtime.CRASH.search("Watchdog timeout configured to 30s"))

    def test_sensitive_log_values_are_not_saved(self):
        text = runtime.safe_log("Backtrace: token=secret https://user:pass@host/path?key=secret")
        self.assertNotIn("secret", text)
        self.assertNotIn("pass@", text)

    def test_missing_coverage_cannot_pass(self):
        args = SimpleNamespace(host="fake", duration=1, modal_entity=None, modal_cycles=20)
        report = runtime.final_report(self.evidence, args)
        self.assertEqual(report["status"], "failed")
        self.assertIn("incomplete_http_coverage", report["failures"])
        self.assertIn("missing_memory_diagnostics", report["failures"])
        self.assertTrue((Path(self.tmp.name)/"report.md").is_file())


class ProtocolTests(unittest.TestCase):
    def test_sse_clock_and_unrelated_payloads(self):
        data = b'\r\nretry: 30000\nid: 12345\nevent: ping\ndata: {"config":"private"}\n\n'
        self.assertEqual(runtime.read_boot_clock(io.BytesIO(data)), 12345)

    def test_malformed_and_oversized_sse_fail(self):
        for data in (b'event: ping\nid: nope\n\n', b'id: -1\nevent: ping\n\n',
                     b'id: 4294967296\nevent: ping\n\n', b'x'*4097, b'event: state\n\n'):
            with self.assertRaises(ValueError): runtime.read_boot_clock(io.BytesIO(data))

    def test_real_http_probe_validates_identity_capabilities_and_sse(self):
        class Handler(BaseHTTPRequestHandler):
            version = 1
            suffix = 'ddeeff'
            def log_message(self, *args): pass
            def do_GET(self):
                bodies = {
                    '/api/v1/identity': json.dumps({'hostname':'test','mac_suffix':self.suffix,'restart_required':False}).encode(),
                    '/api/v1/capabilities': json.dumps({'api':{'version':self.version}}).encode(),
                    '/events': b'id: 12345\nevent: ping\n\n',
                }
                self.send_response(200)
                self.send_header('Content-Type', 'text/event-stream' if self.path=='/events' else 'application/json')
                self.end_headers()
                self.wfile.write(bodies[self.path])
        server = ThreadingHTTPServer(('127.0.0.1',0),Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            probe = runtime.HttpProbe('127.0.0.1',server.server_port,1)
            self.assertEqual(probe.sample(),{'hostname':'test','mac_suffix':'ddeeff','clock_ms':12345})
            Handler.version = 999
            with self.assertRaisesRegex(ValueError, 'unsupported device API'): probe.sample()
            Handler.version = 1
            for suffix in (None, '', 'invalid'):
                Handler.suffix = suffix
                with self.assertRaisesRegex(ValueError, 'MAC suffix'): probe.sample()
        finally:
            server.shutdown()
            server.server_close()
            thread.join()

    def test_cli_requires_explicit_device_and_preserves_reports(self):
        with redirect_stdout(io.StringIO()), patch('sys.stderr', io.StringIO()):
            for argv in ([], ['--host','http://fake'], ['--host','fake','--duration','nan'],
                         ['--host','fake','--modal-entity','bad entity'], ['--host','fake','--api-port','0']):
                with self.assertRaises(SystemExit): runtime.arguments(argv)
            with tempfile.TemporaryDirectory() as directory:
                with self.assertRaises(SystemExit): runtime.arguments(['--host','fake','--output',directory])


class FakeProbe:
    failures = 0
    mac_suffix = "ddeeff"
    def __init__(self, *args): pass
    def sample(self):
        if FakeProbe.failures:
            FakeProbe.failures -= 1
            raise TimeoutError()
        return {"hostname":"test", "mac_suffix":self.mac_suffix, "clock_ms":int(time.monotonic()*1000) & 0xFFFFFFFF}


class RunnerTests(unittest.IsolatedAsyncioTestCase):
    async def run_fake(self, *, crash=False, disconnected=False, acknowledge=True,
                       modal=True, duration=.15, cycles=2, dwell=.005, memory=True, http_mac="ddeeff", change_identity_after_open=False, close_delay=0,
                       record_close=False, cancel_run=False, cancel_close=False, timeout=.05):
        tmp = tempfile.TemporaryDirectory()
        self.addCleanup(tmp.cleanup)
        calls=[]
        suite_task=None
        levels = SimpleNamespace(LOG_LEVEL_INFO=3, LOG_LEVEL_WARN=2)
        class Client:
            def __init__(self, *args, **kwargs): self.connected=False
            async def connect(self, on_stop, **kwargs):
                self.on_stop=on_stop; self.connected=True
            async def device_info(self): return info()
            async def list_entities_services(self):
                return [SimpleNamespace(key=1,object_id='memory__heap_free')], [
                    SimpleNamespace(name='open_modal',supports_response=0),SimpleNamespace(name='close_modal',supports_response=0)]
            def subscribe_states(self, cb):
                if memory: cb(SimpleNamespace(key=1,state=40,missing_state=False))
                cb(SimpleNamespace(key=99,state='secret config'))
            def subscribe_logs(self, cb, **kwargs):
                self.log=cb
                if crash: cb(SimpleNamespace(message=b'Guru Meditation Error: test',level=1))
                if disconnected:
                    asyncio.create_task(self.on_stop(False))
            async def execute_service(self, service, data, **kwargs):
                if not self.connected: raise ConnectionError()
                assert kwargs['return_response'] is None, 'These actions do not advertise responses'
                calls.append(service.name)
                if service.name=='close_modal' and cancel_close:
                    raise asyncio.CancelledError()
                if service.name=='close_modal' and close_delay:
                    if cancel_run:
                        asyncio.get_running_loop().call_later(close_delay / 2, suite_task.cancel)
                    await asyncio.sleep(close_delay)
                    if not self.connected: raise ConnectionError()
                if service.name=='close_modal' and record_close:
                    calls.append('close_complete')
                if service.name=='open_modal' and change_identity_after_open:
                    FakeProbe.mac_suffix = 'aabbcc'
                if service.name=='open_modal' and acknowledge:
                    self.log(SimpleNamespace(message=b'[I][open_modal:122]: Opened media_player.office (slot 1, subcard 0)',level=3))
                return SimpleNamespace(success=True)
            async def disconnect(self):
                calls.append('disconnect'); self.connected=False
                await self.on_stop(True)
        args=SimpleNamespace(host='fake',http_port=80,api_port=6053,timeout=timeout,
            output=Path(tmp.name)/'results',min_heap_percent=5,interval=.005,duration=duration,
            expect_version='2026.9.1',expect_build='test-build',
            modal_entity='media_player.office' if modal else None,
            modal_cycles=cycles,modal_dwell=dwell)
        with patch.dict('sys.modules',{'aioesphomeapi':SimpleNamespace(LogLevel=levels)}), \
             patch.object(runtime,'HttpProbe',FakeProbe), patch.object(FakeProbe,'mac_suffix',http_mac), redirect_stdout(io.StringIO()):
            suite_task=asyncio.create_task(runtime.run_suite(args,Client))
            code=await suite_task
        report=json.loads((args.output/'report.json').read_text())
        events=(args.output/'events.jsonl').read_text()
        self.assertNotIn('secret config',events)
        return code, report, calls

    async def test_success_requires_actual_modal_open_log_and_health(self):
        code,report,calls=await self.run_fake()
        self.assertEqual(code,0,report)
        self.assertEqual(report['counts']['modal_cycle'],2)
        self.assertEqual(calls.count('open_modal'),2)
        self.assertEqual(calls.count('close_modal'),2)
        self.assertFalse(report['failures'])

    async def test_mismatched_protocols_fail_before_any_modal_action(self):
        code,report,calls=await self.run_fake(http_mac="aabbcc")
        self.assertEqual(code,1)
        self.assertIn('protocol_identity_mismatch',report['failures'])
        self.assertNotIn('open_modal',calls)
        self.assertNotIn('close_modal',calls)
        self.assertNotIn('identity_verified',report['counts'])

    async def test_identity_change_closes_owned_modal_before_disconnect(self):
        for close_delay in (0, .02):
            with self.subTest(close_delay=close_delay):
                code,report,calls=await self.run_fake(
                    change_identity_after_open=True, dwell=10, close_delay=close_delay)
                self.assertEqual(code,1)
                self.assertIn('protocol_identity_mismatch',report['failures'])
                self.assertEqual(calls,['open_modal','close_modal','disconnect'])
                self.assertNotIn('modal_cleanup_incomplete',report['failures'])
                self.assertNotIn('modal_error',report['failures'])

    async def test_matching_protocol_identity_is_in_report(self):
        code,report,_=await self.run_fake(modal=False)
        self.assertEqual(code,0,report)
        self.assertEqual(report['http_identity']['mac_suffix'],'ddeeff')
        self.assertGreater(report['counts']['identity_verified'],0)

    async def test_default_passive_mode_sends_no_modal_actions(self):
        code,report,calls=await self.run_fake(modal=False)
        self.assertEqual(code,0,report)
        self.assertEqual(calls,['disconnect'])

    async def test_service_ack_without_opened_modal_cannot_pass(self):
        code,report,calls=await self.run_fake(acknowledge=False)
        self.assertEqual(code,1)
        self.assertIn('modal_error',report['failures'])
        self.assertNotIn('modal_cycle',report['counts'])
        self.assertIn('close_modal',calls)

    async def test_timeout_closes_modal_before_disconnecting(self):
        code,report,calls=await self.run_fake(duration=.03,dwell=10,cycles=10)
        self.assertEqual(code,1)
        self.assertIn('incomplete_modal_coverage',report['failures'])
        self.assertEqual(calls,['open_modal','close_modal','disconnect'])

    async def test_native_crash_log_fails_even_when_http_works(self):
        code,report,_=await self.run_fake(crash=True,modal=False)
        self.assertEqual(code,1)
        self.assertIn('crash_signature',report['failures'])

    async def test_duration_expiring_during_close_allows_completion(self):
        code,report,calls=await self.run_fake(duration=.06,cycles=1,close_delay=.15,record_close=True,timeout=.5)
        self.assertEqual(code,0,report)
        self.assertEqual(calls,['open_modal','close_modal','close_complete','disconnect'])
        self.assertNotIn('modal_cleanup_incomplete',report['failures'])

    async def test_interruption_during_close_allows_completion(self):
        code,report,calls=await self.run_fake(duration=1,cycles=1,close_delay=.15,record_close=True,cancel_run=True,timeout=.5)
        self.assertEqual(code,1)
        self.assertEqual(calls,['open_modal','close_modal','close_complete','disconnect'])
        self.assertIn('interrupted',report['failures'])
        self.assertNotIn('modal_cleanup_incomplete',report['failures'])

    async def test_cancelled_close_is_recorded_as_incomplete_cleanup(self):
        code,report,_=await self.run_fake(duration=.04,cycles=1,cancel_close=True)
        self.assertEqual(code,1)
        self.assertIn('modal_cleanup_incomplete',report['failures'])

    async def test_native_disconnect_is_failure_without_claiming_a_crash(self):
        code,report,_=await self.run_fake(disconnected=True,modal=False)
        self.assertEqual(code,1)
        self.assertIn('native_disconnect',report['failures'])
        self.assertNotIn('crash_signature',report['failures'])

    async def test_recovered_http_failure_still_fails(self):
        FakeProbe.failures=1
        code,report,_=await self.run_fake(modal=False)
        self.assertEqual(code,1)
        self.assertIn('http_error',report['failures'])
        self.assertNotIn('reboot',report['failures'])

    async def test_missing_diagnostic_states_cannot_pass(self):
        code,report,_=await self.run_fake(memory=False,modal=False)
        self.assertEqual(code,1)
        self.assertIn('missing_memory_diagnostics',report['failures'])


if __name__=='__main__':
    unittest.main()
