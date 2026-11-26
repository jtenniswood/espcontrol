#!/usr/bin/env python3
"""Verify that every release-facing artifact declares one compatible contract."""

from __future__ import annotations

from itertools import dropwhile
import json
import os
import re
from urllib.request import Request, urlopen
from pathlib import Path
from firmware_release import current_web_bundle


ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / "product" / "release_contract.json"
PRODUCT_MODEL = ROOT / "product" / "model_v2.json"
DEVICE_MANIFEST = ROOT / "devices" / "manifest.json"
WEB_MANIFEST = ROOT / "docs" / "public" / "webserver" / "web-assets.json"
DOCUMENT_HEADER = ROOT / "components" / "espcontrol" / "panel_config_document.h"
CAPABILITIES_HEADER = ROOT / "components" / "espcontrol" / "panel_config_capabilities.h"
COMPATIBILITY_POLICY = ROOT / "components" / "espcontrol" / "configuration_release_policy.h"
FIRMWARE_MODES = {
    "dual-write": "DUAL_WRITE",
    "read-import-only": "READ_IMPORT_ONLY",
    "native-only": "NATIVE_ONLY",
}


def read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def constant(path: Path, name: str) -> int:
    match = re.search(rf"constexpr uint16_t {name} = (\d+);", path.read_text(encoding="utf-8"))
    if not match:
        raise AssertionError(f"{path.relative_to(ROOT)}: missing {name}")
    return int(match.group(1))


def compatibility_policy(contract: dict) -> dict:
    compatibility = contract.get("panelConfigCompatibility")
    assert isinstance(compatibility, dict), "release contract lacks PanelConfig compatibility policy"
    assert compatibility.get("phase") in FIRMWARE_MODES, (
        "PanelConfig compatibility phase is invalid"
    )
    assert compatibility.get("dualWriteStableReleases") == 2, (
        "PanelConfig compatibility policy must retain two dual-write stable releases"
    )
    assert compatibility.get("readImportOnlyStableReleases") == 1, (
        "PanelConfig compatibility policy must retain one read/import-only stable release"
    )
    if compatibility["phase"] == "native-only":
        gateway = compatibility.get("legacyGatewayRelease")
        assert isinstance(gateway, str) and re.fullmatch(r"v\d+\.\d+\.\d+", gateway), (
            "native-only requires a stable legacy gateway version"
        )
    return compatibility


def validate_release_history(contract: dict, releases: list[dict]) -> None:
    """Require published manifests for the preceding compatibility phase."""
    policy = compatibility_policy(contract)
    phase = policy["phase"]
    if phase == "dual-write":
        return

    matching = [
        release
        for release in releases
        if release["manifest"].get("releaseContract", {}).get("panelConfigDocumentVersion")
        == contract["panelConfigDocumentVersion"]
    ]
    assert matching, "no published release manifests match this PanelConfig document version"
    phases = [
        release["manifest"]["releaseContract"].get("panelConfigCompatibility", {}).get("phase")
        for release in matching
    ]
    preceding = list(dropwhile(lambda published_phase: published_phase == phase, phases))
    if phase == "native-only":
        previous_phase = "read-import-only"
        required = policy["readImportOnlyStableReleases"]
    else:
        previous_phase = "dual-write"
        required = policy["dualWriteStableReleases"]
    assert preceding[:required] == [previous_phase] * required, (
        f"{phase} requires {required} consecutive published {previous_phase} releases "
        "with the current PanelConfig document version"
    )
    if phase == "native-only":
        gateway = next(
            (release for release in matching if release.get("tagName") == policy["legacyGatewayRelease"]),
            None,
        )
        assert gateway is not None and (
            gateway["manifest"]["releaseContract"].get("panelConfigCompatibility", {}).get("phase")
            == "read-import-only"
        ), "legacy gateway must be a published read/import-only release"


def github_release_history(repository: str) -> list[dict]:
    """Read immutable release manifests, newest first, from GitHub releases."""
    assert re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repository), (
        "GitHub repository must be OWNER/NAME"
    )
    headers = {"Accept": "application/vnd.github+json"}
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"

    def fetch_json(url: str) -> object:
        request = Request(url, headers=headers)
        with urlopen(request, timeout=20) as response:  # nosec B310: URLs are checked below.
            return json.loads(response.read().decode("utf-8"))

    releases = fetch_json(f"https://api.github.com/repos/{repository}/releases?per_page=100")
    assert isinstance(releases, list), "GitHub releases response was not a list"
    history = []
    for release in releases:
        if release.get("draft") or release.get("prerelease"):
            continue
        asset = next((item for item in release.get("assets", []) if item.get("name") == "release-manifest.json"), None)
        if asset is None:
            continue
        url = asset.get("browser_download_url")
        assert isinstance(url, str) and url.startswith("https://"), "release manifest asset URL is invalid"
        manifest = fetch_json(url)
        assert isinstance(manifest, dict), "release manifest asset was not a JSON object"
        assert manifest.get("releaseVersion") == release.get("tag_name"), (
            f"published release manifest does not match tag {release.get('tag_name')}"
        )
        assert isinstance(manifest.get("releaseContract"), dict), (
            f"published release {release.get('tag_name')} lacks its release contract"
        )
        history.append({"tagName": release["tag_name"], "manifest": manifest})
    return history


def verify() -> dict:
    contract = read_json(CONTRACT)
    assert contract["schemaVersion"] == 1, "release contract schema version must be 1"
    product_model = read_json(PRODUCT_MODEL)
    assert product_model["modelVersion"] == contract["productModelVersion"], (
        "Product Model version disagrees with release contract"
    )
    document_version = constant(DOCUMENT_HEADER, "PANEL_CONFIG_DOCUMENT_VERSION")
    capabilities = CAPABILITIES_HEADER.read_text(encoding="utf-8")
    assert "PANEL_CONFIG_DOCUMENT_VERSION" in capabilities, (
        "firmware capabilities do not advertise the PanelConfig document version"
    )
    assert document_version == contract["panelConfigDocumentVersion"], (
        "PanelConfig document version disagrees with release contract"
    )
    capability_web_version = constant(CAPABILITIES_HEADER, "PANEL_CONFIG_WEB_ASSET_VERSION")
    assert capability_web_version == contract["webAssetVersion"], (
        "firmware web-asset version disagrees with release contract"
    )
    device_slugs = list(read_json(DEVICE_MANIFEST)["devices"])
    bundle = current_web_bundle(WEB_MANIFEST, WEB_MANIFEST.parent)
    assert bundle.get("webAssetVersion") == contract["webAssetVersion"], (
        "web-asset version disagrees with release contract"
    )
    assert bundle.get("deviceProfiles") == device_slugs, (
        "web-asset device profiles disagree with generated device manifest"
    )
    compatibility = compatibility_policy(contract)
    expected_mode = "LegacyConfigurationMode::" + FIRMWARE_MODES[compatibility["phase"]]
    assert expected_mode in COMPATIBILITY_POLICY.read_text(encoding="utf-8"), (
        "firmware PanelConfig compatibility policy disagrees with release contract"
    )
    return contract


def self_test() -> None:
    verify()
    contract = read_json(CONTRACT)
    read_import_contract = json.loads(json.dumps(contract))
    read_import_contract["panelConfigCompatibility"]["phase"] = "read-import-only"

    def published(phase: str) -> dict:
        manifest_contract = json.loads(json.dumps(contract))
        manifest_contract["panelConfigCompatibility"]["phase"] = phase
        return {"manifest": {"releaseContract": manifest_contract}}

    validate_release_history(read_import_contract, [published("dual-write"), published("dual-write")])
    try:
        validate_release_history(read_import_contract, [published("dual-write")])
    except AssertionError as error:
        assert "2 consecutive published dual-write" in str(error)
    else:
        raise AssertionError("read/import-only accepted a single published dual-write release")
    try:
        validate_release_history(
            read_import_contract,
            [published("dual-write"), published("read-import-only"), published("dual-write")],
        )
    except AssertionError as error:
        assert "2 consecutive published dual-write" in str(error)
    else:
        raise AssertionError("read/import-only accepted non-consecutive dual-write releases")
    native_only_contract = json.loads(json.dumps(contract))
    native_only_policy = native_only_contract["panelConfigCompatibility"]
    native_only_policy["phase"] = "native-only"
    native_only_policy["legacyGatewayRelease"] = "v2.15.0"
    gateway = {**published("read-import-only"), "tagName": "v2.15.0"}
    validate_release_history(native_only_contract, [gateway])
    validate_release_history(
        native_only_contract,
        [published("native-only"), published("native-only"), gateway],
    )
    try:
        validate_release_history(native_only_contract, [published("native-only"), published("dual-write")])
    except AssertionError as error:
        assert "1 consecutive published read-import-only" in str(error)
    else:
        raise AssertionError("native-only accepted release history without a read/import-only release")
    try:
        validate_release_history(native_only_contract, [published("read-import-only")])
    except AssertionError as error:
        assert "legacy gateway must be a published" in str(error)
    else:
        raise AssertionError("native-only accepted an unpublished gateway release")
    original = PRODUCT_MODEL.read_text(encoding="utf-8")
    original_policy = COMPATIBILITY_POLICY.read_text(encoding="utf-8")
    current_phase = contract["panelConfigCompatibility"]["phase"]
    current_mode = FIRMWARE_MODES[current_phase]
    other_mode = next(mode for phase, mode in FIRMWARE_MODES.items() if phase != current_phase)
    try:
        product_model = json.loads(original)
        product_model["modelVersion"] = int(product_model["modelVersion"]) + 1
        PRODUCT_MODEL.write_text(json.dumps(product_model, indent=2) + "\n", encoding="utf-8")
        try:
            verify()
        except AssertionError as error:
            assert "Product Model version" in str(error)
        else:
            raise AssertionError("mismatched Product Model version was accepted")
    finally:
        PRODUCT_MODEL.write_text(original, encoding="utf-8")
    try:
        COMPATIBILITY_POLICY.write_text(
            original_policy.replace(
                f"LegacyConfigurationMode::{current_mode}",
                f"LegacyConfigurationMode::{other_mode}",
            ),
            encoding="utf-8",
        )
        try:
            verify()
        except AssertionError as error:
            assert "firmware PanelConfig compatibility policy" in str(error)
        else:
            raise AssertionError("mismatched PanelConfig compatibility policy was accepted")
    finally:
        COMPATIBILITY_POLICY.write_text(original_policy, encoding="utf-8")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument(
        "--github-repository",
        metavar="OWNER/NAME",
        help="verify the published immutable release manifests before advancing compatibility phase",
    )
    args = parser.parse_args()
    if args.self_test:
        self_test()
        print("Release contract self-test passed.")
    else:
        contract = verify()
        if args.github_repository:
            validate_release_history(contract, github_release_history(args.github_repository))
        print("Release contract checks passed.")
