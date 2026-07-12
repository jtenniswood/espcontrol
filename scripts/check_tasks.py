#!/usr/bin/env python3
"""Plan and run the EspControl validation task graph."""

from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
import os
import signal
import subprocess
import sys
import time
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import TextIO

from check_tasks_data import DOMAINS, PROFILES, TASKS, Task


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "scripts" / "fixtures" / "check_tasks_legacy_coverage.json"


class ConfigurationError(ValueError):
    pass


SUMMARY_SCHEMA_VERSION = 1
TASK_STATUSES = ("passed", "failed", "blocked", "not_run", "cached")


def validate_registry(tasks: tuple[Task, ...] = TASKS) -> dict[str, Task]:
    registry: dict[str, Task] = {}
    for item in tasks:
        if item.id in registry:
            raise ConfigurationError(f"duplicate task ID: {item.id}")
        registry[item.id] = item
        if not item.commands or any(not command for command in item.commands):
            raise ConfigurationError(f"task {item.id} has an empty command")
        invalid_profiles = set(item.profiles) - set(PROFILES)
        invalid_domains = set(item.domains) - set(DOMAINS)
        if invalid_profiles:
            raise ConfigurationError(f"task {item.id} has invalid profiles: {sorted(invalid_profiles)}")
        if invalid_domains:
            raise ConfigurationError(f"task {item.id} has invalid domains: {sorted(invalid_domains)}")
        if item.cache != "never" and not item.inputs:
            raise ConfigurationError(f"cacheable task {item.id} has no inputs")
    for item in tasks:
        missing = set(item.dependencies) - set(registry)
        if missing:
            raise ConfigurationError(f"task {item.id} has missing dependencies: {sorted(missing)}")

    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(task_id: str) -> None:
        if task_id in visiting:
            raise ConfigurationError(f"dependency cycle includes task {task_id}")
        if task_id in visited:
            return
        visiting.add(task_id)
        for dependency in registry[task_id].dependencies:
            visit(dependency)
        visiting.remove(task_id)
        visited.add(task_id)

    for item in tasks:
        visit(item.id)
    return registry


def plan(profile: str, domain: str | None = None, tasks: tuple[Task, ...] = TASKS) -> list[Task]:
    registry = validate_registry(tasks)
    selected = {item.id for item in tasks if profile in item.profiles and (domain is None or domain in item.domains)}

    def add_dependencies(task_id: str) -> None:
        for dependency in registry[task_id].dependencies:
            if dependency not in selected:
                selected.add(dependency)
                add_dependencies(dependency)

    for task_id in tuple(selected):
        add_dependencies(task_id)

    ordered: list[Task] = []
    emitted: set[str] = set()

    def emit(task_id: str) -> None:
        if task_id in emitted:
            return
        for dependency in registry[task_id].dependencies:
            emit(dependency)
        emitted.add(task_id)
        ordered.append(registry[task_id])

    for item in tasks:
        if item.id in selected:
            emit(item.id)
    return ordered


def plan_task(task_id: str, tasks: tuple[Task, ...] = TASKS) -> list[Task]:
    registry = validate_registry(tasks)
    if task_id not in registry:
        raise ConfigurationError(f"unknown task ID: {task_id}")
    ordered: list[Task] = []
    emitted: set[str] = set()

    def emit(selected_id: str) -> None:
        if selected_id in emitted:
            return
        for dependency in registry[selected_id].dependencies:
            emit(dependency)
        emitted.add(selected_id)
        ordered.append(registry[selected_id])

    emit(task_id)
    return ordered


def task_json(item: Task) -> dict[str, object]:
    return {
        "id": item.id,
        "commands": [list(command) for command in item.commands],
        "dependencies": list(item.dependencies),
        "profiles": list(item.profiles),
        "domains": list(item.domains),
        "inputs": list(item.inputs),
        "generated_inputs": list(item.generated_inputs),
        "cache": item.cache,
    }


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


def git_revision(root: Path) -> str | None:
    try:
        result = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        )
    except (FileNotFoundError, subprocess.CalledProcessError):
        return None
    return result.stdout.strip() or None


def depends_on(task_id: str, failed_id: str, registry: dict[str, Task]) -> bool:
    return failed_id in registry[task_id].dependencies or any(
        depends_on(dependency, failed_id, registry)
        for dependency in registry[task_id].dependencies
    )


def run_command(command: tuple[str, ...], root: Path) -> int:
    process: subprocess.Popen[bytes] | None = None
    interrupted = False
    previous_handlers: dict[int, object] = {}

    def forward(signum: int, _frame: object) -> None:
        nonlocal interrupted
        interrupted = True
        if process is None or process.poll() is not None:
            return
        try:
            os.killpg(process.pid, signum)
        except (ProcessLookupError, PermissionError):
            process.send_signal(signum)

    for signum in (signal.SIGINT, signal.SIGTERM):
        previous_handlers[signum] = signal.getsignal(signum)
        signal.signal(signum, forward)
    try:
        try:
            process = subprocess.Popen(command, cwd=root, start_new_session=True)
        except FileNotFoundError:
            print(f"error: executable not found: {command[0]}", file=sys.stderr)
            return 1
        return_code = process.wait()
        return 130 if interrupted or return_code < 0 else return_code
    finally:
        for signum, handler in previous_handlers.items():
            signal.signal(signum, handler)


def execute_tasks(
    selected: list[Task],
    root: Path,
    *,
    profile: str | None,
    domain: str | None,
    requested_task: str | None,
) -> tuple[int, dict[str, object]]:
    started_at = utc_now()
    run_started = time.monotonic()
    results: list[dict[str, object]] = []
    failed_id: str | None = None
    exit_code = 0
    registry = validate_registry(tuple(selected))

    for item in selected:
        if failed_id is not None:
            status = "blocked" if depends_on(item.id, failed_id, registry) else "not_run"
            results.append({
                "id": item.id,
                "status": status,
                "duration_seconds": 0.0,
                "exit_code": None,
                "commands": [list(command) for command in item.commands],
            })
            continue

        print(f"\n==> {item.id}", flush=True)
        task_started = time.monotonic()
        task_exit = 0
        for command in item.commands:
            print(f"$ {' '.join(command)}", flush=True)
            task_exit = run_command(command, root)
            if task_exit != 0:
                break
        duration = round(time.monotonic() - task_started, 3)
        status = "passed" if task_exit == 0 else "failed"
        results.append({
            "id": item.id,
            "status": status,
            "duration_seconds": duration,
            "exit_code": task_exit,
            "commands": [list(command) for command in item.commands],
        })
        if task_exit != 0:
            failed_id = item.id
            exit_code = 130 if task_exit == 130 else 1

    duration = round(time.monotonic() - run_started, 3)
    summary: dict[str, object] = {
        "schema_version": SUMMARY_SCHEMA_VERSION,
        "profile": profile,
        "domain": domain,
        "requested_task": requested_task,
        "git_revision": git_revision(root),
        "started_at": started_at,
        "finished_at": utc_now(),
        "duration_seconds": duration,
        "cache": {"enabled": False, "reason": "result caching is not implemented in this stage"},
        "status": "passed" if exit_code == 0 else "interrupted" if exit_code == 130 else "failed",
        "exit_code": exit_code,
        "tasks": results,
    }
    return exit_code, summary


def summary_markdown(summary: dict[str, object]) -> str:
    lines = [
        "## Check task graph",
        "",
        f"Overall: **{summary['status']}** in {summary['duration_seconds']:.3f}s",
        "",
        "| Task | Status | Duration |",
        "| --- | --- | ---: |",
    ]
    for result in summary["tasks"]:
        lines.append(f"| `{result['id']}` | {result['status']} | {result['duration_seconds']:.3f}s |")
    return "\n".join(lines) + "\n"


def print_summary(summary: dict[str, object], output: TextIO = sys.stdout) -> None:
    print("\nCheck task summary", file=output)
    print(f"{'TASK':28} {'STATUS':10} {'SECONDS':>8}", file=output)
    for result in summary["tasks"]:
        print(f"{result['id']:28} {result['status']:10} {result['duration_seconds']:8.3f}", file=output)
    print(f"Overall: {summary['status']} ({summary['duration_seconds']:.3f}s)", file=output)


def write_summary(summary: dict[str, object], path: Path | None) -> None:
    if path is not None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    github_summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if github_summary:
        with Path(github_summary).open("a", encoding="utf-8") as handle:
            handle.write(summary_markdown(summary))


def self_test() -> None:
    validate_registry()
    expected = json.loads(FIXTURE.read_text())
    actual = {profile: sorted(item.id for item in plan(profile)) for profile in PROFILES}
    for profile in PROFILES:
        wanted = sorted(expected[profile])
        if actual[profile] != wanted:
            raise ConfigurationError(f"legacy coverage differs for {profile}: expected {wanted}, got {actual[profile]}")

    product_order = [item.id for item in plan("product")]
    if product_order.index("device-slots") > product_order.index("device-profiles"):
        raise AssertionError("device slot outputs are not checked before device profiles consume them")

    web_order = [item.id for item in plan("ci", "web")]
    for consumer in ("web-smoke", "web-browser-smoke"):
        if web_order.index("device-manifest-output") > web_order.index(consumer):
            raise AssertionError(f"device manifest output is not checked before {consumer} consumes it")

    firmware_order = [item.id for item in plan("fast", "firmware")]
    for consumer in ("firmware-parser", "firmware-ha-bindings", "device-profiles"):
        if firmware_order.index("device-slots") > firmware_order.index(consumer):
            raise AssertionError(f"device slot outputs are not checked before {consumer} consumes them")

    registry = validate_registry()
    package_scripts = json.loads((ROOT / "package.json").read_text())["scripts"]
    profile_aliases = {
        "check:product": "product",
        "check:fast": "fast",
        "check:ci": "ci",
        "check:all": "all",
        "check:release-preflight": "release",
    }
    for alias, profile in profile_aliases.items():
        expected_command = f"python3 scripts/check_tasks.py run {profile}"
        if package_scripts.get(alias) != expected_command:
            raise AssertionError(f"{alias} does not route through the {profile} graph")

    public_aliases = {
        name: command for name, command in package_scripts.items()
        if name.startswith("check:") and not name.endswith(":legacy")
    }
    for alias, command in public_aliases.items():
        if alias in profile_aliases:
            continue
        task_id = alias.removeprefix("check:")
        if task_id not in registry:
            raise AssertionError(f"{alias} has no matching registered task")
        expected_command = f"python3 scripts/check_tasks.py run-task {task_id}"
        if command != expected_command:
            raise AssertionError(f"{alias} does not route through run-task {task_id}")
    missing_legacy = sorted(alias for alias in public_aliases if f"{alias}:legacy" not in package_scripts)
    if missing_legacy:
        raise AssertionError(f"public check aliases are missing temporary legacy commands: {missing_legacy}")

    if registry["types"].commands != (("npm", "exec", "--", "tsc", "--noEmit"),):
        raise AssertionError("TypeScript checks do not use the project-managed compiler")
    if "icon-groups" not in {item.id for item in plan("fast", "docs")}:
        raise AssertionError("docs plans do not validate icon gallery groups")
    if "docs/.vitepress/theme/components/IconGallery.vue" not in registry["icon-groups"].inputs:
        raise AssertionError("icon gallery changes do not invalidate icon-group checks")

    release_workflow_order = [item.id for item in plan("release", "workflow")]
    for prerequisite in ("generated", "device-manifest-output"):
        if release_workflow_order.index(prerequisite) > release_workflow_order.index("release-confidence"):
            raise AssertionError(f"{prerequisite} is not checked before release confidence consumes it")

    def expect_invalid(tasks: tuple[Task, ...], description: str) -> None:
        try:
            validate_registry(tasks)
        except ConfigurationError:
            return
        raise AssertionError(f"{description} was accepted")

    expect_invalid((TASKS[0], TASKS[0]), "duplicate IDs")
    expect_invalid((Task("missing", (("true",),), dependencies=("absent",), inputs=("x",)),), "missing dependency")
    expect_invalid((Task("profile", (("true",),), profiles=("unknown",), inputs=("x",)),), "invalid profile")
    expect_invalid((Task("domain", (("true",),), domains=("unknown",), inputs=("x",)),), "invalid domain")
    expect_invalid((Task("inputs", (("true",),), cache="deterministic"),), "cacheable task with empty inputs")

    first = Task("first", (("true",),), dependencies=("second",), inputs=("x",))
    second = Task("second", (("true",),), dependencies=("first",), inputs=("x",))
    expect_invalid((first, second), "dependency cycle")

    ordered = (
        Task("consumer", (("true",),), dependencies=("shared",), profiles=("fast",), inputs=("x",)),
        Task("independent", (("true",),), profiles=("fast",), inputs=("x",)),
        Task("shared", (("true",),), inputs=("x",)),
    )
    if [item.id for item in plan("fast", tasks=ordered)] != ["shared", "consumer", "independent"]:
        raise AssertionError("dependency ordering or de-duplication is not stable")

    if [item.id for item in plan_task("consumer", ordered)] != ["shared", "consumer"]:
        raise AssertionError("run-task dependency closure is incorrect")

    with TemporaryDirectory() as tmp:
        root = Path(tmp)
        marker = root / "marker.txt"
        fake_tasks = [
            Task("pass", ((sys.executable, "-c", f"from pathlib import Path; Path({str(marker)!r}).write_text('pass')"),)),
            Task("fail", ((sys.executable, "-c", "raise SystemExit(7)"),), dependencies=("pass",)),
            Task("blocked", ((sys.executable, "-c", "raise SystemExit(0)"),), dependencies=("fail",)),
            Task("not-run", ((sys.executable, "-c", "raise SystemExit(0)"),)),
        ]
        code, summary = execute_tasks(fake_tasks, root, profile="fast", domain=None, requested_task=None)
        statuses = {item["id"]: item["status"] for item in summary["tasks"]}
        if code != 1 or statuses != {"pass": "passed", "fail": "failed", "blocked": "blocked", "not-run": "not_run"}:
            raise AssertionError(f"fail-fast execution is incorrect: {code}, {statuses}")
        if marker.read_text() != "pass":
            raise AssertionError("successful fake command did not execute")
        if summary["schema_version"] != SUMMARY_SCHEMA_VERSION or summary["exit_code"] != 1:
            raise AssertionError("JSON summary schema or exit code is incorrect")
        markdown = summary_markdown(summary)
        if "| `blocked` | blocked |" not in markdown:
            raise AssertionError("Markdown summary omits blocked tasks")

        missing_code, missing_summary = execute_tasks(
            [Task("missing", (("executable-that-does-not-exist",),))],
            root,
            profile=None,
            domain=None,
            requested_task="missing",
        )
        if missing_code != 1 or missing_summary["tasks"][0]["status"] != "failed":
            raise AssertionError("missing executables are not reported as task failures")

        summary_path = root / "summary.json"
        markdown_path = root / "github-summary.md"
        previous_github_summary = os.environ.get("GITHUB_STEP_SUMMARY")
        os.environ["GITHUB_STEP_SUMMARY"] = str(markdown_path)
        try:
            write_summary(summary, summary_path)
        finally:
            if previous_github_summary is None:
                os.environ.pop("GITHUB_STEP_SUMMARY", None)
            else:
                os.environ["GITHUB_STEP_SUMMARY"] = previous_github_summary
        if json.loads(summary_path.read_text())["schema_version"] != SUMMARY_SCHEMA_VERSION:
            raise AssertionError("JSON summary file has the wrong schema version")
        if "## Check task graph" not in markdown_path.read_text():
            raise AssertionError("GitHub Markdown summary was not written")

        interrupt_test = (
            "import os, signal, sys, threading; "
            "from pathlib import Path; "
            "from check_tasks import run_command; "
            "threading.Timer(0.1, lambda: os.kill(os.getpid(), signal.SIGINT)).start(); "
            "child = \"import os, signal, time; signal.signal(signal.SIGINT, lambda *_: os._exit(130)); time.sleep(30)\"; "
            "code = run_command((sys.executable, '-c', child), Path.cwd()); "
            "raise SystemExit(0 if code == 130 else 1)"
        )
        interrupted = subprocess.run(
            [sys.executable, "-c", interrupt_test],
            cwd=ROOT,
            env={**os.environ, "PYTHONPATH": str(ROOT / "scripts")},
            timeout=5,
        )
        if interrupted.returncode != 0:
            raise AssertionError("interrupts are not forwarded to the active child process")

    print(f"check task registry self-test passed ({len(TASKS)} tasks, {len(PROFILES)} profiles)")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    subparsers = parser.add_subparsers(dest="command")
    list_parser = subparsers.add_parser("list", help="list registered tasks")
    list_parser.add_argument("--json", action="store_true")
    plan_parser = subparsers.add_parser("plan", help="show the tasks selected by a profile")
    plan_parser.add_argument("profile", choices=PROFILES)
    plan_parser.add_argument("--domain", choices=DOMAINS)
    plan_parser.add_argument("--json", action="store_true")
    plan_parser.add_argument("--explain", action="store_true")
    run_parser = subparsers.add_parser("run", help="run the tasks selected by a profile")
    run_parser.add_argument("profile", choices=PROFILES)
    run_parser.add_argument("--domain", choices=DOMAINS)
    run_parser.add_argument("--jobs", type=int, default=1)
    run_parser.add_argument("--no-cache", action="store_true")
    run_parser.add_argument("--summary-json", type=Path)
    task_parser = subparsers.add_parser("run-task", help="run one task and its dependencies")
    task_parser.add_argument("task_id")
    task_parser.add_argument("--summary-json", type=Path)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        if args.self_test:
            self_test()
            return 0
        validate_registry()
        if args.command == "list":
            if args.json:
                print(json.dumps([task_json(item) for item in TASKS], indent=2))
            else:
                for item in TASKS:
                    print(f"{item.id:28} {', '.join(item.domains)}")
            return 0
        if args.command == "plan":
            selected = plan(args.profile, args.domain)
            if args.json:
                print(json.dumps({"profile": args.profile, "domain": args.domain, "tasks": [task_json(item) for item in selected]}, indent=2))
            else:
                for index, item in enumerate(selected, 1):
                    reason = ""
                    if args.explain:
                        reason = f"  ({'dependency' if args.profile not in item.profiles else 'profile'})"
                    print(f"{index:2}. {item.id}{reason}")
            return 0
        if args.command == "run":
            if args.jobs != 1:
                raise ConfigurationError("only --jobs 1 is available until parallel execution is introduced")
            selected = plan(args.profile, args.domain)
            exit_code, summary = execute_tasks(
                selected, ROOT, profile=args.profile, domain=args.domain, requested_task=None
            )
            print_summary(summary)
            write_summary(summary, args.summary_json)
            return exit_code
        if args.command == "run-task":
            selected = plan_task(args.task_id)
            exit_code, summary = execute_tasks(
                selected, ROOT, profile=None, domain=None, requested_task=args.task_id
            )
            print_summary(summary)
            write_summary(summary, args.summary_json)
            return exit_code
        print("choose 'list', 'plan', 'run', or 'run-task', or use --self-test", file=sys.stderr)
        return 2
    except ConfigurationError as error:
        print(f"check task configuration error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
