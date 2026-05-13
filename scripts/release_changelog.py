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
        "Controls and setup page",
        paths=("src/webserver/", "docs/public/webserver/"),
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
        "Firmware and device behavior",
        paths=(
            "builds/",
            "common/addon/",
            "common/config/",
            "common/device/",
            "common/theme/",
            "components/",
            "devices/",
        ),
        keywords=("compile", "device", "firmware", "relay", "wifi", "ethernet", "esp32", "lvgl"),
    ),
    Category(
        "Firmware releases and updates",
        paths=(
            ".github/workflows/pages.yml",
            ".github/workflows/release.yml",
            "docs/features/firmware-updates.md",
            "scripts/firmware_release.py",
            "scripts/check_firmware_release.py",
        ),
        keywords=("asset", "ota", "release", "tag", "update", "version"),
    ),
    Category(
        "Supported screens and hardware notes",
        paths=("docs/screens/",),
        keywords=("buy link", "hardware", "mount", "panel", "price", "screen", "stand"),
    ),
    Category(
        "Documentation",
        paths=("docs/", "README.md"),
        keywords=("doc", "docs", "faq", "install", "readme", "troubleshooting"),
    ),
    Category(
        "Build, tests, and maintenance",
        paths=(".github/", ".agents/", "package.json", "package-lock.json", "renovate.json", "scripts/"),
        keywords=("ci", "check", "license", "maintenance", "refactor", "test"),
    ),
)

FALLBACK_CATEGORY = "Other changes"


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


def remote_url() -> str:
    configured = run_git(["config", "--get", "remote.origin.url"], check=False).strip()
    if not configured:
        return DEFAULT_REPO_URL
    if configured.startswith("git@github.com:"):
        return "https://github.com/" + configured.removeprefix("git@github.com:").removesuffix(".git")
    return configured.removesuffix(".git")


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
    best_title = FALLBACK_CATEGORY
    best_score = 0
    for category in CATEGORIES:
        score = score_category(commit, category)
        if score > best_score:
            best_title = category.title
            best_score = score
    return best_title


def grouped_commits(commits: list[Commit]) -> dict[str, list[Commit]]:
    groups: dict[str, list[Commit]] = {}
    for commit in commits:
        groups.setdefault(categorize(commit), []).append(commit)
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
            "No commits were found in this release range.",
            "",
        ])
        return "\n".join(lines)

    to_label = display_ref(to_ref)
    comparison = compare_url(from_ref, comparison_ref(to_ref), repo_url)
    if comparison:
        lines.extend([f"[Full comparison]({comparison})", ""])

    breaking = breaking_changes(commits)
    if breaking:
        lines.extend(["## Important upgrade notes", ""])
        for commit in breaking:
            lines.append(f"- {linked_subject(commit.subject, repo_url)} ({linked_commit(commit, repo_url)})")
        lines.append("")

    lines.extend(["## Summary", ""])
    lines.append(f"- {len(commits)} commits are included in this release.")
    lines.append(f"- Release range: `{from_ref or 'start'}` to `{to_label}`.")
    lines.append("")

    groups = grouped_commits(commits)
    lines.extend(["## Detailed changes", ""])
    for category in [category.title for category in CATEGORIES] + [FALLBACK_CATEGORY]:
        entries = groups.get(category)
        if not entries:
            continue
        lines.extend([f"### {category}", ""])
        for commit in entries:
            subject = linked_subject(commit.subject, repo_url)
            commit_link = linked_commit(commit, repo_url)
            lines.append(f"- {subject} ({commit.date}, {commit_link}, {human_file_count(commit)})")
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
