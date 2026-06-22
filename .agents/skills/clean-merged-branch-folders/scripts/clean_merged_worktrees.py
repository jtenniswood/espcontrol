#!/usr/bin/env python3
"""Find or remove local Git worktrees whose branches are merged to main."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


PROTECTED_BRANCHES = {"main", "master", "develop", "dev", "release"}


@dataclass
class Worktree:
    path: Path
    branch_ref: str | None
    head: str | None
    detached: bool = False
    bare: bool = False

    @property
    def branch(self) -> str | None:
        if not self.branch_ref or not self.branch_ref.startswith("refs/heads/"):
            return None
        return self.branch_ref.removeprefix("refs/heads/")


@dataclass
class Decision:
    worktree: Worktree
    removable: bool
    reason: str
    pr_url: str | None = None


def run(
    args: list[str],
    *,
    cwd: Path | None = None,
    check: bool = False,
    text: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=cwd, check=check, capture_output=True, text=text)


def git(args: list[str], *, cwd: Path | None = None, check: bool = False) -> subprocess.CompletedProcess[str]:
    return run(["git", *args], cwd=cwd, check=check)


def repo_root() -> Path:
    result = git(["rev-parse", "--show-toplevel"], check=True)
    return Path(result.stdout.strip()).resolve()


def parse_worktrees(root: Path) -> list[Worktree]:
    result = git(["worktree", "list", "--porcelain"], cwd=root, check=True)
    records = result.stdout.strip().split("\n\n")
    worktrees: list[Worktree] = []

    for record in records:
        if not record.strip():
            continue
        fields: dict[str, str | bool] = {}
        for line in record.splitlines():
            if " " in line:
                key, value = line.split(" ", 1)
                fields[key] = value
            else:
                fields[line] = True

        path_value = fields.get("worktree")
        if not isinstance(path_value, str):
            continue

        branch_value = fields.get("branch")
        head_value = fields.get("HEAD")
        worktrees.append(
            Worktree(
                path=Path(path_value).resolve(),
                branch_ref=branch_value if isinstance(branch_value, str) else None,
                head=head_value if isinstance(head_value, str) else None,
                detached=bool(fields.get("detached")),
                bare=bool(fields.get("bare")),
            )
        )

    return worktrees


def is_dirty(path: Path) -> bool:
    result = git(["status", "--porcelain"], cwd=path)
    return bool(result.stdout.strip())


def ref_exists(root: Path, ref: str) -> bool:
    return git(["rev-parse", "--verify", "--quiet", ref], cwd=root).returncode == 0


def is_ancestor(root: Path, branch: str, base_ref: str) -> bool:
    return git(["merge-base", "--is-ancestor", branch, base_ref], cwd=root).returncode == 0


def gh_available() -> bool:
    return shutil.which("gh") is not None


def github_merged_pr(branch: str, base_branch: str) -> tuple[bool, str | None]:
    if not gh_available():
        return False, None

    result = run(
        [
            "gh",
            "pr",
            "list",
            "--state",
            "merged",
            "--head",
            branch,
            "--base",
            base_branch,
            "--limit",
            "1",
            "--json",
            "number,title,url,mergedAt",
        ]
    )
    if result.returncode != 0:
        return False, None

    try:
        prs = json.loads(result.stdout)
    except json.JSONDecodeError:
        return False, None

    if not prs:
        return False, None

    return True, prs[0].get("url")


def decide(
    root: Path,
    wt: Worktree,
    *,
    current_path: Path,
    base_ref: str,
    base_branch: str,
    protected_branches: set[str],
    use_github: bool,
) -> Decision:
    branch = wt.branch
    if wt.bare:
        return Decision(wt, False, "bare repository")
    if wt.detached or branch is None:
        return Decision(wt, False, "detached or non-local branch")
    if wt.path == current_path:
        return Decision(wt, False, "current worktree")
    if branch in protected_branches:
        return Decision(wt, False, f"protected branch '{branch}'")
    if not wt.path.exists():
        return Decision(wt, False, "path no longer exists; run git worktree prune separately")
    if is_dirty(wt.path):
        return Decision(wt, False, "has uncommitted changes")

    if is_ancestor(root, branch, base_ref):
        return Decision(wt, True, f"branch '{branch}' is contained in {base_ref}")

    if use_github:
        merged, pr_url = github_merged_pr(branch, base_branch)
        if merged:
            return Decision(wt, True, f"GitHub reports branch '{branch}' merged into {base_branch}", pr_url)

    return Decision(wt, False, f"branch '{branch}' is not confirmed merged into {base_branch}")


def remove_worktree(root: Path, decision: Decision) -> bool:
    result = git(["worktree", "remove", str(decision.worktree.path)], cwd=root)
    if result.returncode == 0:
        return True
    print(f"FAILED remove {decision.worktree.path}: {result.stderr.strip()}", file=sys.stderr)
    return False


def delete_branch(root: Path, branch: str) -> bool:
    result = git(["branch", "-d", branch], cwd=root)
    if result.returncode == 0:
        return True
    print(f"KEPT branch {branch}: {result.stderr.strip()}", file=sys.stderr)
    return False


def format_decision(decision: Decision) -> str:
    branch = decision.worktree.branch or "<none>"
    state = "REMOVE" if decision.removable else "SKIP"
    line = f"{state:6} {decision.worktree.path} [{branch}] - {decision.reason}"
    if decision.pr_url:
        line += f" ({decision.pr_url})"
    return line


def main() -> int:
    parser = argparse.ArgumentParser(
        description="List or remove local Git worktrees whose branches are merged to main."
    )
    parser.add_argument("--apply", action="store_true", help="remove safe candidate worktree folders")
    parser.add_argument(
        "--delete-branches",
        action="store_true",
        help="after removing a worktree, delete its local branch with git branch -d",
    )
    parser.add_argument("--base-ref", default="origin/main", help="base ref used for Git merge checks")
    parser.add_argument("--base-branch", default="main", help="base branch used for GitHub PR checks")
    parser.add_argument("--no-fetch", action="store_true", help="skip git fetch before checking")
    parser.add_argument("--no-github", action="store_true", help="skip GitHub CLI merged-PR checks")
    parser.add_argument(
        "--protected",
        action="append",
        default=[],
        help="extra branch name to protect from removal; may be used multiple times",
    )
    args = parser.parse_args()

    root = repo_root()
    current_path = Path.cwd().resolve()
    protected_branches = PROTECTED_BRANCHES | set(args.protected)

    if not args.no_fetch:
        fetch = git(["fetch", "origin", args.base_branch, "--prune"], cwd=root)
        if fetch.returncode != 0:
            print(f"WARNING fetch failed: {fetch.stderr.strip()}", file=sys.stderr)

    if not ref_exists(root, args.base_ref):
        print(f"ERROR base ref '{args.base_ref}' was not found.", file=sys.stderr)
        return 2

    decisions = [
        decide(
            root,
            wt,
            current_path=current_path,
            base_ref=args.base_ref,
            base_branch=args.base_branch,
            protected_branches=protected_branches,
            use_github=not args.no_github,
        )
        for wt in parse_worktrees(root)
    ]

    removable = [decision for decision in decisions if decision.removable]
    skipped = [decision for decision in decisions if not decision.removable]

    print(f"Repository: {root}")
    print(f"Base:       {args.base_ref}")
    print(f"Mode:       {'apply' if args.apply else 'dry-run'}")
    print()

    for decision in decisions:
        print(format_decision(decision))

    print()
    print(f"Candidates: {len(removable)} removable, {len(skipped)} skipped")

    if not args.apply:
        print("Dry run only. Re-run with --apply to remove candidate worktree folders.")
        return 0

    removed: list[Decision] = []
    for decision in removable:
        if remove_worktree(root, decision):
            removed.append(decision)
            branch = decision.worktree.branch
            if args.delete_branches and branch:
                delete_branch(root, branch)

    print()
    print(f"Removed {len(removed)} worktree folder(s).")
    if args.delete_branches:
        print("Requested local branch deletion with git branch -d; unmerged branches were kept.")
    else:
        print("Local branch refs were kept.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
