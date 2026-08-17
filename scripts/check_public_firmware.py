#!/usr/bin/env python3
"""Verify public firmware manifests and referenced binaries.

This checks the deployed GitHub Pages firmware tree without requiring the
release tag. Stable manifests are required except for explicitly optional
slugs; beta manifests are optional but must be internally valid when present.
"""

from __future__ import annotations

import argparse
from contextlib import redirect_stdout
from functools import partial
import hashlib
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import io
import json
from pathlib import Path
import re
import sys
import time
from tempfile import TemporaryDirectory
from threading import Thread
import urllib.error
import urllib.request
from urllib.parse import urljoin


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = ROOT / "devices" / "manifest.json"
STABLE_VERSION_RE = re.compile(r"^v[0-9]+(\.[0-9]+){2}$")
MD5_RE = re.compile(r"^[0-9a-f]{32}$", re.IGNORECASE)
RECOVERY_REQUIRED_FROM = (2, 6, 4)


class PublicFirmwareError(RuntimeError):
    pass


class QuietHandler(SimpleHTTPRequestHandler):
    def log_message(self, format: str, *args: object) -> None:
        pass


def fetch_json(url: str) -> dict:
    request = urllib.request.Request(url, headers={"User-Agent": "espcontrol-public-firmware-check"})
    with urllib.request.urlopen(request, timeout=30) as response:
        data = response.read()
    try:
        parsed = json.loads(data.decode("utf-8"))
    except json.JSONDecodeError as exc:
        raise PublicFirmwareError(f"{url} is not valid JSON: {exc}") from exc
    if not isinstance(parsed, dict):
        raise PublicFirmwareError(f"{url} must contain a JSON object")
    return parsed


def assert_url_non_empty(url: str) -> None:
    request = urllib.request.Request(
        url,
        method="HEAD",
        headers={"User-Agent": "espcontrol-public-firmware-check"},
    )
    try:
        with urllib.request.urlopen(request, timeout=30) as response:
            length = response.headers.get("Content-Length")
            if length is not None and int(length) <= 0:
                raise PublicFirmwareError(f"{url} is empty")
            if length is not None:
                return
    except urllib.error.HTTPError as exc:
        if exc.code not in (403, 405):
            raise

    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "espcontrol-public-firmware-check",
            "Range": "bytes=0-0",
        },
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        if not response.read(1):
            raise PublicFirmwareError(f"{url} is empty")


def manifest_url(base_url: str, slug: str, beta: bool = False) -> str:
    return base_url.rstrip("/") + f"/firmware/{slug}/{'beta/' if beta else ''}manifest.json"


def versions_url(base_url: str, slug: str) -> str:
    return base_url.rstrip("/") + f"/firmware/{slug}/versions.json"


def require_build(manifest: dict, url: str) -> dict:
    builds = manifest.get("builds")
    if not isinstance(builds, list) or not builds or not isinstance(builds[0], dict):
        raise PublicFirmwareError(f"{url} has no firmware build")
    return builds[0]


def verify_versions_index(base_url: str, slug: str, latest_version: str) -> None:
    url = versions_url(base_url, slug)
    data = fetch_json(url)
    device = str(data.get("device", "")).strip()
    if device != slug:
        raise PublicFirmwareError(f"{url} device {device!r} must match slug {slug!r}")
    entries = data.get("versions")
    if not isinstance(entries, list) or not entries:
        raise PublicFirmwareError(f"{url} must contain a non-empty versions list")
    if len(entries) > 5:
        raise PublicFirmwareError(f"{url} must contain at most latest plus four previous versions")

    seen: set[str] = set()
    for idx, entry in enumerate(entries):
        if not isinstance(entry, dict):
            raise PublicFirmwareError(f"{url} version entry {idx + 1} must be an object")
        version = str(entry.get("version", "")).strip()
        if not STABLE_VERSION_RE.fullmatch(version):
            raise PublicFirmwareError(f"{url} version entry {idx + 1} has invalid stable version {version!r}")
        key = version.lower()
        if key in seen:
            raise PublicFirmwareError(f"{url} lists {version} more than once")
        seen.add(key)
        if idx == 0 and version != latest_version:
            raise PublicFirmwareError(f"{url} first version {version!r} must match latest {latest_version!r}")
        release_url = str(entry.get("release_url", "")).strip()
        if not release_url:
            raise PublicFirmwareError(f"{url} version {version} is missing release_url")
        ota = entry.get("ota")
        if not isinstance(ota, dict):
            raise PublicFirmwareError(f"{url} version {version} is missing ota")
        ota_path = str(ota.get("path", "")).strip()
        if not ota_path or ota_path.rsplit("/", 1)[-1] != f"{slug}.ota.bin":
            raise PublicFirmwareError(f"{url} version {version} must reference {slug}.ota.bin")
        md5 = str(ota.get("md5", "")).strip()
        if not MD5_RE.fullmatch(md5):
            raise PublicFirmwareError(f"{url} version {version} has invalid ota.md5")
        assert_url_non_empty(urljoin(url, ota_path))


def verify_public_slug(base_url: str, slug: str, beta: bool = False) -> str:
    url = manifest_url(base_url, slug, beta=beta)
    manifest = fetch_json(url)
    version = str(manifest.get("version", "")).strip()
    if not version or version in {"dev", "0.0.0"}:
        raise PublicFirmwareError(f"{url} has invalid firmware version {version!r}")
    if manifest.get("home_assistant_domain") != "esphome":
        raise PublicFirmwareError(f"{url} home_assistant_domain must be esphome")

    build = require_build(manifest, url)
    ota = build.get("ota")
    if not isinstance(ota, dict) or ota.get("path") != f"{slug}.ota.bin":
        raise PublicFirmwareError(f"{url} must reference {slug}.ota.bin")
    assert_url_non_empty(urljoin(url, ota["path"]))

    parts = build.get("parts")
    if beta and (not parts):
        return version
    if not isinstance(parts, list) or not parts or not isinstance(parts[0], dict):
        raise PublicFirmwareError(f"{url} has no factory firmware part")
    if parts[0].get("path") != f"{slug}.factory.bin":
        raise PublicFirmwareError(f"{url} must reference {slug}.factory.bin")
    assert_url_non_empty(urljoin(url, parts[0]["path"]))

    if not beta:
        verify_versions_index(base_url, slug, version)
    return version


def stable_version_tuple(version: str) -> tuple[int, int, int]:
    if not STABLE_VERSION_RE.fullmatch(version):
        raise PublicFirmwareError(f"invalid stable firmware version {version!r}")
    major, minor, patch = version.removeprefix("v").split(".")
    return int(major), int(minor), int(patch)


def verify_recovery_slug(base_url: str, slug: str, expected_version: str) -> None:
    url = base_url.rstrip("/") + f"/firmware/{slug}/recovery/manifest.json"
    manifest = fetch_json(url)
    if manifest.get("name") != "Espcontrol":
        raise PublicFirmwareError(f"{url} must retain the normal firmware identity")
    if str(manifest.get("version", "")).strip() != expected_version:
        raise PublicFirmwareError(f"{url} must match stable version {expected_version}")
    if manifest.get("home_assistant_domain") != "esphome":
        raise PublicFirmwareError(f"{url} home_assistant_domain must be esphome")
    if manifest.get("new_install_prompt_erase") is not True:
        raise PublicFirmwareError(f"{url} must offer the erase choice")
    build = require_build(manifest, url)
    if build.get("chipFamily") != "ESP32-P4" or "ota" in build:
        raise PublicFirmwareError(f"{url} must contain a USB-only ESP32-P4 build")
    parts = build.get("parts")
    expected_path = f"{slug}.recovery.bin"
    if (
        not isinstance(parts, list)
        or len(parts) != 1
        or not isinstance(parts[0], dict)
        or parts[0].get("path") != expected_path
        or parts[0].get("offset") != 0
    ):
        raise PublicFirmwareError(f"{url} must reference {expected_path} at offset 0")
    assert_url_non_empty(urljoin(url, expected_path))


def load_slugs(path: Path) -> list[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    devices = data.get("devices")
    if not isinstance(devices, dict) or not devices:
        raise PublicFirmwareError(f"{path} must contain a non-empty devices object")
    return sorted(devices)


def load_recovery_slugs(path: Path) -> set[str]:
    data = json.loads(path.read_text(encoding="utf-8"))
    devices = data.get("devices", {})
    return {
        slug
        for slug, device in devices.items()
        if device.get("firmware", {}).get("build", {}).get("chip") == "ESP32-P4"
    }


def verify_public_firmware(
    base_url: str,
    slugs: list[str],
    recovery_slugs: set[str],
    optional_stable_slugs: set[str],
    retries: int,
    delay: float,
) -> None:
    last_error: Exception | None = None
    for attempt in range(1, retries + 1):
        try:
            for slug in slugs:
                try:
                    stable_version = verify_public_slug(base_url, slug, beta=False)
                except urllib.error.HTTPError as exc:
                    if exc.code == 404 and slug in optional_stable_slugs:
                        print(f"::warning::No stable firmware manifest found for optional slug {slug}")
                        continue
                    else:
                        raise
                if (
                    slug in recovery_slugs
                    and stable_version_tuple(stable_version) >= RECOVERY_REQUIRED_FROM
                ):
                    verify_recovery_slug(base_url, slug, stable_version)
                try:
                    verify_public_slug(base_url, slug, beta=True)
                except urllib.error.HTTPError as exc:
                    if exc.code != 404:
                        raise
            return
        except Exception as exc:  # noqa: BLE001 - converted to a CI-friendly error after retries
            last_error = exc
            if attempt >= retries:
                break
            print(f"Public firmware check attempt {attempt} failed: {exc}", file=sys.stderr)
            time.sleep(delay)
    raise PublicFirmwareError(f"Public firmware verification failed after {retries} attempts: {last_error}")


def write_manifest(directory: Path, slug: str, include_factory: bool = True) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    ota_bytes = b"ota"
    (directory / f"{slug}.ota.bin").write_bytes(ota_bytes)
    parts = []
    if include_factory:
        (directory / f"{slug}.factory.bin").write_bytes(b"factory")
        parts.append({"path": f"{slug}.factory.bin", "offset": 0})
    (directory / "manifest.json").write_text(json.dumps({
        "name": "Espcontrol",
        "version": "v1.2.3",
        "home_assistant_domain": "esphome",
        "builds": [{
            "chipFamily": "ESP32-S3",
            "parts": parts,
            "ota": {
                "path": f"{slug}.ota.bin",
                "md5": hashlib.md5(ota_bytes).hexdigest(),
                "release_url": "https://example.invalid/releases/v1.2.3",
            },
        }],
    }), encoding="utf-8")


def write_versions_index(directory: Path, slug: str) -> None:
    old_dir = directory / "versions" / "v1.2.2"
    old_dir.mkdir(parents=True, exist_ok=True)
    old_bytes = b"old-ota"
    (old_dir / f"{slug}.ota.bin").write_bytes(old_bytes)
    (directory / "versions.json").write_text(json.dumps({
        "device": slug,
        "versions": [
            {
                "version": "v1.2.3",
                "release_url": "https://example.invalid/releases/v1.2.3",
                "ota": {
                    "path": f"{slug}.ota.bin",
                    "md5": hashlib.md5(b"ota").hexdigest(),
                },
            },
            {
                "version": "v1.2.2",
                "release_url": "https://example.invalid/releases/v1.2.2",
                "ota": {
                    "path": f"versions/v1.2.2/{slug}.ota.bin",
                    "md5": hashlib.md5(old_bytes).hexdigest(),
                },
            },
        ],
    }), encoding="utf-8")


def write_recovery_manifest(directory: Path, slug: str) -> None:
    directory.mkdir(parents=True, exist_ok=True)
    (directory / f"{slug}.recovery.bin").write_bytes(b"recovery")
    (directory / "manifest.json").write_text(json.dumps({
        "name": "Espcontrol",
        "version": "v1.2.3",
        "home_assistant_domain": "esphome",
        "new_install_prompt_erase": True,
        "builds": [{
            "chipFamily": "ESP32-P4",
            "parts": [{"path": f"{slug}.recovery.bin", "offset": 0}],
        }],
    }), encoding="utf-8")


def self_test() -> None:
    with TemporaryDirectory() as tmp:
        base = Path(tmp)
        write_manifest(base / "firmware" / "required-panel", "required-panel")
        write_versions_index(base / "firmware" / "required-panel", "required-panel")
        write_manifest(base / "firmware" / "required-panel" / "beta", "required-panel", include_factory=False)
        write_recovery_manifest(
            base / "firmware" / "required-panel" / "recovery", "required-panel"
        )
        handler = partial(QuietHandler, directory=str(base))
        server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
        thread = Thread(target=server.serve_forever, daemon=True)
        thread.start()
        url = f"http://127.0.0.1:{server.server_port}"
        try:
            warning_output = io.StringIO()
            with redirect_stdout(warning_output):
                verify_public_firmware(
                    url,
                    ["required-panel", "optional-panel"],
                    set(),
                    {"optional-panel"},
                    1,
                    0,
                )
            if "optional slug optional-panel" not in warning_output.getvalue():
                raise PublicFirmwareError("self-test expected missing optional firmware to warn")
            verify_recovery_slug(url, "required-panel", "v1.2.3")
            try:
                verify_public_firmware(url, ["missing-panel"], set(), set(), 1, 0)
            except PublicFirmwareError:
                pass
            else:
                raise PublicFirmwareError("self-test expected missing required firmware to fail")
            versions_index = base / "firmware" / "required-panel" / "versions.json"
            versions_data = json.loads(versions_index.read_text(encoding="utf-8"))
            versions_data["device"] = "wrong-panel"
            versions_index.write_text(json.dumps(versions_data), encoding="utf-8")
            try:
                verify_public_firmware(url, ["required-panel"], set(), set(), 1, 0)
            except PublicFirmwareError:
                pass
            else:
                raise PublicFirmwareError("self-test expected mismatched versions device to fail")
        finally:
            server.shutdown()
            thread.join(timeout=5)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run local script smoke tests")
    parser.add_argument("--base-url", help="public site base URL")
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST), help="device manifest path")
    parser.add_argument("--slugs", nargs="*", help="specific slugs to verify")
    parser.add_argument("--optional-stable-slugs", nargs="*", default=[], help="stable slugs allowed to be missing")
    parser.add_argument("--retries", type=int, default=1)
    parser.add_argument("--delay", type=float, default=15)
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        if args.self_test:
            self_test()
        else:
            if not args.base_url:
                raise PublicFirmwareError("--base-url is required unless --self-test is used")
            slugs = args.slugs or load_slugs(Path(args.manifest))
            recovery_slugs = load_recovery_slugs(Path(args.manifest))
            verify_public_firmware(
                args.base_url,
                slugs,
                recovery_slugs,
                set(args.optional_stable_slugs),
                args.retries,
                args.delay,
            )
    except (OSError, urllib.error.URLError, PublicFirmwareError) as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    print("Public firmware checks passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
