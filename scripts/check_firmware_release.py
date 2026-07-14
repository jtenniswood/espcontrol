#!/usr/bin/env python3
"""Smoke tests for scripts/firmware_release.py."""

from __future__ import annotations

from contextlib import redirect_stderr
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import io
import json
from pathlib import Path
import re
from tempfile import TemporaryDirectory
from threading import Thread

import firmware_release


SLUG = "guition-esp32-s3-4848s040"
VERSION = "v9.8.7"
CHIP = "ESP32-S3"
PROJECT_NAME = "jtenniswood.espcontrol"
ESPHOME_ENV = Path(__file__).resolve().parents[1] / ".github" / "esphome.env"
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


def make_release_files(base: Path, slug: str = SLUG, version: str = VERSION) -> tuple[Path, Path, Path]:
    factory = base / f"{slug}.factory.bin"
    ota = base / f"{slug}.ota.bin"
    manifest = base / f"{slug}.manifest.json"
    write_release_image(factory, version)
    write_release_image(ota, version)
    run_ok([
        "manifest",
        "--slug", slug,
        "--chip", CHIP,
        "--version", version,
        "--factory", str(factory),
        "--ota", str(ota),
        "--out", str(manifest),
    ])
    return manifest, factory, ota


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
    test_valid_files_and_directory()
    test_placeholder_fails()
    test_unrelated_placeholder_strings_pass()
    test_wrong_manifest_version_fails()
    test_missing_chip_family_fails()
    test_wrong_chip_family_fails()
    test_wrong_md5_fails()
    test_missing_asset_fails()
    test_wrong_slug_path_fails()
    test_public_pages_verification()
    print("Firmware release helper tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
