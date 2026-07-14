#!/usr/bin/env python3
"""Generate plain-language pull request testing guidance from changed files."""

from __future__ import annotations

import argparse
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

from device_profiles import load_device_profiles


ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_WIDE_PREFIXES = (
    "common/",
    "components/",
    "docs/public/webserver/",
    "src/webserver/",
)
FIRMWARE_WIDE_FILES = {
    ".github/esphome.env",
    "devices/manifest.json",
}
FIRMWARE_WIDE_SCRIPTS = (
    "scripts/build.py",
    "scripts/device_matrix.py",
    "scripts/device_profiles.py",
    "scripts/generate_device_slots.py",
)
PROCESS_FILES = (
    "AGENTS.md",
    ".github/pull_request_template.md",
    "docs/reference/contributing.md",
    "package.json",
    "package-lock.json",
    "scripts/check_pr_process.py",
    "scripts/pr_testing_guidance.py",
)
TEST_ONLY_SCRIPT_PREFIXES = (
    "scripts/check_",
)
TEST_ONLY_FILES = {
    "scripts/pr_testing_guidance.py",
}


@dataclass(frozen=True)
class Guidance:
    devices: list[str]
    firmware_related: bool
    process_related: bool
    process_only: bool
    docs_only: bool
    test_only: bool
    changed_files: list[str]


def git_changed_files(base_ref: str) -> list[str]:
    paths: list[str] = []
    for command in (
        ["git", "diff", "--name-only", f"{base_ref}...HEAD"],
        ["git", "diff", "--name-only"],
        ["git", "diff", "--name-only", "--cached"],
        ["git", "ls-files", "--others", "--exclude-standard"],
    ):
        result = subprocess.run(command, cwd=ROOT, check=True, text=True, capture_output=True)
        paths.extend(result.stdout.splitlines())
    return clean_paths(paths)


def clean_paths(paths: list[str]) -> list[str]:
    return sorted({path.strip() for path in paths if path.strip()})


def paths_from_file(path: Path) -> list[str]:
    return clean_paths(path.read_text(encoding="utf-8").splitlines())


def test_only_path(path: str) -> bool:
    if path in TEST_ONLY_FILES:
        return True
    if path.startswith(TEST_ONLY_SCRIPT_PREFIXES):
        return True
    return (
        path.startswith("common/config/")
        and path.endswith("card_normalization_fixtures.json")
    )


def affected_devices(paths: list[str], slugs: set[str]) -> tuple[set[str], bool]:
    devices: set[str] = set()
    all_devices = False

    for path in paths:
        if test_only_path(path):
            continue
        parts = path.split("/")
        if len(parts) >= 2 and parts[0] == "devices" and parts[1] in slugs:
            devices.add(parts[1])
            continue
        if len(parts) >= 2 and parts[0] == "builds":
            build_slug = parts[1].replace(".factory.yaml", "").replace(".yaml", "")
            if build_slug in slugs:
                devices.add(build_slug)
                continue
        if (
            len(parts) >= 4
            and parts[0] == "docs"
            and parts[1] == "public"
            and parts[2] == "webserver"
            and parts[3] in slugs
        ):
            devices.add(parts[3])
            continue

        if path in FIRMWARE_WIDE_FILES or path in FIRMWARE_WIDE_SCRIPTS:
            all_devices = True
            continue
        if path.startswith(FIRMWARE_WIDE_PREFIXES):
            all_devices = True

    return devices, all_devices


def analyze(paths: list[str]) -> Guidance:
    profiles = load_device_profiles()
    slugs = set(profiles)
    devices, all_devices = affected_devices(paths, slugs)

    firmware_related = all_devices or bool(devices)
    if all_devices:
        devices = set(profiles)

    process_related = any(path in PROCESS_FILES or path.startswith(".github/workflows/") for path in paths)
    process_only = bool(paths) and all(
        path in PROCESS_FILES or path.startswith(".github/workflows/")
        for path in paths
    ) and not firmware_related
    docs_only = bool(paths) and all(
        path.startswith("docs/")
        or path in {".github/pull_request_template.md", "AGENTS.md"}
        or path.startswith(".github/")
        for path in paths
    ) and not firmware_related
    test_only = bool(paths) and all(test_only_path(path) for path in paths) and not firmware_related

    return Guidance(
        devices=sorted(devices),
        firmware_related=firmware_related,
        process_related=process_related,
        process_only=process_only,
        docs_only=docs_only,
        test_only=test_only,
        changed_files=paths,
    )


def public_device_name(slug: str) -> str:
    profile = load_device_profiles()[slug]
    public = profile["public"]
    return f"{public['name']} ({public['screenSize']}, {public['orientation']})"


def render_markdown(guidance: Guidance) -> str:
    lines = [
        "<!-- espcontrol-pr-testing-guidance -->",
        "## Automated testing guidance",
        "",
    ]

    if guidance.firmware_related:
        lines.extend(
            [
                "Device testing is expected before merge because this change may affect firmware or the on-device experience.",
                "",
                "Suggested devices to test:",
            ]
        )
        for slug in guidance.devices:
            lines.append(f"- {public_device_name(slug)}")
        lines.extend(
            [
                "",
                "Suggested checks:",
                "- Confirm the device boots normally and does not restart repeatedly.",
                "- Confirm the affected screen, card, or setting appears correctly.",
                "- Confirm touch/navigation still works where the changed area is interactive.",
                "- Confirm there is no obvious text overlap, clipping, blank screen, or missing icon.",
                "",
                "Suggested PR status: Ready for device test once automated checks pass.",
            ]
        )
    elif guidance.process_only:
        lines.extend(
            [
                "No physical device test is expected from the changed files.",
                "",
                "Suggested checks:",
                "- Confirm the PR template and automated guidance are clear.",
                "- Confirm automated checks pass.",
                "",
                "Suggested PR status: Ready to merge after review and user confirmation.",
            ]
        )
    elif guidance.docs_only:
        lines.extend(
            [
                "No physical device test is expected from the changed files.",
                "",
                "Suggested checks:",
                "- Confirm the documentation or workflow text says what a user should do.",
                "- Confirm automated checks pass.",
                "",
                "Suggested PR status: Ready to merge after review and user confirmation.",
            ]
        )
    elif guidance.test_only:
        lines.extend(
            [
                "No physical device test is expected from the changed files.",
                "",
                "Suggested checks:",
                "- Confirm the relevant automated checks pass.",
                "",
                "Suggested PR status: Ready to merge after review and user confirmation.",
            ]
        )
    else:
        lines.extend(
            [
                "No specific device could be inferred from the changed files.",
                "",
                "Suggested checks:",
                "- Confirm automated checks pass.",
                "- If the change affects what appears on a display, test the relevant device before merge.",
                "",
                "Suggested PR status: Needs human judgement on whether device testing is required.",
            ]
        )

    if guidance.process_related:
        lines.extend(
            [
                "",
                "Process note:",
                "- This PR changes workflow or review process files, so check that the PR template/check output remains clear.",
            ]
        )

    lines.extend(["", "Changed files considered:"])
    for path in guidance.changed_files[:20]:
        lines.append(f"- `{path}`")
    if len(guidance.changed_files) > 20:
        lines.append(f"- ...and {len(guidance.changed_files) - 20} more")

    return "\n".join(lines).rstrip() + "\n"


def run_self_test() -> None:
    specific = analyze(["devices/guition-esp32-p4-jc1060p470/device/lvgl.yaml"])
    assert specific.firmware_related
    assert specific.devices == ["guition-esp32-p4-jc1060p470"]
    assert "Guition JC1060P470" in render_markdown(specific)

    wide = analyze(["common/device/screen_clock.yaml"])
    assert wide.firmware_related
    assert len(wide.devices) >= 5

    generated_web = analyze(["docs/public/webserver/www.js"])
    assert generated_web.firmware_related
    assert not generated_web.docs_only
    assert len(generated_web.devices) >= 5

    esphome_version = analyze([".github/esphome.env"])
    assert esphome_version.firmware_related
    assert not esphome_version.docs_only
    assert len(esphome_version.devices) >= 5

    docs = analyze(["docs/reference/contributing.md"])
    assert docs.docs_only or docs.process_only
    assert not docs.firmware_related

    process = analyze([".github/workflows/pr-testing-guidance.yml", "scripts/pr_testing_guidance.py"])
    assert process.process_only
    assert "No physical device test is expected" in render_markdown(process)

    parser_fixtures = analyze([
        "common/config/parser_runtime_card_normalization_fixtures.json",
        "scripts/check_config_formats.js",
        "scripts/check_firmware_parser.py",
        "scripts/pr_testing_guidance.py",
    ])
    assert parser_fixtures.test_only
    assert parser_fixtures.process_related
    assert not parser_fixtures.firmware_related
    assert "No physical device test is expected" in render_markdown(parser_fixtures)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base-ref", default="origin/main", help="Base ref for local git diff mode.")
    parser.add_argument("--changed-files", type=Path, help="File containing one changed path per line.")
    parser.add_argument("--output", type=Path, help="Write markdown guidance to this path.")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()

    if args.self_test:
        run_self_test()
        print("PR testing guidance self-test passed.")
        return 0

    paths = paths_from_file(args.changed_files) if args.changed_files else git_changed_files(args.base_ref)
    markdown = render_markdown(analyze(paths))
    if args.output:
        args.output.write_text(markdown, encoding="utf-8")
    else:
        sys.stdout.write(markdown)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
