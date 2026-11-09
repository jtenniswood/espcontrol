#!/usr/bin/env python3
"""Regression checks for native Windows and POSIX V3 Docker commands."""
import types
import unittest
from pathlib import PurePosixPath, PureWindowsPath
from unittest.mock import patch

import build_jc8012p4a1_v3_test as builder


class DockerCommandTests(unittest.TestCase):
    def command(self, path_type, root, common, git_dir, host_os):
        with patch.object(builder, "os", host_os):
            return builder.docker_command(
                path_type(root), path_type(root) / "cache with spaces",
                path_type(common), path_type(git_dir), path_type(root) / "container.git",
            )

    def test_windows_checkout_without_posix_user_functions(self):
        command = self.command(
            PureWindowsPath, "C:/Users/Test User/espcontrol",
            "C:/Users/Test User/espcontrol/.git", "C:/Users/Test User/espcontrol/.git",
            types.SimpleNamespace(name="nt"),
        )
        self.assertNotIn("--user", command)
        self.assertIn("C:\\Users\\Test User\\espcontrol:/config", command)
        self.assertIn("C:\\Users\\Test User\\espcontrol\\cache with spaces:/idf-cache", command)
        self.assertFalse(any(arg.startswith("GIT_") for arg in command))
        self.assertIn("ESPHOME_ESP_IDF_PREFIX=/idf-cache", command)

    def test_windows_linked_worktree_preserves_its_head(self):
        command = self.command(
            PureWindowsPath, "D:/test worktree", "C:/repo/.git",
            "C:/repo/.git/worktrees/test-v3", types.SimpleNamespace(name="nt"),
        )
        self.assertIn("C:\\repo\\.git:/git-metadata:ro", command)
        self.assertIn("D:\\test worktree:/config", command)
        self.assertIn("D:\\test worktree\\container.git:/config/.git:ro", command)

    def test_posix_preserves_host_user_and_linked_worktree(self):
        command = self.command(
            PurePosixPath, "/tmp/test worktree", "/repo/.git",
            "/repo/.git/worktrees/test-v3",
            types.SimpleNamespace(name="posix", getuid=lambda: 501, getgid=lambda: 20),
        )
        self.assertEqual(command[:5], ["docker", "run", "--rm", "--user", "501:20"])
        self.assertIn("/repo/.git:/git-metadata:ro", command)
        self.assertIn("/tmp/test worktree/container.git:/config/.git:ro", command)

    def test_posix_regular_checkout(self):
        command = self.command(
            PurePosixPath, "/repo", "/repo/.git", "/repo/.git",
            types.SimpleNamespace(name="posix", getuid=lambda: 1000, getgid=lambda: 1000),
        )
        self.assertFalse(any("/git-metadata" in arg for arg in command))
        self.assertFalse(any(arg.startswith("GIT_") for arg in command))


if __name__ == "__main__":
    unittest.main()
