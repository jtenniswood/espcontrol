#!/usr/bin/env python3
"""Add a release tag to the generated web bundle compatibility source."""

from __future__ import annotations

import argparse
import json
import re
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_SCRIPT = ROOT / "scripts" / "build.py"
RELEASE_TAG_RE = re.compile(r"^v\d+\.\d+\.\d+(?:-[0-9A-Za-z][0-9A-Za-z.-]*)?$")
VERSION_LIST_RE = re.compile(
    r"(?ms)^(WEB_ASSET_SUPPORTED_FIRMWARE_VERSIONS = \(\n)(.*?)(^\))"
)
CURRENT_VERSION_RE = re.compile(
    r"(?m)^WEB_ASSET_CURRENT_FIRMWARE_VERSION = (.+)$"
)
MAX_STABLE_RELEASES = 5


class PrepareReleaseWebAssetsError(RuntimeError):
    pass


def is_prerelease(tag: str) -> bool:
    return "-" in tag


def release_catalog() -> list[dict]:
    try:
        result = subprocess.run(
            [
                "gh", "release", "list", "--limit", "30",
                "--json", "tagName,isDraft,isPrerelease,publishedAt",
            ],
            check=True,
            capture_output=True,
            text=True,
        )
        data = json.loads(result.stdout)
    except (OSError, subprocess.CalledProcessError, json.JSONDecodeError) as exc:
        raise PrepareReleaseWebAssetsError(
            "Could not read the published release catalogue with gh"
        ) from exc
    if not isinstance(data, list):
        raise PrepareReleaseWebAssetsError("GitHub release catalogue is not a list")
    return [release for release in data if isinstance(release, dict)]


def supported_versions(tag: str, releases: list[dict]) -> list[str]:
    published = [
        release
        for release in releases
        if release.get("isDraft") is False and isinstance(release.get("tagName"), str)
    ]
    stable = [
        release["tagName"]
        for release in published
        if release.get("isPrerelease") is False
    ]
    prereleases = [
        release["tagName"]
        for release in published
        if release.get("isPrerelease") is True
    ]
    if is_prerelease(tag):
        prereleases = [tag]
    else:
        stable = [tag, *stable]
    stable = list(dict.fromkeys(stable))[:MAX_STABLE_RELEASES]
    prereleases = list(dict.fromkeys(prereleases))[:1]
    return ["dev", *stable, *prereleases]


def prepare(path: Path, tag: str, releases: list[dict], set_current_version: bool = True) -> bool:
    if not RELEASE_TAG_RE.fullmatch(tag):
        raise PrepareReleaseWebAssetsError(
            f"{tag!r} is not a full release tag such as v1.2.3 or v1.2.3-beta.1"
        )
    source = path.read_text(encoding="utf-8")
    match = VERSION_LIST_RE.search(source)
    if not match:
        raise PrepareReleaseWebAssetsError(
            f"Could not find WEB_ASSET_SUPPORTED_FIRMWARE_VERSIONS in {path}"
        )
    current_match = CURRENT_VERSION_RE.search(source)
    if not current_match:
        raise PrepareReleaseWebAssetsError(
            f"Could not find WEB_ASSET_CURRENT_FIRMWARE_VERSION in {path}"
        )
    versions = re.findall(r'"([^"]+)"', match.group(2))
    if not versions or versions[0] != "dev":
        raise PrepareReleaseWebAssetsError("web asset compatibility list must start with dev")
    updated_versions = supported_versions(tag, releases)
    updated_current = (
        f'WEB_ASSET_CURRENT_FIRMWARE_VERSION = "{tag}"'
        if set_current_version
        else "WEB_ASSET_CURRENT_FIRMWARE_VERSION = None"
    )
    if versions == updated_versions and current_match.group(0) == updated_current:
        return False
    entries = "".join(f'    "{version}",\n' for version in updated_versions)
    updated = source[:match.start()] + match.group(1) + entries + match.group(3) + source[match.end():]
    current_match = CURRENT_VERSION_RE.search(updated)
    assert current_match is not None
    updated = updated[:current_match.start()] + updated_current + updated[current_match.end():]
    path.write_text(updated, encoding="utf-8")
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("tag", help="New release tag, for example v1.2.3")
    parser.add_argument("--build-script", type=Path, default=BUILD_SCRIPT)
    parser.add_argument(
        "--releases-json",
        type=Path,
        help="Saved GitHub release catalogue, for repeatable testing",
    )
    parser.add_argument(
        "--legacy-only",
        action="store_true",
        help="Update published compatibility versions without assigning the current source bundle to the tag",
    )
    args = parser.parse_args(argv)
    try:
        releases = (
            json.loads(args.releases_json.read_text(encoding="utf-8"))
            if args.releases_json
            else release_catalog()
        )
        if not isinstance(releases, list):
            raise PrepareReleaseWebAssetsError("release catalogue is not a list")
        changed = prepare(
            args.build_script,
            args.tag,
            releases,
            set_current_version=not args.legacy_only,
        )
    except PrepareReleaseWebAssetsError as exc:
        print(f"::error::{exc}")
        return 1
    if changed:
        print(f"Added {args.tag} to the web asset compatibility list. Run scripts/build.py next.")
    else:
        print(f"{args.tag} is already present in the web asset compatibility list.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
