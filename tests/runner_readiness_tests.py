"""Local contract tests; no real runner, mount, credential or Docker access."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location(
    "runner_readiness", Path(__file__).resolve().parents[1] / "scripts/check_runner_readiness.py")
assert SPEC is not None and SPEC.loader is not None
readiness = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(readiness)


class ReadinessTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name) / "work"
        self.temp = Path(self.tmp.name) / "temp"
        self.work.mkdir()
        self.temp.mkdir()
        self.hook = Path(self.tmp.name) / "cleanup.sh"
        self.hook.write_text("#!/bin/sh\nexit 0\n")
        self.hook.chmod(0o755)
        self.env = {
            "GITHUB_REPOSITORY": "owner/project", "GITHUB_WORKSPACE": str(self.work),
            "GITHUB_SHA": "a" * 40, "RUNNER_TEMP": str(self.temp),
            "ACTUAL_RUNNER_NAME": "github-runner-project", "ACTUAL_RUNNER_OS": "Linux",
            "ACTUAL_RUNNER_ARCH": "X64", "RUNNER_LABELS": "project,runner,github-runner-project",
            "RUNNER_CLEANUP_WORKSPACE": "true", "RUNNER_CLEANUP_TMP": "true",
            "RUNNER_CLEANUP_DOCKER": "false", "ACTIONS_RUNNER_HOOK_JOB_COMPLETED": str(self.hook),
            "DOCKER_HOST": "unix:///var/run/docker.sock",
        }
        self.commands = []

    def command(self, args):
        self.commands.append(args)
        if args[0] == "git":
            return str(self.work) if args[-1] == "--show-toplevel" else "a" * 40
        if args[0] == "findmnt":
            return '{"filesystems":[{"fstype":"ext4","source":"/dev/loop7[work]","size":21000000000,"maj:min":"7:7"}]}'
        if args[0] == "docker":
            return "29.0.0"
        raise AssertionError(args)

    def check(self):
        with patch.dict(os.environ, self.env, clear=True), \
             patch.object(readiness, "command", side_effect=self.command), \
             patch.object(readiness, "is_socket", return_value=True), \
             patch.object(readiness.platform, "system", return_value="Linux"), \
             patch.object(readiness.platform, "machine", return_value="x86_64"):
            readiness.verify("owner/project", "github-runner-project")

    def test_current_contract_passes_and_only_own_probes_removed(self):
        unrelated = self.temp / "unrelated"
        unrelated.write_text("keep")
        self.check()
        self.assertEqual(list(self.work.iterdir()), [])
        self.assertEqual(list(self.temp.iterdir()), [unrelated])
        docker = [cmd for cmd in self.commands if cmd[0] == "docker"]
        self.assertEqual(len(docker), 1)
        self.assertIn("version", docker[0])
        self.assertNotIn("run", docker[0])

    def test_wrong_repository_or_runner_refused(self):
        for key in ["GITHUB_REPOSITORY", "ACTUAL_RUNNER_NAME", "ACTUAL_RUNNER_OS", "ACTUAL_RUNNER_ARCH"]:
            with self.subTest(key=key):
                old = self.env[key]
                self.env[key] = "wrong"
                with self.assertRaises(RuntimeError): self.check()
                self.env[key] = old

    def test_old_labels_and_dind_refused(self):
        for key, value in [("RUNNER_LABELS", "project,server,dev"),
                           ("DOCKER_HOST", "tcp://github-runner-docker:2375"),
                           ("RUNNER_CLEANUP_DOCKER", "true")]:
            with self.subTest(key=key):
                old = self.env[key]
                self.env[key] = value
                with self.assertRaises(RuntimeError): self.check()
                self.env[key] = old

    def test_bootstrap_token_presence_refused_without_printing_value(self):
        for value in ["", "test-value-not-a-credential"]:
            self.env["ACCESS_TOKEN"] = value
            with self.assertRaisesRegex(RuntimeError, "bootstrap") as raised: self.check()
            self.assertNotIn("test-value-not-a-credential", str(raised.exception))

    def test_non_executable_hook_refused(self):
        self.hook.chmod(0o644)
        with self.assertRaises(RuntimeError): self.check()

    def test_storage_drift_refused(self):
        real_command = self.command
        for change in [{"fstype": "tmpfs"}, {"size": 50000000000}, {"source": "/dev/sda1"},
                       {"size": 1000000000}, {"maj:min": "8:1"}]:
            with self.subTest(change=change):
                def altered(args):
                    import json
                    result = real_command(args)
                    if args[0] == "findmnt" and ("maj:min" not in change or args[-1] == str(self.temp)):
                        data = json.loads(result)
                        data["filesystems"][0].update(change)
                        return json.dumps(data)
                    return result
                with patch.object(self, "command", side_effect=altered):
                    with self.assertRaises(RuntimeError): self.check()

    def test_checkout_mismatch_refused(self):
        self.env["GITHUB_SHA"] = "b" * 40
        with self.assertRaises(RuntimeError): self.check()

    def test_missing_socket_refused(self):
        with patch.object(readiness, "is_socket", return_value=False):
            # check() sets the positive fixture; directly exercise the predicate helper instead.
            with patch.dict(os.environ, self.env, clear=True), \
                 patch.object(readiness, "command", side_effect=self.command), \
                 patch.object(readiness.platform, "system", return_value="Linux"), \
                 patch.object(readiness.platform, "machine", return_value="x86_64"):
                with self.assertRaises(RuntimeError): readiness.verify("owner/project", "github-runner-project")


if __name__ == "__main__":
    unittest.main()
