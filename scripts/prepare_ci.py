#!/usr/bin/env python3
"""Regenerate derived files and run the PR checks before pushing a branch."""

from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import sys

from check_tasks import run_command


ROOT = Path(__file__).resolve().parents[1]
PYTHON = sys.executable
# Authored sources must agree before any derived files are rewritten. This does
# not bless compatibility fixtures or choose between conflicting product edits.
PREPARE = (
    ("npm", "ci", "--no-audit", "--no-fund"),
    (PYTHON, "scripts/check_product_model_v2.py"),
    (PYTHON, "scripts/generate_device_manifest.py"),
    (PYTHON, "scripts/generate_device_slots.py"),
    (PYTHON, "scripts/build.py"),
    (PYTHON, "scripts/check_product_snapshot.py", "--update"),
)
CHECKS = (
    (PYTHON, "scripts/check_mipi_rgb_models.py"),
    (PYTHON, "scripts/check_tasks.py", "run", "ci", "--no-cache",
     "--summary-json", ".cache/prepare-ci-summary.json"),
)
DOCS_BUILD = ("npm", "run", "docs:build")
BROWSER_INSTALL = ("npx", "playwright", "install", "chromium")
BROWSER_CHECK = ("npm", "run", "docs:check-installer")


def prepare(run=run_command) -> int:
    for command in PREPARE:
        code = run(command, ROOT)
        if code:
            print("Preparation stopped. Fix the error above, then rerun npm run prepare:ci.")
            return code
    failures = []
    for command in CHECKS:
        code = run(command, ROOT)
        if code == 130:
            return code
        if code:
            failures.append(command)
    code = run(DOCS_BUILD, ROOT)
    if code == 130:
        return code
    if code:
        failures.append(DOCS_BUILD)
        print("Browser installer check skipped because the docs build failed.")
    else:
        code = run(BROWSER_INSTALL, ROOT)
        if code == 130:
            return code
        if code:
            failures.append(BROWSER_INSTALL)
        else:
            code = run(BROWSER_CHECK, ROOT)
            if code == 130:
                return code
            if code:
                failures.append(BROWSER_CHECK)
    if failures:
        print("\nChecks requiring attention:")
        for command in failures:
            print("- " + " ".join(command))
    print("\nReview git diff and include the regenerated files with your source changes.")
    return 1 if failures else 0


def self_test() -> None:
    def check(outcomes, expected, expected_code):
        calls = []

        def fake_run(command, root):
            assert root == ROOT
            calls.append(command)
            return outcomes.get(command, 0)

        assert prepare(fake_run) == expected_code
        assert calls == expected, calls

    all_commands = list(PREPARE + CHECKS) + [DOCS_BUILD, BROWSER_INSTALL, BROWSER_CHECK]
    check({}, all_commands, 0)
    # An invalid lockfile or authored-source conflict stops before generators.
    check({PREPARE[0]: 1}, [PREPARE[0]], 1)
    check({PREPARE[1]: 1}, list(PREPARE[:2]), 1)
    # Validation failures are collected, while cancellation always stops work.
    check({CHECKS[0]: 1, CHECKS[1]: 1}, all_commands, 1)
    check({CHECKS[0]: 130}, list(PREPARE) + [CHECKS[0]], 130)
    check({BROWSER_INSTALL: 1}, all_commands[:-1], 1)
    check({DOCS_BUILD: 1}, all_commands[:-2], 1)
    print("CI preparation self-test passed.")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    node_major = int(subprocess.check_output(
        ["node", "-p", "process.versions.node.split('.')[0]"], text=True).strip())
    if node_major != 24:
        print("Use Node.js 24 to match CI before running npm run prepare:ci.")
        return 1
    return prepare()


if __name__ == "__main__":
    raise SystemExit(main())
