#!/usr/bin/env python3
"""Smoke tests for scripts/firmware_release.py."""

from __future__ import annotations

from contextlib import redirect_stderr
from functools import partial
import hashlib
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import io
import json
from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory
from threading import Thread

import firmware_release
import prepare_c6_firmware


SLUG = "guition-esp32-s3-4848s040"
RECOVERY_SLUG = "guition-esp32-p4-jc4880p443"
VERSION = "v9.8.7"
CHIP = "ESP32-S3"
PROJECT_NAME = "jtenniswood.espcontrol"
ESPHOME_ENV = Path(__file__).resolve().parents[1] / ".github" / "esphome.env"
RELEASE_WORKFLOW = Path(__file__).resolve().parents[1] / ".github" / "workflows" / "release.yml"
PAGES_WORKFLOW = Path(__file__).resolve().parents[1] / ".github" / "workflows" / "pages.yml"
ROOT = Path(__file__).resolve().parents[1]
RELEASE_SKILL = (
    Path(__file__).resolve().parents[1]
    / ".agents"
    / "skills"
    / "release"
    / "SKILL.md"
)
ESPHOME_ENV_RE = re.compile(r"^ESPHOME_VERSION=20[0-9]{2}\.[0-9]{1,2}\.[0-9]{1,2}$")


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        pass


def write_image(path: Path, *strings: str) -> None:
    payload = bytearray(b"\x00header\x00")
    for item in strings:
        payload.extend(item.encode("ascii"))
        payload.append(0)
    path.write_bytes(bytes(payload))


def project_version_string(version: str) -> str:
    return f"Project {PROJECT_NAME} version {version}"


def release_image_strings(version: str, *extra: str) -> tuple[str, ...]:
    return (
        version,
        project_version_string(version),
        "package_import_url",
        version,
        PROJECT_NAME,
        "project_version",
        *extra,
    )


def write_release_image(path: Path, version: str, *extra: str) -> None:
    write_image(path, *release_image_strings(version, *extra))


def run_ok(args: list[str]) -> None:
    code = firmware_release.main(args)
    assert code == 0, f"{args} exited {code}"


def run_fails(args: list[str]) -> None:
    with redirect_stderr(io.StringIO()):
        code = firmware_release.main(args)
    assert code != 0, f"{args} unexpectedly passed"


class FakeGitHub:
    def __init__(self, tag: str, draft: bool = True, wrong_size: bool = False) -> None:
        self.tag = tag
        self.draft = draft
        self.wrong_size = wrong_size
        self.assets: list[dict[str, object]] = []
        self.calls: list[list[str]] = []

    def __call__(self, arguments: list[str]) -> str:
        self.calls.append(arguments)
        if arguments[:2] == ["release", "view"]:
            return json.dumps({"tagName": self.tag, "isDraft": self.draft, "assets": self.assets})
        if arguments[:2] == ["release", "upload"]:
            end = arguments.index("--clobber")
            files = [Path(value) for value in arguments[3:end]]
            self.assets = [
                {
                    "name": path.name,
                    "size": path.stat().st_size + (1 if self.wrong_size else 0),
                }
                for path in files
            ]
            return ""
        if arguments[:2] == ["release", "edit"]:
            self.draft = False
            return ""
        raise AssertionError(f"unexpected fake gh command: {arguments}")


def validate_esphome_env(path: Path) -> None:
    lines = path.read_text(encoding="utf-8").splitlines()
    assert len(lines) == 1, f"{path}: expected exactly one ESPHOME_VERSION line"
    assert ESPHOME_ENV_RE.fullmatch(lines[0]), (
        f"{path}: expected ESPHOME_VERSION to be a stable numeric ESPHome release, "
        "for example ESPHOME_VERSION=2026.6.5"
    )


def test_esphome_env_format() -> None:
    validate_esphome_env(ESPHOME_ENV)
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        valid = base / "valid.env"
        valid.write_text("ESPHOME_VERSION=2026.6.5\n", encoding="utf-8")
        validate_esphome_env(valid)

        for value in (
            "",
            "export ESPHOME_VERSION=2026.6.5\n",
            "ESPHOME_VERSION=\"2026.6.5\"\n",
            "ESPHOME_VERSION=2026.6.5-beta.1\n",
            "OTHER_VERSION=2026.6.5\n",
            "ESPHOME_VERSION=2026.6.5\nEXTRA=value\n",
        ):
            invalid = base / "invalid.env"
            invalid.write_text(value, encoding="utf-8")
            try:
                validate_esphome_env(invalid)
            except AssertionError:
                pass
            else:
                raise AssertionError(f"invalid ESPHome env value passed validation: {value!r}")


def test_release_workflow_uses_current_ota_output() -> None:
    workflow = RELEASE_WORKFLOW.read_text(encoding="utf-8")
    current_copy = 'cp "${BUILD_DIR}/firmware.ota.bin" dist/firmware/${{ matrix.slug }}.ota.bin'
    legacy_copy = 'cp "${BUILD_DIR}/firmware.bin" dist/firmware/${{ matrix.slug }}.ota.bin'
    assert current_copy in workflow, "release workflow must package ESPHome's firmware.ota.bin output"
    assert legacy_copy not in workflow, "release workflow still packages the obsolete firmware.bin output"
    assert "types: [published]" not in workflow, "public releases must not start an incomplete firmware build"
    assert "required: true" in workflow, "release workflow must require an explicit draft tag"
    assert "scripts/firmware_release.py verify-draft" in workflow
    assert "scripts/firmware_release.py verify-bundle" in workflow
    assert "scripts/firmware_release.py publish-draft" in workflow
    assert "scripts/firmware_release.py verify-recovery" in workflow
    assert str(prepare_c6_firmware.C6_RELATIVE_PATH) in workflow
    assert "path: dist/firmware/" in workflow, "publishable firmware must use the dist boundary"


def test_pages_excludes_draft_prereleases() -> None:
    workflow = PAGES_WORKFLOW.read_text(encoding="utf-8")
    assert "select((.draft | not) and .prerelease)" in workflow
    assert "select(.prerelease)" not in workflow


def test_release_skill_creates_selected_tag_before_draft() -> None:
    skill = RELEASE_SKILL.read_text(encoding="utf-8")
    tag_creation = skill.index('git tag -a "$TAG"')
    assert skill.index('TAG="vX.Y.Z"') < tag_creation
    assert skill.index('TAG="vX.Y.Z-beta.N"') < tag_creation
    assert skill.index('gh release create "$TAG"', tag_creation) > tag_creation


def make_release_files(base: Path, slug: str = SLUG, version: str = VERSION) -> tuple[Path, Path, Path]:
    factory = base / f"{slug}.factory.bin"
    ota = base / f"{slug}.ota.bin"
    manifest = base / f"{slug}.manifest.json"
    write_release_image(factory, version)
    write_release_image(ota, version)
    run_ok([
        "manifest",
        "--slug", slug,
        "--chip", "ESP32-P4" if slug == RECOVERY_SLUG else CHIP,
        "--version", version,
        "--factory", str(factory),
        "--ota", str(ota),
        "--out", str(manifest),
    ])
    return manifest, factory, ota


def make_recovery_files(
    base: Path, slug: str = RECOVERY_SLUG, version: str = VERSION
) -> tuple[Path, Path, Path, Path, Path]:
    normal_manifest, normal_factory, normal_ota = make_release_files(base, slug, version)
    recovery = base / f"{slug}.recovery.bin"
    recovery_manifest = base / f"{slug}.recovery.manifest.json"
    c6_firmware = base / ".build-dependencies" / "network_adapter_esp32c6.bin"
    c6_firmware.parent.mkdir()
    c6_firmware.write_bytes(b"verified-c6-payload")
    write_release_image(recovery, version)
    with recovery.open("ab") as handle:
        handle.write(c6_firmware.read_bytes())
    run_ok([
        "recovery-manifest",
        "--slug", slug,
        "--version", version,
        "--recovery", str(recovery),
        "--out", str(recovery_manifest),
    ])
    return recovery_manifest, recovery, c6_firmware, normal_factory, normal_ota


def test_recovery_manifest_and_payload_verification() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        recovery_manifest, recovery, c6, normal_factory, normal_ota = make_recovery_files(base)
        run_ok([
            "verify-recovery",
            "--slug", RECOVERY_SLUG,
            "--version", VERSION,
            "--manifest", str(recovery_manifest),
            "--recovery", str(recovery),
            "--c6-firmware", str(c6),
            "--normal-factory", str(normal_factory),
            "--normal-ota", str(normal_ota),
        ])
        run_ok([
            "verify-bundle",
            "--version", VERSION,
            "--dir", str(base),
            "--slugs", RECOVERY_SLUG,
            "--recovery-slugs", RECOVERY_SLUG,
        ])

        with normal_factory.open("ab") as handle:
            handle.write(c6.read_bytes())
        run_fails([
            "verify-recovery",
            "--slug", RECOVERY_SLUG,
            "--version", VERSION,
            "--manifest", str(recovery_manifest),
            "--recovery", str(recovery),
            "--c6-firmware", str(c6),
            "--normal-factory", str(normal_factory),
            "--normal-ota", str(normal_ota),
        ])


def test_c6_dependency_preparation_is_verified_and_atomic() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        source = base / "source.bin"
        output = base / "cache" / "firmware.bin"
        payload = b"test-c6-firmware"
        source.write_bytes(payload)
        original_hash = prepare_c6_firmware.C6_SHA256
        prepare_c6_firmware.C6_SHA256 = hashlib.sha256(payload).hexdigest()
        try:
            prepare_c6_firmware.prepare(output, source.as_uri())
            assert output.read_bytes() == payload

            source.write_bytes(b"different-source")
            prepare_c6_firmware.prepare(output, source.as_uri())
            assert output.read_bytes() == payload, "verified cache should be reused"

            output.write_bytes(b"corrupt-cache")
            try:
                prepare_c6_firmware.prepare(output, source.as_uri())
            except prepare_c6_firmware.C6FirmwareError:
                pass
            else:
                raise AssertionError("checksum mismatch unexpectedly passed")
            assert not output.exists(), "invalid dependency should not remain cached"
            assert not list(output.parent.glob("*.tmp")), "temporary downloads were not cleaned"
        finally:
            prepare_c6_firmware.C6_SHA256 = original_hash


def test_recovery_sources_and_documentation_stay_complete() -> None:
    recovery_yaml = (ROOT / "common/device/esp32_c6_recovery.yaml").read_text(
        encoding="utf-8"
    )
    assert f'version: "{prepare_c6_firmware.C6_VERSION}"' in recovery_yaml
    assert f'sha256: "{prepare_c6_firmware.C6_SHA256}"' in recovery_yaml
    assert str(prepare_c6_firmware.C6_RELATIVE_PATH).replace(
        ".firmware-deps/", "../.firmware-deps/"
    ) in recovery_yaml
    recovery_source = (ROOT / "components/c6_recovery/c6_recovery.cpp").read_text(
        encoding="utf-8"
    )
    recovery_header = (ROOT / "components/c6_recovery/c6_recovery.h").read_text(
        encoding="utf-8"
    )
    assert "esp_hosted_connect_to_slave()" in recovery_source
    assert "setup_priority::WIFI + 1.0f" in recovery_header
    assert "current_version > target_version" in recovery_source
    assert "current_version < target_version" in recovery_source
    online_updater = (
        ROOT / "common/device/esp32_c6_firmware_update.yaml"
    ).read_text(encoding="utf-8")
    assert "platform: esp32_hosted" in online_updater
    assert "type: http" in online_updater
    selector = (
        ROOT / "docs/.vitepress/theme/components/C6RecoverySelector.vue"
    ).read_text(encoding="utf-8")
    assert "devices.find((device) => device.slug === requested)" in selector
    assert "/recovery/manifest.json" in selector
    assert "Chrome or Edge" in selector
    assert "exact panel model and revision" in selector
    assert "Repair C6 and reinstall EspControl" in selector

    p4_slugs = [
        entry["slug"]
        for entry in json.loads(
            subprocess.run(
                ["python3", "scripts/device_matrix.py", "release"],
                cwd=ROOT,
                check=True,
                capture_output=True,
                text=True,
            ).stdout
        )["include"]
        if entry["recovery"]
    ]
    for slug in p4_slugs:
        wrapper = ROOT / "builds" / f"{slug}.recovery.yaml"
        assert wrapper.is_file(), f"{slug}: recovery build wrapper is missing"
        assert "esp32_c6_recovery.yaml" in wrapper.read_text(encoding="utf-8")
        normal_factory = ROOT / "builds" / f"{slug}.factory.yaml"
        assert "esp32_c6_recovery" not in normal_factory.read_text(encoding="utf-8")

    install = (ROOT / "docs/getting-started/install.md").read_text(encoding="utf-8")
    assert "/getting-started/c6-recovery" in install
    screen_docs = {
        "guition-esp32-p4-jc1060p470": ROOT / "docs/screens/jc1060p470.md",
        "guition-esp32-p4-jc4880p443": ROOT / "docs/screens/jc4880p443.md",
        "guition-esp32-p4-jc8012p4a1": ROOT / "docs/screens/jc8012p4a1.md",
        "guition-esp32-p4-jc8012p4a1-v2": ROOT / "docs/screens/jc8012p4a1-v2.md",
        "esp32-p4-86": ROOT / "docs/screens/p4-86.md",
    }
    for slug, path in screen_docs.items():
        text = path.read_text(encoding="utf-8")
        assert "C6RecoveryCallout" in text and slug in text, (
            f"{path}: C6 recovery link is missing"
        )
        assert slug in selector, f"{slug}: recovery selector option is missing"
    s3_doc = (ROOT / "docs/screens/4848s040.md").read_text(encoding="utf-8")
    assert "C6RecoveryCallout" not in s3_doc
    assert "guition-esp32-s3-4848s040" not in selector


def test_valid_files_and_directory() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        run_ok([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])
        run_ok(["verify-directory", "--version", VERSION, "--dir", str(base), "--slugs", SLUG])


def test_placeholder_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        write_release_image(ota, VERSION, project_version_string("dev"))
        run_ok([
            "manifest",
            "--slug", SLUG,
            "--chip", CHIP,
            "--version", VERSION,
            "--factory", str(factory),
            "--ota", str(ota),
            "--out", str(manifest),
        ])
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_unrelated_placeholder_strings_pass() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        write_release_image(ota, VERSION, "Version unknown", "Dev", "0.0.0", "Dev build", "dev")
        run_ok([
            "manifest",
            "--slug", SLUG,
            "--chip", CHIP,
            "--version", VERSION,
            "--factory", str(factory),
            "--ota", str(ota),
            "--out", str(manifest),
        ])
        run_ok([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_wrong_manifest_version_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        data = json.loads(manifest.read_text())
        data["version"] = "v0.0.1"
        manifest.write_text(json.dumps(data))
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_missing_chip_family_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        data = json.loads(manifest.read_text())
        data["builds"][0].pop("chipFamily")
        manifest.write_text(json.dumps(data))
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_wrong_chip_family_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        data = json.loads(manifest.read_text())
        data["builds"][0]["chipFamily"] = "ESP32-P4"
        manifest.write_text(json.dumps(data))
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_wrong_md5_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        write_image(ota, VERSION, "changed-after-manifest")
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_missing_asset_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        make_release_files(base)
        (base / f"{SLUG}.factory.bin").unlink()
        run_fails(["verify-directory", "--version", VERSION, "--dir", str(base), "--slugs", SLUG])


def test_release_inventory_rejects_extra_files() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        make_release_files(base)
        (base / "partial-build.log").write_text("not a release asset", encoding="utf-8")
        try:
            firmware_release.verify_release_inventory(base, [SLUG])
        except firmware_release.FirmwareReleaseError:
            pass
        else:
            raise AssertionError("release inventory accepted an unexpected build file")


def test_draft_release_publishes_only_after_remote_asset_verification() -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        base = root / "firmware"
        base.mkdir()
        make_release_files(base)
        notes = root / "release-notes.md"
        notes.write_text("Verified release notes\n", encoding="utf-8")
        github = FakeGitHub(VERSION)
        firmware_release.publish_draft_release(
            base, [SLUG], VERSION, "owner/repo", notes, gh_runner=github
        )
        assert github.draft is False
        assert [call[:2] for call in github.calls] == [
            ["release", "view"],
            ["release", "upload"],
            ["release", "view"],
            ["release", "edit"],
            ["release", "view"],
        ]


def test_published_or_mismatched_asset_release_stays_unpublished() -> None:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        base = root / "firmware"
        base.mkdir()
        make_release_files(base)
        notes = root / "release-notes.md"
        notes.write_text("Verified release notes\n", encoding="utf-8")

        already_published = FakeGitHub(VERSION, draft=False)
        try:
            firmware_release.publish_draft_release(
                base, [SLUG], VERSION, "owner/repo", notes, gh_runner=already_published
            )
        except firmware_release.FirmwareReleaseError:
            pass
        else:
            raise AssertionError("publisher accepted an already-public release")
        assert len(already_published.calls) == 1

        wrong_size = FakeGitHub(VERSION, wrong_size=True)
        try:
            firmware_release.publish_draft_release(
                base, [SLUG], VERSION, "owner/repo", notes, gh_runner=wrong_size
            )
        except firmware_release.FirmwareReleaseError:
            pass
        else:
            raise AssertionError("publisher accepted an asset with the wrong byte size")
        assert wrong_size.draft is True
        assert not any(call[:2] == ["release", "edit"] for call in wrong_size.calls)


def test_wrong_slug_path_fails() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        manifest, factory, ota = make_release_files(base)
        data = json.loads(manifest.read_text())
        data["builds"][0]["ota"]["path"] = "other-panel.ota.bin"
        manifest.write_text(json.dumps(data))
        run_fails([
            "verify-files",
            "--slug", SLUG,
            "--version", VERSION,
            "--manifest", str(manifest),
            "--factory", str(factory),
            "--ota", str(ota),
        ])


def test_public_pages_verification() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        stable_dir = base / "firmware" / SLUG
        stable_dir.mkdir(parents=True)
        stable_manifest, _, _ = make_release_files(stable_dir)
        stable_manifest.rename(stable_dir / "manifest.json")

        beta_dir = stable_dir / "beta"
        beta_dir.mkdir()
        beta_manifest, _, _ = make_release_files(beta_dir, version="v9.8.8-beta.1")
        beta_manifest.rename(beta_dir / "manifest.json")

        handler = partial(QuietHandler, directory=str(base))
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            run_ok([
                "verify-pages",
                "--version", VERSION,
                "--base-url", f"http://127.0.0.1:{server.server_port}",
                "--slugs", SLUG,
            ])
        finally:
            server.shutdown()
            thread.join(timeout=5)


def main() -> int:
    test_esphome_env_format()
    test_release_workflow_uses_current_ota_output()
    test_pages_excludes_draft_prereleases()
    test_release_skill_creates_selected_tag_before_draft()
    test_valid_files_and_directory()
    test_placeholder_fails()
    test_unrelated_placeholder_strings_pass()
    test_wrong_manifest_version_fails()
    test_missing_chip_family_fails()
    test_wrong_chip_family_fails()
    test_wrong_md5_fails()
    test_missing_asset_fails()
    test_release_inventory_rejects_extra_files()
    test_recovery_manifest_and_payload_verification()
    test_c6_dependency_preparation_is_verified_and_atomic()
    test_recovery_sources_and_documentation_stay_complete()
    test_draft_release_publishes_only_after_remote_asset_verification()
    test_published_or_mismatched_asset_release_stays_unpublished()
    test_wrong_slug_path_fails()
    test_public_pages_verification()
    print("Firmware release helper tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
