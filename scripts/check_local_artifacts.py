#!/usr/bin/env python3
"""Fail when local machine metadata files are present in the repo tree."""

from __future__ import annotations

import argparse
import fnmatch
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


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
TRACKED_LOCAL_ONLY_PATTERNS = (
    "secrets.yaml",
    "secrets.yml",
    "*.secrets.yaml",
    "*.secrets.yml",
    "wifi_secrets*.yaml",
    "wifi-secrets*.yaml",
    "devices/*/dev-pr*.yaml",
)
STALE_TRACKED_ARTIFACTS = {
    Path("common/config/strings.en.json"),
}
TEMP_PR_PREFIX = ".tmp-pr"


def should_skip_dir(path: Path, root: Path = ROOT) -> bool:
    rel = path.relative_to(root)
    return path.name in SKIP_DIRS or rel in SKIP_PATHS


def is_local_only_tracked_file(path: Path) -> bool:
    value = path.as_posix()
    return any(
        fnmatch.fnmatchcase(value, pattern)
        if "/" in pattern
        else fnmatch.fnmatchcase(path.name, pattern)
        for pattern in TRACKED_LOCAL_ONLY_PATTERNS
    )


def find_tracked_local_artifacts(root: Path = ROOT) -> list[Path]:
    result = subprocess.run(
        ["git", "ls-files", "-z"],
        cwd=root,
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
            or is_local_only_tracked_file(path)
            or path in STALE_TRACKED_ARTIFACTS
        ):
            paths.append(path)
    return sorted(paths)


def find_local_artifacts(root: Path = ROOT) -> tuple[list[Path], list[Path]]:
    ds_store_files: list[Path] = []
    temp_pr_dirs: list[Path] = []
    stack = [root]
    while stack:
        current = stack.pop()
        for child in current.iterdir():
            if child.is_dir():
                if child.parent == root and child.name.startswith(TEMP_PR_PREFIX):
                    temp_pr_dirs.append(child.relative_to(root))
                    continue
                if not should_skip_dir(child, root):
                    stack.append(child)
                continue
            if child.name == ".DS_Store":
                ds_store_files.append(child.relative_to(root))
    return sorted(ds_store_files), sorted(temp_pr_dirs)


def run_git(repo: Path, *args: str) -> None:
    subprocess.run(["git", *args], cwd=repo, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def write_file(root: Path, path: str, text: str = "") -> None:
    target = root / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text, encoding="utf-8")


def run_self_test() -> int:
    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        write_file(root, ".DS_Store")
        write_file(root, "node_modules/.DS_Store")
        (root / ".tmp-pr-demo").mkdir()
        ds_store_files, temp_pr_dirs = find_local_artifacts(root)
        assert ds_store_files == [Path(".DS_Store")]
        assert temp_pr_dirs == [Path(".tmp-pr-demo")]

    with TemporaryDirectory() as tmp:
        repo = Path(tmp)
        run_git(repo, "init")
        write_file(repo, ".DS_Store")
        write_file(repo, "cache.pyc")
        write_file(repo, "src/__pycache__/module.py")
        write_file(repo, "src/app.py")
        run_git(repo, "add", ".")
        assert find_tracked_local_artifacts(repo) == [
            Path(".DS_Store"),
            Path("cache.pyc"),
            Path("src/__pycache__/module.py"),
        ]

    print("Local artifact self-test passed.")
    return 0


def check_local_artifacts(root: Path = ROOT) -> int:
    ds_store_files, temp_pr_dirs = find_local_artifacts(root)

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

    tracked_artifacts = find_tracked_local_artifacts(root)
    if tracked_artifacts:
        print("Found generated local files tracked by git:")
        for path in tracked_artifacts:
            print(f"  {path}")
        print("Remove them from version control before committing.")
        return 1

    print("Local artifact check passed.")
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true", help="run checker self-tests")
    args = parser.parse_args(argv)
    if args.self_test:
        return run_self_test()
    return check_local_artifacts()


if __name__ == "__main__":
    raise SystemExit(main())
