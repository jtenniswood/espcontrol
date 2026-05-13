#!/usr/bin/env python3
"""Smoke tests for scripts/release_changelog.py."""

from __future__ import annotations

import subprocess
from pathlib import Path
from tempfile import TemporaryDirectory

import release_changelog


def git(repo: Path, *args: str) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=repo,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout.strip()


def write(repo: Path, path: str, text: str) -> None:
    target = repo / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)


def commit(repo: Path, subject: str) -> str:
    git(repo, "add", ".")
    git(repo, "commit", "-m", subject)
    return git(repo, "rev-parse", "--short", "HEAD")


def with_temp_repo() -> tuple[TemporaryDirectory[str], Path]:
    tmp = TemporaryDirectory()
    repo = Path(tmp.name)
    git(repo, "init", "-b", "main")
    git(repo, "config", "user.email", "test@example.com")
    git(repo, "config", "user.name", "Test User")
    git(repo, "remote", "add", "origin", "https://github.com/example/espcontrol.git")
    write(repo, "README.md", "# Demo\n")
    commit(repo, "Initial release")
    git(repo, "tag", "v1.0.0")
    return tmp, repo


def test_future_release_uses_latest_tag() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "src/webserver/types/light_temperature.js", "export const type = 'light';\n")
        short_hash = commit(repo, "Add light brightness card type (#12)")
        full_hash = git(repo, "rev-parse", "HEAD")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            release_changelog.remote_url(),
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Changes since `v1.0.0`." in text
    assert "### Controls and setup page" in text
    assert "Add light brightness card type" in text
    assert f"[{short_hash}]" in text
    assert "[#12](https://github.com/example/espcontrol/pull/12)" in text
    assert f"Release range: `v1.0.0` to `{short_hash} (HEAD)`." in text
    assert f"[Full comparison](https://github.com/example/espcontrol/compare/v1.0.0...{full_hash})" in text


def test_existing_tag_uses_previous_tag() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "components/espcontrol/button_grid.h", "// firmware\n")
        commit(repo, "Fix relay card behavior")
        git(repo, "tag", "v1.1.0")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "v1.1.0"),
            "v1.1.0",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Changes since `v1.0.0`." in text
    assert "Release range: `v1.0.0` to `v1.1.0`." in text
    assert "### Firmware and device behavior" in text
    assert "Fix relay card behavior" in text


def main() -> int:
    test_future_release_uses_latest_tag()
    test_existing_tag_uses_previous_tag()
    print("Release changelog tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
