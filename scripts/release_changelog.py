#!/usr/bin/env python3
"""Build a detailed changelog for a GitHub release.

The output is intended for the body of a GitHub Release. It compares the
release target with the previous release tag, groups commits by practical area,
and keeps commit links so the detail is still traceable.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_REPO_URL = "https://github.com/jtenniswood/espcontrol"
STABLE_TAG_RE = re.compile(r"^v\d+\.\d+\.\d+$")
PR_RE = re.compile(r"\(#(?P<number>\d+)\)")


@dataclass(frozen=True)
class Category:
    title: str
    paths: tuple[str, ...] = ()
    keywords: tuple[str, ...] = ()


@dataclass
class Commit:
    full_hash: str
    short_hash: str
    date: str
    subject: str
    files: list[str] = field(default_factory=list)


CATEGORIES = (
    Category(
        "User-facing features and setup",
        paths=("src/webserver/", "docs/public/webserver/", "docs/public/device-profiles.json"),
        keywords=(
            "action",
            "backlight",
            "button",
            "card",
            "climate",
            "clock",
            "color",
            "confirmation",
            "garage",
            "gate",
            "grid",
            "light",
            "lock",
            "modal",
            "rotation",
            "screen",
            "sensor",
            "setup",
            "slider",
            "subpage",
            "switch",
            "timezone",
            "weather",
        ),
    ),
    Category(
        "Firmware and device fixes",
        paths=(
            "builds/",
            "common/addon/",
            "common/assets/",
            "common/config/",
            "common/device/",
            "common/theme/",
            "components/",
            "devices/",
        ),
        keywords=(
            "asset",
            "device",
            "firmware",
            "fix",
            "fixed",
            "glyph",
            "icon",
            "relay",
            "wifi",
            "ethernet",
            "esp32",
            "lvgl",
            "stabilize",
        ),
    ),
)

INTERNAL_PATHS = (
    ".agents/",
    ".github/",
    "dev-docs/",
    "docs/",
    "package.json",
    "package-lock.json",
    "README.md",
    "renovate.json",
    "scripts/",
)
PUBLIC_SETUP_PATHS = ("docs/public/webserver/", "docs/public/device-profiles.json")
PUBLIC_RELEASE_SCRIPT_PATHS = (
    "scripts/firmware_release.py",
    "scripts/check_firmware_release.py",
    "scripts/check_public_firmware.py",
)
INTERNAL_SUBJECT_PREFIXES = (
    "add review",
    "ci:",
    "docs:",
    "merge ",
    "page review",
    "refactor",
    "resolve pr",
    "test:",
)
INTERNAL_SUBJECT_KEYWORDS = (
    "artifact check",
    "changelog",
    "check:",
    "compile workflow",
    "developer doc",
    "documentation",
    "release notes",
    "smoke expectation",
    "test hook",
)
BUG_FIX_SUBJECT_PREFIXES = (
    "clear ",
    "fix",
    "fixed ",
    "keep ",
    "preserve ",
    "reduce ",
    "reset ",
    "stabilize ",
)
DEVICE_SPECIFIC_PATH_PATTERNS = (
    re.compile(r"^devices/([^/]+)/"),
    re.compile(r"^docs/public/webserver/([^/]+)/"),
    re.compile(r"^builds/([^/.]+)(?:\.factory)?\.yaml$"),
)
FEATURE_SECTION = "User-facing features"
FIX_SECTION = "User-facing bug fixes"


class ChangelogError(RuntimeError):
    pass


def run_git(args: list[str], cwd: Path | None = None, check: bool = True) -> str:
    result = subprocess.run(
        ["git", *args],
        cwd=cwd or ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if check and result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise ChangelogError(f"git {' '.join(args)} failed: {detail}")
    return result.stdout


def ref_exists(ref: str) -> bool:
    result = subprocess.run(
        ["git", "rev-parse", "--verify", "--quiet", ref],
        cwd=ROOT,
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return result.returncode == 0


def tag_exists(tag: str) -> bool:
    return ref_exists(f"refs/tags/{tag}")


def resolve_commit(ref: str) -> str:
    return run_git(["rev-parse", "--verify", f"{ref}^{{commit}}"]).strip()


def short_commit(ref: str) -> str:
    return run_git(["rev-parse", "--short", f"{ref}^{{commit}}"]).strip()


def display_ref(ref: str) -> str:
    if tag_exists(ref):
        return ref
    short = short_commit(ref)
    return short if ref == short else f"{short} ({ref})"


def comparison_ref(ref: str) -> str:
    return ref if tag_exists(ref) else resolve_commit(ref)


def remove_prefix(value: str, prefix: str) -> str:
    return value[len(prefix) :] if value.startswith(prefix) else value


def remove_suffix(value: str, suffix: str) -> str:
    return value[: -len(suffix)] if suffix and value.endswith(suffix) else value


def remote_url() -> str:
    configured = run_git(["config", "--get", "remote.origin.url"], check=False).strip()
    if not configured:
        return DEFAULT_REPO_URL
    if configured.startswith("git@github.com:"):
        return "https://github.com/" + remove_suffix(remove_prefix(configured, "git@github.com:"), ".git")
    return remove_suffix(configured, ".git")


def stable_tags() -> list[str]:
    tags = run_git(["tag", "--list", "v*", "--sort=version:refname"]).splitlines()
    return [tag for tag in tags if STABLE_TAG_RE.match(tag)]


def is_ancestor(ancestor: str, ref: str) -> bool:
    result = subprocess.run(
        ["git", "merge-base", "--is-ancestor", ancestor, ref],
        cwd=ROOT,
        check=False,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    return result.returncode == 0


def latest_reachable_tag(ref: str) -> str | None:
    for tag in reversed(stable_tags()):
        if is_ancestor(tag, ref):
            return tag
    return None


def previous_tag_for_existing_tag(tag: str) -> str | None:
    tags = stable_tags()
    if tag in tags:
        index = tags.index(tag)
        if index > 0:
            return tags[index - 1]
    return latest_reachable_tag(f"{tag}^")


def default_to_ref(version: str) -> str:
    return version if tag_exists(version) else "HEAD"


def default_from_ref(version: str, to_ref: str) -> str | None:
    if tag_exists(version) and to_ref == version:
        return previous_tag_for_existing_tag(version)
    return latest_reachable_tag(to_ref)


def git_log_range(from_ref: str | None, to_ref: str) -> list[str]:
    return [f"{from_ref}..{to_ref}"] if from_ref else [to_ref]


def load_commits(from_ref: str | None, to_ref: str) -> list[Commit]:
    output = run_git([
        "log",
        "--reverse",
        "--date=short",
        "--format=commit%x09%H%x09%h%x09%ad%x09%s",
        "--name-only",
        *git_log_range(from_ref, to_ref),
    ])
    commits: list[Commit] = []
    current: Commit | None = None
    for line in output.splitlines():
        if line.startswith("commit\t"):
            parts = line.split("\t", 4)
            if len(parts) != 5:
                raise ChangelogError(f"Unexpected git log line: {line}")
            current = Commit(
                full_hash=parts[1],
                short_hash=parts[2],
                date=parts[3],
                subject=parts[4],
            )
            commits.append(current)
        elif line.strip() and current is not None:
            current.files.append(line.strip())
    return commits


def score_category(commit: Commit, category: Category) -> int:
    score = 0
    subject = commit.subject.lower()
    for path in commit.files:
        for prefix in category.paths:
            if path == prefix.rstrip("/") or path.startswith(prefix):
                score += 3
    for keyword in category.keywords:
        if keyword in subject:
            score += 2
    return score


def categorize(commit: Commit) -> str:
    best_title = ""
    best_score = 0
    for category in CATEGORIES:
        score = score_category(commit, category)
        if score > best_score:
            best_title = category.title
            best_score = score
    return best_title


def touches_any_path(path: str, prefixes: tuple[str, ...]) -> bool:
    return any(path == prefix.rstrip("/") or path.startswith(prefix) for prefix in prefixes)


def is_internal_change(commit: Commit) -> bool:
    subject = commit.subject.lower()
    if any(touches_any_path(path, PUBLIC_SETUP_PATHS) for path in commit.files):
        return False
    if touches_any(commit, PUBLIC_RELEASE_SCRIPT_PATHS) and any(
        keyword in subject
        for keyword in (
            "asset",
            "firmware",
            "manifest",
            "metadata",
            "ota",
            "release",
            "update",
            "url",
        )
    ):
        return False
    if any(subject.startswith(prefix) for prefix in INTERNAL_SUBJECT_PREFIXES):
        return True
    if any(keyword in subject for keyword in INTERNAL_SUBJECT_KEYWORDS):
        return True
    return bool(commit.files) and all(
        touches_any_path(path, INTERNAL_PATHS) and not touches_any_path(path, PUBLIC_SETUP_PATHS)
        for path in commit.files
    )


def user_facing_commits(commits: list[Commit]) -> list[Commit]:
    return [commit for commit in commits if categorize(commit) and not is_internal_change(commit)]


def public_change_section(commit: Commit) -> str:
    subject = commit.subject.lower()
    if any(subject.startswith(prefix) for prefix in BUG_FIX_SUBJECT_PREFIXES):
        return FIX_SECTION
    return FEATURE_SECTION


def grouped_public_changes(commits: list[Commit]) -> dict[str, list[Commit]]:
    groups: dict[str, list[Commit]] = {}
    for commit in commits:
        groups.setdefault(public_change_section(commit), []).append(commit)
    return groups


def grouped_commits(commits: list[Commit]) -> dict[str, list[Commit]]:
    groups: dict[str, list[Commit]] = {}
    for commit in commits:
        category = categorize(commit)
        if category:
            groups.setdefault(category, []).append(commit)
    return groups


def human_file_count(commit: Commit) -> str:
    count = len(set(commit.files))
    if count == 0:
        return "no file list"
    if count == 1:
        return "1 file"
    return f"{count} files"


def linked_commit(commit: Commit, repo_url: str | None) -> str:
    if not repo_url:
        return commit.short_hash
    return f"[{commit.short_hash}]({repo_url}/commit/{commit.full_hash})"


def linked_subject(subject: str, repo_url: str | None) -> str:
    if not repo_url:
        return subject

    def replace(match: re.Match[str]) -> str:
        number = match.group("number")
        return f"([#{number}]({repo_url}/pull/{number}))"

    return PR_RE.sub(replace, subject)


def clean_release_subject(subject: str, keep_pr: bool = True) -> str:
    text = subject.strip()
    text = re.sub(r"^(fix|feat):\s*", "", text, flags=re.IGNORECASE)
    if not keep_pr:
        text = re.sub(r"\s+\(#\d+\)$", "", text).strip()
    if text:
        text = text[0].upper() + text[1:]
    return text


def compare_url(from_ref: str | None, to_ref: str, repo_url: str | None) -> str | None:
    if not repo_url or not from_ref:
        return None
    return f"{repo_url}/compare/{from_ref}...{to_ref}"


def breaking_changes(commits: list[Commit]) -> list[Commit]:
    return [
        commit
        for commit in commits
        if "breaking change" in commit.subject.lower() or commit.subject.lower().startswith("breaking:")
    ]


def touches_any(commit: Commit, prefixes: tuple[str, ...]) -> bool:
    for path in commit.files:
        if touches_any_path(path, prefixes):
            return True
    return False


def device_slug_from_path(path: str) -> str | None:
    for pattern in DEVICE_SPECIFIC_PATH_PATTERNS:
        match = pattern.match(path)
        if match:
            return match.group(1)
    return None


def release_affects_all_devices(commits: list[Commit]) -> bool:
    all_device_paths = (
        "common/",
        "components/",
        "devices/manifest.json",
        "docs/public/device-profiles.json",
        *PUBLIC_RELEASE_SCRIPT_PATHS,
        "src/webserver/",
    )
    generated_device_paths = ("builds/", "docs/public/webserver/")
    for commit in commits:
        if touches_any(commit, all_device_paths):
            return True
        if any(
            touches_any_path(path, generated_device_paths) and not device_slug_from_path(path)
            for path in commit.files
        ):
            return True
    return False


def affected_device_slugs(commits: list[Commit]) -> list[str]:
    slugs: set[str] = set()
    for commit in commits:
        for path in commit.files:
            slug = device_slug_from_path(path)
            if slug:
                slugs.add(slug)
    return sorted(slugs)


def notable_subjects(commits: list[Commit], limit: int = 2) -> list[str]:
    subjects: list[str] = []
    for commit in commits:
        if commit.subject.startswith("Merge "):
            continue
        subject = clean_release_subject(commit.subject, keep_pr=False)
        if subject not in subjects:
            subjects.append(subject)
        if len(subjects) == limit:
            break
    return subjects


def release_focus_lines(groups: dict[str, list[Commit]]) -> list[str]:
    lines: list[str] = []
    for section in (FEATURE_SECTION, FIX_SECTION):
        entries = groups.get(section)
        if not entries:
            continue
        notable = notable_subjects(entries)
        suffix = f" Notable: {'; '.join(notable)}." if notable else ""
        lines.append(f"- {section}: {len(entries)} change{'s' if len(entries) != 1 else ''}.{suffix}")
    return lines


def update_guidance(commits: list[Commit], breaking: list[Commit]) -> str:
    if breaking:
        return "Review the important upgrade notes before updating production displays."
    if release_affects_all_devices(commits) or affected_device_slugs(commits):
        return "Recommended for users who want the latest firmware, setup page, device fixes, or documented behavior."
    return "Optional update. This release does not appear to change device firmware or setup behavior."


def affected_devices_text(commits: list[Commit]) -> str:
    if release_affects_all_devices(commits):
        return "All supported displays may be affected because shared firmware, setup, or build files changed."

    slugs = affected_device_slugs(commits)
    if slugs:
        return ", ".join(f"`{slug}`" for slug in slugs)

    return "No device-specific firmware or setup-page changes were detected."


def build_changelog(version: str, from_ref: str | None, to_ref: str, repo_url: str | None) -> str:
    commits = load_commits(from_ref, to_ref)
    title = f"# EspControl {version}"
    lines = [title, ""]

    if from_ref:
        lines.append(f"Changes since `{from_ref}`.")
    else:
        lines.append("Initial release changelog.")
    lines.append("")

    if not commits:
        lines.extend([
            "No user-facing features or bug fixes were found in this release range.",
            "",
        ])
        return "\n".join(lines)

    public_commits = user_facing_commits(commits)
    to_label = display_ref(to_ref)
    comparison = compare_url(from_ref, comparison_ref(to_ref), repo_url)
    if comparison:
        lines.extend([f"[Full comparison]({comparison})", ""])

    breaking = breaking_changes(public_commits)
    groups = grouped_public_changes(public_commits)
    lines.extend(["## What changed?", ""])
    focus_lines = release_focus_lines(groups)
    if focus_lines:
        lines.extend(focus_lines)
    else:
        lines.append("- No user-facing features or bug fixes were detected from the commits in this release.")
    lines.append("")

    lines.extend(["## Update guidance", ""])
    lines.append(f"- {update_guidance(public_commits, breaking)}")
    lines.append(f"- Affected devices: {affected_devices_text(public_commits)}")
    lines.append("")

    lines.extend(["## Known issues", ""])
    lines.append("- No release-specific known issues are listed automatically for this release.")
    lines.append("- Check open GitHub issues before updating devices you rely on every day.")
    lines.append("")

    if breaking:
        lines.extend(["## Important upgrade notes", ""])
        for commit in breaking:
            lines.append(f"- {linked_subject(commit.subject, repo_url)} ({linked_commit(commit, repo_url)})")
        lines.append("")

    lines.extend(["## Summary", ""])
    change_word = "change" if len(public_commits) == 1 else "changes"
    verb = "is" if len(public_commits) == 1 else "are"
    lines.append(f"- {len(public_commits)} user-facing {change_word} {verb} included in this release.")
    lines.append(f"- Release range: `{from_ref or 'start'}` to `{to_label}`.")
    lines.append("")

    lines.extend(["## User-facing changes", ""])
    for section in (FEATURE_SECTION, FIX_SECTION):
        entries = groups.get(section)
        if not entries:
            continue
        lines.extend([f"### {section}", ""])
        for commit in entries:
            subject = linked_subject(clean_release_subject(commit.subject), repo_url)
            lines.append(f"- {subject}")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version", help="Release version to show in the changelog, for example v1.12.0")
    parser.add_argument("--from", dest="from_ref", help="Previous release tag or commit. Defaults to the previous stable tag.")
    parser.add_argument("--to", dest="to_ref", help="Release target ref. Defaults to the tag if it exists, otherwise HEAD.")
    parser.add_argument("--repo-url", default=None, help="Repository URL for commit and comparison links.")
    parser.add_argument("--no-links", action="store_true", help="Do not add GitHub links to commits, PRs, or comparisons.")
    parser.add_argument("--output", help="Write the changelog to a file instead of stdout.")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    try:
        to_ref = args.to_ref or default_to_ref(args.version)
        from_ref = args.from_ref if args.from_ref is not None else default_from_ref(args.version, to_ref)
        repo_url = None if args.no_links else args.repo_url or remote_url()
        changelog = build_changelog(args.version, from_ref, to_ref, repo_url)
        if args.output:
            output = Path(args.output)
            output.parent.mkdir(parents=True, exist_ok=True)
            output.write_text(changelog)
        else:
            print(changelog, end="")
    except ChangelogError as exc:
        print(f"::error::{exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
