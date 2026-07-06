#!/usr/bin/env python3
"""Fail when local machine metadata files are present in the repo tree."""

from __future__ import annotations

from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SKIP_DIRS = {
    ".git",
    ".cache",
    ".esphome",
    ".pio-core",
    ".platformio",
    ".platformio-run",
    "node_modules",
    "__pycache__",
}
SKIP_PATHS = {
    Path("docs/.vitepress/cache"),
    Path("docs/.vitepress/dist"),
    Path("builds/.esphome"),
}
TRACKED_ARTIFACT_SUFFIXES = (".pyc", ".pyo")
TRACKED_ARTIFACT_NAMES = {".DS_Store"}
STALE_TRACKED_ARTIFACTS = {
    Path("common/config/strings.en.json"),
}
TEMP_PR_PREFIX = ".tmp-pr"


def should_skip_dir(path: Path) -> bool:
    rel = path.relative_to(ROOT)
    return path.name in SKIP_DIRS or rel in SKIP_PATHS


def find_tracked_local_artifacts() -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=ROOT,
        check=True,
        capture_output=True,
    )
    paths = []
    for raw_path in result.stdout.split(b"\0"):
        if not raw_path:
            continue
        path = Path(raw_path.decode())
        if (
            "__pycache__" in path.parts
            or path.name in TRACKED_ARTIFACT_NAMES
            or path.suffix in TRACKED_ARTIFACT_SUFFIXES
            or path in STALE_TRACKED_ARTIFACTS
        ):
            paths.append(path)
    return sorted(paths)


def main() -> int:
    ds_store_files: list[Path] = []
    temp_pr_dirs: list[Path] = []
    stack = [ROOT]
    while stack:
        current = stack.pop()
        for child in current.iterdir():
            if child.is_dir():
                if child.parent == ROOT and child.name.startswith(TEMP_PR_PREFIX):
                    temp_pr_dirs.append(child.relative_to(ROOT))
                    continue
                if not should_skip_dir(child):
                    stack.append(child)
                continue
            if child.name == ".DS_Store":
                ds_store_files.append(child.relative_to(ROOT))

    if temp_pr_dirs:
        print("Found temporary PR folders in the repository root:")
        for path in sorted(temp_pr_dirs):
            print(f"  {path}")
        print("Remove them before running local checks or committing.")
        return 1

    if ds_store_files:
        print("Found .DS_Store files that should not be kept in the repository:")
        for path in sorted(ds_store_files):
            print(f"  {path}")
        print("Remove them before committing.")
        return 1

    tracked_artifacts = find_tracked_local_artifacts()
    if tracked_artifacts:
        print("Found generated local files tracked by git:")
        for path in tracked_artifacts:
            print(f"  {path}")
        print("Remove them from version control before committing.")
        return 1

    print("Local artifact check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
