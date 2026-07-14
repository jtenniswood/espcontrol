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
        write(repo, "src/webserver/cards/light_temperature.ts", "export const type = 'light';\n")
        commit(repo, "Add light brightness card type (#12)")
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
    assert "## What changed?" in text
    assert "- User-facing features: 1 change. Notable: Add light brightness card type." in text
    assert "## Update guidance" in text
    assert "Recommended for users who want the latest firmware, setup page, device fixes, or documented behavior." in text
    assert "### User-facing features" in text
    assert "Add light brightness card type" in text
    assert "[#12](https://github.com/example/espcontrol/pull/12)" in text
    assert "1 user-facing change is included in this release." in text
    assert "Release range: `v1.0.0` to `" in text
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
    assert "Affected devices: All supported displays may be affected" in text
    assert "Release range: `v1.0.0` to `v1.1.0`." in text
    assert "### User-facing bug fixes" in text
    assert "Fix relay card behavior" in text


def test_device_build_change_reports_specific_device() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "builds/guition-esp32-p4-jc1060p470.yaml", "substitutions:\n  name: test\n")
        commit(repo, "Fix 7inch P4 display build")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Affected devices: `guition-esp32-p4-jc1060p470`" in text
    assert "All supported displays may be affected" not in text


def test_generated_web_bundle_change_reports_specific_device() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "docs/public/webserver/guition-esp32-p4-jc1060p470/www.js", "console.log('web');\n")
        commit(repo, "Fix 7inch P4 setup page")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Affected devices: `guition-esp32-p4-jc1060p470`" in text
    assert "All supported displays may be affected" not in text


def test_shared_display_assets_are_user_facing() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "common/assets/icon_glyphs.yaml", "thermostat: '\\ue000'\n")
        commit(repo, "Add thermostat icon glyphs")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Add thermostat icon glyphs" in text
    assert "1 user-facing change is included in this release." in text
    assert "Affected devices: All supported displays may be affected" in text


def test_device_manifest_change_affects_all_devices() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "devices/manifest.json", '{"devices":[]}\n')
        commit(repo, "Update supported display manifest")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Update supported display manifest" in text
    assert "Affected devices: All supported displays may be affected" in text


def test_public_device_profile_change_is_user_facing() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "docs/public/device-profiles.json", '{"devices":[]}\n')
        commit(repo, "Update install device profiles")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Update install device profiles" in text
    assert "1 user-facing change is included in this release." in text
    assert "Affected devices: All supported displays may be affected" in text


def test_docs_prefixed_public_device_profile_change_is_user_facing() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "docs/public/device-profiles.json", '{"devices":[]}\n')
        commit(repo, "docs: update device profiles")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "docs: update device profiles" in text
    assert "1 user-facing change is included in this release." in text
    assert "Optional update" not in text


def test_public_release_helper_changes_are_user_facing() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, "scripts/firmware_release.py", "# firmware release metadata\n")
        commit(repo, "Fix firmware release manifest URLs")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "Fix firmware release manifest URLs" in text
    assert "1 user-facing change is included in this release." in text
    assert "Optional update" not in text


def test_internal_changes_are_not_listed_as_user_facing() -> None:
    tmp, repo = with_temp_repo()
    original_root = release_changelog.ROOT
    try:
        release_changelog.ROOT = repo
        write(repo, ".github/workflows/ci.yml", "name: CI\n")
        commit(repo, "ci: tighten release checks")
        write(repo, "scripts/check_release_changelog.py", "# test\n")
        commit(repo, "Update release changelog tests")
        text = release_changelog.build_changelog(
            "v1.1.0",
            release_changelog.default_from_ref("v1.1.0", "HEAD"),
            "HEAD",
            None,
        )
    finally:
        release_changelog.ROOT = original_root
        tmp.cleanup()

    assert "No user-facing features or bug fixes were detected" in text
    assert "0 user-facing changes are included in this release." in text
    assert "ci: tighten release checks" not in text
    assert "Update release changelog tests" not in text


def main() -> int:
    test_future_release_uses_latest_tag()
    test_existing_tag_uses_previous_tag()
    test_device_build_change_reports_specific_device()
    test_generated_web_bundle_change_reports_specific_device()
    test_shared_display_assets_are_user_facing()
    test_device_manifest_change_affects_all_devices()
    test_public_device_profile_change_is_user_facing()
    test_public_release_helper_changes_are_user_facing()
    test_internal_changes_are_not_listed_as_user_facing()
    print("Release changelog tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
