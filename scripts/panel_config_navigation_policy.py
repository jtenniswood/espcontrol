"""Navigation compatibility declarations for staged PanelConfig releases.

A format-two reader can support only flat pages. Nested writes therefore need
their own reader-floor proof, independent of the durable document version.
"""

from __future__ import annotations

from collections.abc import Mapping, Sequence
import re
from typing import Any


def navigation_depths(contract: Mapping[str, Any]) -> tuple[int, int]:
    read_depth = contract.get("panelConfigReadNavigationDepth")
    write_depth = contract.get("panelConfigWriteNavigationDepth")
    assert type(read_depth) is int and 1 <= read_depth <= 2, (
        "PanelConfig reader navigation depth must be one or two"
    )
    assert type(write_depth) is int and 1 <= write_depth <= read_depth, (
        "PanelConfig writer navigation depth must not exceed its reader depth"
    )
    if read_depth == 2:
        versions = contract.get("panelConfigReadVersions")
        assert isinstance(versions, list) and 2 in versions, (
            "depth-two page readers require the version-two document reader"
        )
    if write_depth == 2:
        assert contract.get("panelConfigDocumentVersion") == 2 and contract.get("panelConfigWriteVersion") == 2, (
            "nested page writes require durable document version two"
        )
        compatibility = contract.get("panelConfigCompatibility")
        assert isinstance(compatibility, Mapping) and compatibility.get("phase") == "read-import-only", (
            "nested page writes require retirement of the legacy write mirror"
        )
    return read_depth, write_depth


def reader_floor_releases(
    contract: Mapping[str, Any], releases: Sequence[Mapping[str, Any]],
) -> Sequence[Mapping[str, Any]]:
    """Return the supported downgrade range, supplied newest first by GitHub."""
    floor = contract.get("panelConfigMinimumReaderRelease")
    assert isinstance(floor, str) and re.fullmatch(r"v\d+\.\d+\.\d+", floor), (
        "durable v2 requires an explicit stable minimum reader release"
    )
    floor_index = next((i for i, release in enumerate(releases) if release.get("tagName") == floor), None)
    assert floor_index is not None, "minimum reader release is not published with an immutable manifest"
    return releases[:floor_index + 1]


def validate_navigation_reader_floor(
    contract: Mapping[str, Any], releases: Sequence[Mapping[str, Any]],
) -> None:
    _, write_depth = navigation_depths(contract)
    if write_depth < 2:
        return
    for release in reader_floor_releases(contract, releases):
        manifest = release.get("manifest")
        declared = manifest.get("releaseContract") if isinstance(manifest, Mapping) else None
        assert isinstance(declared, Mapping), (
            "supported downgrade range lacks published reader navigation-depth proof"
        )
        depth = declared.get("panelConfigReadNavigationDepth")
        versions = declared.get("panelConfigReadVersions")
        assert (type(depth) is int and depth >= write_depth and
                isinstance(versions, list) and 2 in versions), (
            "supported downgrade range includes a release without a depth-two page reader"
        )
