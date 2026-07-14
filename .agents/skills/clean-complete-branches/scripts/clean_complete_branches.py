#!/usr/bin/env python3
"""Find or remove completed local and remote Git branches plus worktrees."""

from __future__ import annotations

import argparse
import json
import shutil
import subprocess
import sys
from dataclasses import dataclass, field
from pathlib import Path


PROTECTED_BRANCHES = {"main", "master", "develop", "dev", "release", "gh-pages"}


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
class BranchCandidate:
    name: str
    local_ref: str | None = None
    remote_ref: str | None = None
    worktrees: list[Worktree] = field(default_factory=list)
    complete: bool = False
    skip_reason: str | None = None
    evidence: str | None = None
    pr_url: str | None = None


def run(
    args: list[str],
    *,
    cwd: Path | None = None,
    check: bool = False,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(args, cwd=cwd, check=check, capture_output=True, text=True)


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


def branch_names(root: Path, ref_prefix: str) -> list[str]:
    result = git(["for-each-ref", "--format=%(refname:short)", ref_prefix], cwd=root, check=True)
    return [line.strip() for line in result.stdout.splitlines() if line.strip()]


def local_branches(root: Path) -> list[str]:
    return branch_names(root, "refs/heads")


def remote_branches(root: Path, remote: str) -> list[str]:
    branches = []
    for ref in branch_names(root, f"refs/remotes/{remote}"):
        if ref == f"{remote}/HEAD":
            continue
        if ref.startswith(f"{remote}/"):
            branches.append(ref.removeprefix(f"{remote}/"))
    return branches


def is_dirty(path: Path) -> bool:
    result = git(["status", "--porcelain"], cwd=path)
    return bool(result.stdout.strip())


def ref_exists(root: Path, ref: str) -> bool:
    return git(["rev-parse", "--verify", "--quiet", ref], cwd=root).returncode == 0


def is_ancestor(root: Path, ref: str, base_ref: str) -> bool:
    return git(["merge-base", "--is-ancestor", ref, base_ref], cwd=root).returncode == 0


def gh_available() -> bool:
    return shutil.which("gh") is not None


def gh_prs(branch: str, base_branch: str, state: str) -> list[dict[str, object]]:
    if not gh_available():
        return []

    result = run(
        [
            "gh",
            "pr",
            "list",
            "--state",
            state,
            "--head",
            branch,
            "--base",
            base_branch,
            "--limit",
            "5",
            "--json",
            "number,title,url,mergedAt,state",
        ]
    )
    if result.returncode != 0:
        return []

    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError:
        return []

    if not isinstance(data, list):
        return []
    return [item for item in data if isinstance(item, dict)]


def has_open_pr(branch: str, base_branch: str) -> tuple[bool, str | None]:
    prs = gh_prs(branch, base_branch, "open")
    if not prs:
        return False, None
    url = prs[0].get("url")
    return True, url if isinstance(url, str) else None


def merged_pr(branch: str, base_branch: str) -> tuple[bool, str | None]:
    prs = gh_prs(branch, base_branch, "merged")
    if not prs:
        return False, None
    url = prs[0].get("url")
    return True, url if isinstance(url, str) else None


def current_branch(root: Path) -> str | None:
    result = git(["branch", "--show-current"], cwd=root)
    branch = result.stdout.strip()
    return branch or None


def build_candidates(root: Path, remote: str) -> dict[str, BranchCandidate]:
    candidates: dict[str, BranchCandidate] = {}

    for branch in local_branches(root):
        candidates.setdefault(branch, BranchCandidate(name=branch)).local_ref = branch

    for branch in remote_branches(root, remote):
        candidates.setdefault(branch, BranchCandidate(name=branch)).remote_ref = f"{remote}/{branch}"

    for wt in parse_worktrees(root):
        branch = wt.branch
        if branch:
            candidates.setdefault(branch, BranchCandidate(name=branch)).worktrees.append(wt)

    return candidates


def decide_candidate(
    root: Path,
    candidate: BranchCandidate,
    *,
    base_ref: str,
    base_branch: str,
    protected_branches: set[str],
    current: str | None,
    use_github: bool,
) -> BranchCandidate:
    branch = candidate.name
    if branch in protected_branches:
        candidate.skip_reason = f"protected branch '{branch}'"
        return candidate
    if branch == current:
        candidate.skip_reason = "current branch"
        return candidate

    for wt in candidate.worktrees:
        if wt.bare:
            candidate.skip_reason = "bare repository worktree"
            return candidate
        if wt.detached:
            candidate.skip_reason = "detached worktree"
            return candidate
        if wt.path.exists() and is_dirty(wt.path):
            candidate.skip_reason = f"worktree has uncommitted changes: {wt.path}"
            return candidate

    if use_github:
        open_pr, open_pr_url = has_open_pr(branch, base_branch)
        if open_pr:
            candidate.skip_reason = "open GitHub PR"
            candidate.pr_url = open_pr_url
            return candidate

    refs_to_check = [ref for ref in [candidate.local_ref, candidate.remote_ref] if ref]
    for ref in refs_to_check:
        if is_ancestor(root, ref, base_ref):
            candidate.complete = True
            candidate.evidence = f"{ref} is contained in {base_ref}"
            return candidate

    if use_github:
        merged, pr_url = merged_pr(branch, base_branch)
        if merged:
            candidate.complete = True
            candidate.evidence = f"GitHub reports a merged PR into {base_branch}"
            candidate.pr_url = pr_url
            return candidate

    candidate.skip_reason = f"not confirmed complete against {base_branch}"
    return candidate


def format_candidate(candidate: BranchCandidate) -> str:
    state = "CLEAN" if candidate.complete else "SKIP"
    places = []
    if candidate.local_ref:
        places.append("local")
    if candidate.remote_ref:
        places.append("remote")
    if candidate.worktrees:
        places.append(f"{len(candidate.worktrees)} worktree(s)")
    place_text = ", ".join(places) if places else "unknown"
    reason = candidate.evidence if candidate.complete else candidate.skip_reason
    line = f"{state:5} {candidate.name:45} [{place_text}] - {reason}"
    if candidate.pr_url:
        line += f" ({candidate.pr_url})"
    return line


def remove_worktrees(root: Path, candidate: BranchCandidate) -> int:
    removed = 0
    for wt in candidate.worktrees:
        if not wt.path.exists():
            continue
        result = git(["worktree", "remove", str(wt.path)], cwd=root)
        if result.returncode == 0:
            removed += 1
        else:
            print(f"FAILED worktree remove {wt.path}: {result.stderr.strip()}", file=sys.stderr)
    return removed


def delete_local_branch(root: Path, branch: str, *, force: bool) -> bool:
    flag = "-D" if force else "-d"
    result = git(["branch", flag, branch], cwd=root)
    if result.returncode == 0:
        return True
    print(f"KEPT local branch {branch}: {result.stderr.strip()}", file=sys.stderr)
    return False


def delete_remote_branch(root: Path, remote: str, branch: str) -> bool:
    result = git(["push", remote, "--delete", branch], cwd=root)
    if result.returncode == 0:
        return True
    print(f"KEPT remote branch {remote}/{branch}: {result.stderr.strip()}", file=sys.stderr)
    return False


def main() -> int:
    parser = argparse.ArgumentParser(
        description="List or remove completed Git branches, remote branches, and worktrees."
    )
    parser.add_argument("--apply", action="store_true", help="remove complete local worktrees and branches")
    parser.add_argument(
        "--delete-remotes",
        action="store_true",
        help="with --apply, delete matching completed remote branches from GitHub",
    )
    parser.add_argument("--remote", default="origin", help="remote name to inspect and delete from")
    parser.add_argument("--base-ref", default="origin/main", help="base ref used for Git merge checks")
    parser.add_argument("--base-branch", default="main", help="base branch used for GitHub PR checks")
    parser.add_argument("--no-fetch", action="store_true", help="skip git fetch before checking")
    parser.add_argument("--no-github", action="store_true", help="skip GitHub CLI PR checks")
    parser.add_argument(
        "--protected",
        action="append",
        default=[],
        help="extra branch name to protect from removal; may be used multiple times",
    )
    args = parser.parse_args()

    root = repo_root()
    protected_branches = PROTECTED_BRANCHES | set(args.protected)
    use_github = not args.no_github

    if not args.no_fetch:
        fetch = git(["fetch", args.remote, args.base_branch, "--prune"], cwd=root)
        if fetch.returncode != 0:
            print(f"WARNING fetch failed: {fetch.stderr.strip()}", file=sys.stderr)

    if not ref_exists(root, args.base_ref):
        print(f"ERROR base ref '{args.base_ref}' was not found.", file=sys.stderr)
        return 2

    candidates = build_candidates(root, args.remote)
    current = current_branch(root)
    decisions = [
        decide_candidate(
            root,
            candidate,
            base_ref=args.base_ref,
            base_branch=args.base_branch,
            protected_branches=protected_branches,
            current=current,
            use_github=use_github,
        )
        for candidate in sorted(candidates.values(), key=lambda item: item.name)
    ]

    complete = [candidate for candidate in decisions if candidate.complete]
    skipped = [candidate for candidate in decisions if not candidate.complete]

    print(f"Repository:      {root}")
    print(f"Base:            {args.base_ref}")
    print(f"Remote:          {args.remote}")
    print(f"GitHub checks:   {'on' if use_github and gh_available() else 'off'}")
    print(f"Mode:            {'apply' if args.apply else 'dry-run'}")
    print(f"Delete remotes:  {'yes' if args.apply and args.delete_remotes else 'no'}")
    print()

    for candidate in decisions:
        print(format_candidate(candidate))

    print()
    print(f"Candidates: {len(complete)} complete, {len(skipped)} skipped")

    if not args.apply:
        print("Dry run only. Re-run with --apply to remove local candidates.")
        print("Add --delete-remotes with --apply to delete matching GitHub remote branches.")
        return 0

    removed_worktrees = 0
    deleted_local = 0
    deleted_remote = 0

    for candidate in complete:
        removed_worktrees += remove_worktrees(root, candidate)

        if candidate.local_ref:
            force = bool(candidate.pr_url) and not is_ancestor(root, candidate.local_ref, args.base_ref)
            if delete_local_branch(root, candidate.name, force=force):
                deleted_local += 1

        if args.delete_remotes and candidate.remote_ref:
            if delete_remote_branch(root, args.remote, candidate.name):
                deleted_remote += 1

    print()
    print(f"Removed {removed_worktrees} worktree folder(s).")
    print(f"Deleted {deleted_local} local branch ref(s).")
    if args.delete_remotes:
        print(f"Deleted {deleted_remote} GitHub remote branch ref(s).")
    else:
        print("GitHub remote branches were kept.")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
