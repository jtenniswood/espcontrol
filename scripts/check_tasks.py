#!/usr/bin/env python3
"""Plan and run the EspControl validation task graph."""

from __future__ import annotations

import argparse
import ast
from concurrent.futures import FIRST_COMPLETED, Future, ThreadPoolExecutor, wait
from contextlib import contextmanager, redirect_stdout
from datetime import datetime, timezone
from fnmatch import fnmatchcase
import hashlib
from io import StringIO
import json
import os
import platform
import shutil
import signal
import subprocess
import sys
import threading
import time
from pathlib import Path
from tempfile import NamedTemporaryFile, TemporaryDirectory
from typing import TextIO

from check_tasks_data import DOMAINS, PROFILES, TASKS, Task


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "scripts" / "fixtures" / "check_tasks_legacy_coverage.json"


class ConfigurationError(ValueError):
    pass


SUMMARY_SCHEMA_VERSION = 1
TASK_STATUSES = ("passed", "failed", "blocked", "not_run", "cached")
CACHE_SCHEMA_VERSION = 1
CACHE_DIRECTORY = "espcontrol-check-cache-v1"
CACHE_SUCCESS_STATUSES = {"passed", "cached"}
LOCKFILES = (
    "package-lock.json",
    "npm-shrinkwrap.json",
    "pyproject.toml",
    "uv.lock",
    "poetry.lock",
    "Pipfile.lock",
    "requirements.txt",
)


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
        if item.cache not in {"never", "deterministic"}:
            raise ConfigurationError(f"task {item.id} has invalid cache policy: {item.cache}")
        if item.cache != "never" and not item.inputs:
            raise ConfigurationError(f"cacheable task {item.id} has no inputs")
        if not isinstance(item.parallel_safe, bool):
            raise ConfigurationError(f"task {item.id} has invalid parallel safety")
        if len(set(item.cache_env)) != len(item.cache_env) or any(not name for name in item.cache_env):
            raise ConfigurationError(f"task {item.id} has invalid cache environment declarations")
        if len(set(item.cache_tools)) != len(item.cache_tools) or any(not name for name in item.cache_tools):
            raise ConfigurationError(f"task {item.id} has invalid cache tool declarations")
        if len(set(item.cache_inputs)) != len(item.cache_inputs) or any(not path for path in item.cache_inputs):
            raise ConfigurationError(f"task {item.id} has invalid cache input declarations")
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


def plan_task_ids(task_ids: set[str], tasks: tuple[Task, ...] = TASKS) -> list[Task]:
    registry = validate_registry(tasks)
    unknown = task_ids - set(registry)
    if unknown:
        raise ConfigurationError(f"unknown task IDs: {sorted(unknown)}")
    selected = set(task_ids)

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


def git_output(root: Path, *args: str) -> str:
    try:
        result = subprocess.run(
            ["git", *args],
            cwd=root,
            check=True,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError as error:
        raise ConfigurationError("git executable not found") from error
    except subprocess.CalledProcessError as error:
        message = error.stderr.strip() or error.stdout.strip() or "git command failed"
        raise ConfigurationError(message) from error
    return result.stdout


def resolve_changed_base(root: Path, requested: str | None = None) -> tuple[str, str]:
    candidates = (requested,) if requested else ("origin/main", "main")
    for candidate in candidates:
        if candidate is None:
            continue
        exists = subprocess.run(
            ["git", "rev-parse", "--verify", "--quiet", f"{candidate}^{{commit}}"],
            cwd=root,
            capture_output=True,
        )
        if exists.returncode == 0:
            merge_base = subprocess.run(
                ["git", "merge-base", "HEAD", candidate],
                cwd=root,
                capture_output=True,
                text=True,
            )
            if merge_base.returncode == 0 and merge_base.stdout.strip():
                return candidate, merge_base.stdout.strip()
            if requested:
                raise ConfigurationError(f"could not find merge base with {candidate}")
    if requested:
        raise ConfigurationError(f"changed-file base ref does not exist: {requested}")
    raise ConfigurationError("changed-file routing requires origin/main or local main")


def parse_name_status(output: str) -> set[str]:
    fields = output.split("\0")
    if fields and fields[-1] == "":
        fields.pop()
    paths: set[str] = set()
    index = 0
    while index < len(fields):
        status = fields[index]
        index += 1
        path_count = 2 if status.startswith(("R", "C")) else 1
        if index + path_count > len(fields):
            raise ConfigurationError("git returned malformed changed-path data")
        paths.update(fields[index:index + path_count])
        index += path_count
    return {path for path in paths if path}


def changed_paths(root: Path, merge_base: str) -> list[str]:
    committed = git_output(root, "diff", "--name-status", "-z", "-M", f"{merge_base}..HEAD")
    tracked = git_output(root, "diff", "--name-status", "-z", "-M", merge_base)
    staged = git_output(root, "diff", "--cached", "--name-status", "-z", "-M", merge_base)
    untracked = git_output(root, "ls-files", "--others", "--exclude-standard", "-z")
    paths = parse_name_status(committed)
    paths.update(parse_name_status(tracked))
    paths.update(parse_name_status(staged))
    paths.update(path for path in untracked.split("\0") if path)
    return sorted(paths)


def matches_input(path: str, pattern: str) -> bool:
    patterns = {pattern}
    if "/**/" in pattern:
        patterns.add(pattern.replace("/**/", "/"))
    return any(fnmatchcase(path, candidate) for candidate in patterns)


def changed_plan(
    paths: list[str], tasks: tuple[Task, ...] = TASKS
) -> tuple[list[Task], dict[str, list[str]], str | None]:
    validate_registry(tasks)
    matched: dict[str, list[str]] = {}
    unmatched: list[str] = []
    force_fast: list[str] = []
    sensitive = (
        "scripts/check_tasks.py",
        "scripts/check_tasks_data.py",
        "package-lock.json",
    )
    sensitive_patterns = (
        "scripts/check_*",
        "scripts/generate_*",
    )
    catch_all_task_ids = {"public-firmware-script"}

    for path in paths:
        path_matches = []
        for item in tasks:
            patterns = item.inputs + item.generated_inputs
            if any(matches_input(path, pattern) for pattern in patterns):
                matched.setdefault(item.id, []).append(path)
                path_matches.append(item.id)
        if (
            path in sensitive
            or path == "scripts/build.py"
            or any(matches_input(path, pattern) for pattern in sensitive_patterns)
            or matches_input(path, ".github/workflows/**")
        ):
            force_fast.append(path)
        elif not (set(path_matches) - catch_all_task_ids):
            unmatched.append(path)

    fallback_paths = sorted(set(force_fast + unmatched))
    if fallback_paths:
        fast_ids = {item.id for item in plan("fast", tasks=tasks)}
        direct_ids = set(matched)
        selected = plan_task_ids(fast_ids | direct_ids, tasks)
        reason = "full fast profile required by " + ", ".join(fallback_paths)
        reasons: dict[str, list[str]] = {}
        for item in selected:
            item_reasons = []
            if item.id in fast_ids:
                item_reasons.append(reason)
            if item.id in matched:
                item_reasons.extend(
                    f"input matched {path}" for path in sorted(matched[item.id])
                )
            if not item_reasons:
                consumers = sorted(
                    task_id for task_id in direct_ids
                    if item.id in {dependency.id for dependency in plan_task(task_id, tasks)[:-1]}
                )
                item_reasons.extend(
                    f"dependency of {task_id} selected by {', '.join(sorted(matched[task_id]))}"
                    for task_id in consumers
                )
            reasons[item.id] = item_reasons
        return selected, reasons, reason

    direct_ids = set(matched)
    selected = plan_task_ids(direct_ids, tasks)
    reasons: dict[str, list[str]] = {}
    for item in selected:
        if item.id in matched:
            reasons[item.id] = [f"input matched {path}" for path in sorted(matched[item.id])]
            continue
        consumers = sorted(
            task_id for task_id in direct_ids
            if item.id in {dependency.id for dependency in plan_task(task_id, tasks)[:-1]}
        )
        reasons[item.id] = [
            f"dependency of {task_id} selected by {', '.join(sorted(matched[task_id]))}"
            for task_id in consumers
        ]
    return selected, reasons, None


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
        "parallel_safe": item.parallel_safe,
        "cache_env": list(item.cache_env),
        "cache_tools": list(item.cache_tools),
        "cache_inputs": list(item.cache_inputs),
    }


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def git_common_directory(root: Path) -> Path:
    common = Path(git_output(root, "rev-parse", "--git-common-dir").strip())
    if not common.is_absolute():
        common = root / common
    return common.resolve()


def cache_directory(root: Path) -> Path:
    return git_common_directory(root) / CACHE_DIRECTORY


class CacheKeyBuilder:
    def __init__(self, root: Path) -> None:
        self.root = root
        self._file_fingerprints: dict[Path, dict[str, object]] = {}
        self._tool_versions: dict[str, dict[str, object]] = {}
        self._browser: dict[str, object] | None = None
        project_files = git_output(root, "ls-files", "-co", "--exclude-standard", "-z")
        self._project_files = tuple(
            root / relative
            for relative in sorted(path for path in project_files.split("\0") if path)
        )

    def file_fingerprint(self, path: Path) -> dict[str, object]:
        path = path.absolute()
        if path in self._file_fingerprints:
            return self._file_fingerprints[path]
        try:
            relative = path.relative_to(self.root.absolute()).as_posix()
        except ValueError:
            relative = str(path)
        if path.is_symlink():
            value: dict[str, object] = {
                "path": relative,
                "kind": "symlink",
                "target": os.readlink(path),
            }
            resolved_target = path.resolve()
            if resolved_target.is_file():
                value["target_size"] = resolved_target.stat().st_size
                value["target_sha256"] = sha256_file(resolved_target)
        elif path.is_file():
            value = {
                "path": relative,
                "kind": "file",
                "size": path.stat().st_size,
                "sha256": sha256_file(path),
            }
        else:
            value = {"path": relative, "kind": "missing"}
        self._file_fingerprints[path] = value
        return value

    def pattern_fingerprints(self, patterns: tuple[str, ...]) -> list[dict[str, object]]:
        fingerprints: list[dict[str, object]] = []
        for pattern in patterns:
            matches = sorted(
                {
                    path
                    for path in self._project_files
                    if matches_input(path.relative_to(self.root).as_posix(), pattern)
                    and (path.is_file() or path.is_symlink())
                },
                key=lambda path: path.relative_to(self.root).as_posix(),
            )
            fingerprints.append({
                "pattern": pattern,
                "matches": [self.file_fingerprint(path) for path in matches],
            })
        return fingerprints

    def tool_version(self, executable: str) -> dict[str, object]:
        if executable in self._tool_versions:
            return self._tool_versions[executable]
        local_executable = self.root / executable
        resolved = str(local_executable) if "/" in executable and local_executable.exists() else shutil.which(executable)
        if resolved is None:
            value: dict[str, object] = {"executable": executable, "state": "missing"}
        else:
            try:
                result = subprocess.run(
                    [resolved, "--version"],
                    cwd=self.root,
                    capture_output=True,
                    text=True,
                    timeout=5,
                )
                version = (result.stdout or result.stderr).strip()
                value = {
                    "executable": executable,
                    "path": str(Path(resolved).resolve()),
                    "return_code": result.returncode,
                    "version": version,
                }
            except (OSError, subprocess.TimeoutExpired) as error:
                value = {
                    "executable": executable,
                    "path": str(Path(resolved).resolve()),
                    "error": type(error).__name__,
                }
        self._tool_versions[executable] = value
        return value

    def browser_fingerprint(self) -> dict[str, object]:
        if self._browser is not None:
            return self._browser
        script = (
            "const {chromium}=require('playwright');"
            "const p=require('playwright/package.json');"
            "console.log(JSON.stringify({version:p.version,path:chromium.executablePath()}));"
        )
        try:
            result = subprocess.run(
                ["node", "-e", script],
                cwd=self.root,
                check=True,
                capture_output=True,
                text=True,
                timeout=10,
            )
            details = json.loads(result.stdout)
            executable = Path(details["path"])
            stat = executable.stat()
            browser_version = subprocess.run(
                [str(executable), "--version"],
                capture_output=True,
                text=True,
                timeout=5,
            )
            details["executable"] = {
                "path": str(executable.resolve()),
                "size": stat.st_size,
                "mtime_ns": stat.st_mtime_ns,
                "return_code": browser_version.returncode,
                "version": (browser_version.stdout or browser_version.stderr).strip(),
            }
            self._browser = details
        except (
            FileNotFoundError,
            OSError,
            subprocess.CalledProcessError,
            subprocess.TimeoutExpired,
            json.JSONDecodeError,
            KeyError,
        ) as error:
            self._browser = {"state": "unavailable", "error": type(error).__name__}
        return self._browser

    def cacheable(self, item: Task) -> bool:
        return item.cache == "deterministic" and (
            item.id != "web-browser-smoke"
            or self.browser_fingerprint().get("state") != "unavailable"
        )

    def python_command_dependencies(self, item: Task) -> list[dict[str, object]]:
        """Fingerprint repo-local Python modules imported by task entry points."""
        pending: list[Path] = []
        for command in item.commands:
            if len(command) > 1 and Path(command[1]).suffix == ".py":
                candidate = (self.root / command[1]).absolute()
                if candidate.is_relative_to(self.root.absolute()) and candidate.exists():
                    pending.append(candidate)

        discovered: set[Path] = set()
        while pending:
            source = pending.pop()
            if source in discovered:
                continue
            discovered.add(source)
            try:
                tree = ast.parse(source.read_text(encoding="utf-8"))
            except (OSError, SyntaxError, UnicodeError):
                continue
            module_names: set[str] = set()
            for node in ast.walk(tree):
                if isinstance(node, ast.Import):
                    module_names.update(alias.name for alias in node.names)
                elif isinstance(node, ast.ImportFrom) and node.module:
                    module_names.add(node.module)
            for module_name in module_names:
                relative = Path(*module_name.split("."))
                for base in (source.parent, self.root):
                    candidates = (base / relative).with_suffix(".py"), base / relative / "__init__.py"
                    local = next((path.absolute() for path in candidates if path.exists()), None)
                    if (
                        local is not None
                        and local.is_relative_to(self.root.absolute())
                        and local not in discovered
                    ):
                        pending.append(local)
                        break

        return [
            self.file_fingerprint(path)
            for path in sorted(discovered)
        ]

    def key(
        self,
        item: Task,
        dependency_keys: dict[str, str],
    ) -> str:
        tools = sorted({
            "python3",
            "node",
            "npm",
            *item.cache_tools,
            *(command[0] for command in item.commands),
        })
        payload: dict[str, object] = {
            "schema_version": CACHE_SCHEMA_VERSION,
            "task": task_json(item),
            "dependencies": {
                dependency: dependency_keys[dependency]
                for dependency in item.dependencies
            },
            "inputs": self.pattern_fingerprints(item.inputs),
            "generated_inputs": self.pattern_fingerprints(item.generated_inputs),
            "cache_inputs": self.pattern_fingerprints(item.cache_inputs),
            "python_command_dependencies": self.python_command_dependencies(item),
            "orchestrator": [
                self.file_fingerprint(self.root / "scripts" / "check_tasks.py"),
                self.file_fingerprint(self.root / "scripts" / "check_tasks_data.py"),
            ],
            "lockfiles": [
                self.file_fingerprint(self.root / lockfile)
                for lockfile in LOCKFILES
                if (self.root / lockfile).exists()
            ],
            "platform": {
                "system": platform.system(),
                "release": platform.release(),
                "machine": platform.machine(),
            },
            "tools": [self.tool_version(executable) for executable in tools],
            "environment": {
                name: os.environ.get(name)
                for name in item.cache_env
            },
        }
        if item.id == "web-browser-smoke":
            payload["browser"] = self.browser_fingerprint()
        encoded = json.dumps(payload, sort_keys=True, separators=(",", ":")).encode()
        return hashlib.sha256(encoded).hexdigest()


class CacheStore:
    def __init__(self, root: Path) -> None:
        self.root = root

    def entry_path(self, task_id: str, key: str) -> Path:
        return self.root / task_id / f"{key}.json"

    def contains(self, task_id: str, key: str) -> bool:
        path = self.entry_path(task_id, key)
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError):
            return False
        return value == {
            "schema_version": CACHE_SCHEMA_VERSION,
            "task_id": task_id,
            "key": key,
            "status": "passed",
        }

    def write_pass(self, task_id: str, key: str) -> None:
        directory = self.entry_path(task_id, key).parent
        directory.mkdir(parents=True, exist_ok=True)
        value = {
            "schema_version": CACHE_SCHEMA_VERSION,
            "task_id": task_id,
            "key": key,
            "status": "passed",
        }
        with NamedTemporaryFile(
            "w",
            encoding="utf-8",
            dir=directory,
            prefix=".entry-",
            suffix=".tmp",
            delete=False,
        ) as handle:
            json.dump(value, handle, sort_keys=True)
            handle.write("\n")
            temporary = Path(handle.name)
        try:
            os.replace(temporary, self.entry_path(task_id, key))
        finally:
            temporary.unlink(missing_ok=True)


def cache_status(root: Path) -> dict[str, object]:
    directory = cache_directory(root)
    entries = 0
    corrupt = 0
    size_bytes = 0
    tasks: set[str] = set()
    if directory.exists():
        for path in directory.rglob("*.json"):
            entries += 1
            tasks.add(path.parent.name)
            try:
                size_bytes += path.stat().st_size
                value = json.loads(path.read_text(encoding="utf-8"))
                if (
                    value.get("schema_version") != CACHE_SCHEMA_VERSION
                    or value.get("status") != "passed"
                    or value.get("task_id") != path.parent.name
                    or value.get("key") != path.stem
                ):
                    corrupt += 1
            except (OSError, json.JSONDecodeError, AttributeError):
                corrupt += 1
    return {
        "path": str(directory),
        "entries": entries,
        "tasks": len(tasks),
        "corrupt_entries": corrupt,
        "size_bytes": size_bytes,
    }


def clear_cache(root: Path) -> Path:
    directory = cache_directory(root)
    if directory.exists():
        shutil.rmtree(directory)
    return directory


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


class ProcessController:
    """Track active child process groups so the main thread can interrupt all of them."""

    def __init__(self) -> None:
        self.interrupted = False
        self._lock = threading.Lock()
        self._processes: set[subprocess.Popen[str]] = set()

    def add(self, process: subprocess.Popen[str]) -> None:
        with self._lock:
            self._processes.add(process)

    def remove(self, process: subprocess.Popen[str]) -> None:
        with self._lock:
            self._processes.discard(process)

    def forward(self, signum: int, _frame: object) -> None:
        self.interrupted = True
        with self._lock:
            processes = tuple(self._processes)
        for process in processes:
            if process.poll() is not None:
                continue
            try:
                os.killpg(process.pid, signum)
            except (ProcessLookupError, PermissionError):
                try:
                    process.send_signal(signum)
                except ProcessLookupError:
                    pass


@contextmanager
def forward_interrupts(controller: ProcessController):
    previous_handlers: dict[int, object] = {}
    if threading.current_thread() is threading.main_thread():
        for signum in (signal.SIGINT, signal.SIGTERM):
            previous_handlers[signum] = signal.getsignal(signum)
            signal.signal(signum, controller.forward)
    try:
        yield
    finally:
        for signum, handler in previous_handlers.items():
            signal.signal(signum, handler)


def run_process(
    command: tuple[str, ...],
    root: Path,
    controller: ProcessController,
    *,
    capture: bool,
) -> tuple[int, str]:
    try:
        process = subprocess.Popen(
            command,
            cwd=root,
            start_new_session=True,
            stdout=subprocess.PIPE if capture else None,
            stderr=subprocess.STDOUT if capture else None,
            text=True,
        )
    except FileNotFoundError:
        message = f"error: executable not found: {command[0]}\n"
        if not capture:
            print(message, end="", file=sys.stderr)
        return 1, message if capture else ""
    except OSError as error:
        message = f"error: could not run {command[0]}: {error}\n"
        if not capture:
            print(message, end="", file=sys.stderr)
        return 1, message if capture else ""

    controller.add(process)
    try:
        output, _ = process.communicate()
    finally:
        controller.remove(process)
    return_code = process.returncode
    code = 130 if controller.interrupted or return_code < 0 else return_code
    return code, output or ""


def run_command(command: tuple[str, ...], root: Path) -> int:
    controller = ProcessController()
    with forward_interrupts(controller):
        code, _ = run_process(command, root, controller, capture=False)
    return code


def execute_task(
    item: Task,
    root: Path,
    controller: ProcessController,
    *,
    capture: bool,
) -> tuple[dict[str, object], str]:
    output: list[str] = []

    def emit(line: str) -> None:
        if capture:
            output.append(line + "\n")
        else:
            print(line, flush=True)

    emit(f"\n==> {item.id}")
    task_started = time.monotonic()
    task_exit = 0
    for command in item.commands:
        emit(f"$ {' '.join(command)}")
        task_exit, command_output = run_process(
            command,
            root,
            controller,
            capture=capture,
        )
        if capture and command_output:
            output.append(command_output)
            if not command_output.endswith("\n"):
                output.append("\n")
        if task_exit != 0:
            break
    duration = round(time.monotonic() - task_started, 3)
    result = {
        "id": item.id,
        "status": "passed" if task_exit == 0 else "failed",
        "duration_seconds": duration,
        "exit_code": task_exit,
        "commands": [list(command) for command in item.commands],
    }
    return result, "".join(output)


def skipped_result(item: Task, status: str) -> dict[str, object]:
    return {
        "id": item.id,
        "status": status,
        "duration_seconds": 0.0,
        "exit_code": None,
        "commands": [list(command) for command in item.commands],
        "cache": {"state": "not_run"},
    }


def cached_result(item: Task, key: str) -> dict[str, object]:
    return {
        "id": item.id,
        "status": "cached",
        "duration_seconds": 0.0,
        "exit_code": 0,
        "commands": [list(command) for command in item.commands],
        "cache": {"state": "hit", "key": key},
    }


def execute_tasks(
    selected: list[Task],
    root: Path,
    *,
    profile: str | None,
    domain: str | None,
    requested_task: str | None,
    jobs: int = 1,
    no_cache: bool = False,
) -> tuple[int, dict[str, object]]:
    if jobs < 1:
        raise ConfigurationError("--jobs must be at least 1")
    started_at = utc_now()
    run_started = time.monotonic()
    requested_jobs = jobs
    jobs = 1 if profile == "release" else jobs
    result_by_id: dict[str, dict[str, object]] = {}
    exit_code = 0
    registry = validate_registry(tuple(selected))
    controller = ProcessController()
    ci_disabled = os.environ.get("CI", "").lower() == "true"
    cache_enabled = not no_cache and not ci_disabled
    cache_reason = (
        "disabled by --no-cache"
        if no_cache
        else "disabled because CI=true"
        if ci_disabled
        else "enabled for deterministic read-only tasks"
    )
    cache_builder: CacheKeyBuilder | None = None
    cache_store: CacheStore | None = None
    cache_keys: dict[str, str] = {}
    cache_eligible: set[str] = set()
    cache_checked: dict[str, bool] = {}
    cache_hits = 0
    cache_misses = 0
    cache_writes = 0
    if cache_enabled and any(item.cache == "deterministic" for item in selected):
        cache_builder = CacheKeyBuilder(root)
        cache_store = CacheStore(cache_directory(root))
        for item in selected:
            cache_keys[item.id] = cache_builder.key(item, cache_keys)
            if cache_builder.cacheable(item):
                cache_eligible.add(item.id)

    def check_cache(item: Task) -> dict[str, object] | None:
        nonlocal cache_hits, cache_misses
        if item.id not in cache_eligible or cache_store is None:
            return None
        if item.id not in cache_checked:
            cache_checked[item.id] = cache_store.contains(item.id, cache_keys[item.id])
            if cache_checked[item.id]:
                cache_hits += 1
            else:
                cache_misses += 1
        if cache_checked[item.id]:
            return cached_result(item, cache_keys[item.id])
        return None

    def record_execution(item: Task, result: dict[str, object]) -> None:
        nonlocal cache_writes
        if item.id in cache_eligible and cache_store is not None:
            result["cache"] = {"state": "miss", "key": cache_keys[item.id]}
            if result["status"] == "passed":
                cache_store.write_pass(item.id, cache_keys[item.id])
                cache_writes += 1
        else:
            result["cache"] = {"state": "disabled"}

    with forward_interrupts(controller):
        if jobs == 1:
            failed_id: str | None = None
            for item in selected:
                if controller.interrupted and failed_id is None:
                    result_by_id[item.id] = skipped_result(item, "not_run")
                    exit_code = 130
                    continue
                if failed_id is not None:
                    status = "blocked" if depends_on(item.id, failed_id, registry) else "not_run"
                    result_by_id[item.id] = skipped_result(item, status)
                    continue
                hit = check_cache(item)
                if hit is not None:
                    print(f"\n==> {item.id}\ncache hit", flush=True)
                    result_by_id[item.id] = hit
                    continue
                result, _ = execute_task(item, root, controller, capture=False)
                record_execution(item, result)
                result_by_id[item.id] = result
                task_exit = int(result["exit_code"])
                if task_exit != 0:
                    failed_id = item.id
                    exit_code = 130 if task_exit == 130 else 1
        else:
            pending = list(selected)
            failed_ids: set[str] = set()
            captured_outputs: dict[str, str] = {}
            replayed: set[str] = set()

            def replay_completed() -> None:
                for planned in selected:
                    if planned.id in captured_outputs and planned.id not in replayed:
                        print(captured_outputs[planned.id], end="", flush=True)
                        replayed.add(planned.id)

            with ThreadPoolExecutor(max_workers=jobs) as executor:
                active: dict[Future[tuple[dict[str, object], str]], Task] = {}
                while (pending or active) and not failed_ids and not controller.interrupted:
                    passed_ids = {
                        task_id
                        for task_id, result in result_by_id.items()
                        if result["status"] in CACHE_SUCCESS_STATUSES
                    }
                    ready = [
                        item
                        for item in pending
                        if set(item.dependencies) <= passed_ids
                    ]
                    cached_ready = [
                        (item, check_cache(item))
                        for item in ready
                    ]
                    hits = [(item, result) for item, result in cached_ready if result is not None]
                    if hits:
                        for item, result in hits:
                            print(f"\n==> {item.id}\ncache hit", flush=True)
                            result_by_id[item.id] = result
                            pending.remove(item)
                        continue
                    ready_parallel = [
                        item
                        for item in ready
                        if item.parallel_safe
                    ]
                    while ready_parallel and len(active) < jobs:
                        item = ready_parallel.pop(0)
                        pending.remove(item)
                        future = executor.submit(
                            execute_task,
                            item,
                            root,
                            controller,
                            capture=True,
                        )
                        active[future] = item

                    if active:
                        done, _ = wait(active, return_when=FIRST_COMPLETED)
                        for future in sorted(done, key=lambda value: selected.index(active[value])):
                            item = active.pop(future)
                            result, task_output = future.result()
                            record_execution(item, result)
                            result_by_id[item.id] = result
                            captured_outputs[item.id] = task_output
                            if result["status"] == "failed":
                                failed_ids.add(item.id)
                        continue

                    ready_serial = [
                        item
                        for item in ready
                        if not item.parallel_safe
                    ]
                    if ready_serial:
                        replay_completed()
                        item = ready_serial[0]
                        pending.remove(item)
                        result, _ = execute_task(item, root, controller, capture=False)
                        record_execution(item, result)
                        result_by_id[item.id] = result
                        if result["status"] == "failed":
                            failed_ids.add(item.id)
                        continue
                    if pending:
                        raise ConfigurationError("parallel scheduler could not find a runnable task")

                if (failed_ids or controller.interrupted) and active:
                    done, _ = wait(active)
                    for future in sorted(done, key=lambda value: selected.index(active[value])):
                        item = active[future]
                        result, task_output = future.result()
                        record_execution(item, result)
                        result_by_id[item.id] = result
                        captured_outputs[item.id] = task_output
                        if result["status"] == "failed":
                            failed_ids.add(item.id)
                replay_completed()

            if failed_ids or controller.interrupted:
                exit_code = 130 if controller.interrupted or any(
                    result_by_id[task_id]["exit_code"] == 130 for task_id in failed_ids
                ) else 1
                for item in pending:
                    status = "blocked" if any(
                        depends_on(item.id, failed_id, registry) for failed_id in failed_ids
                    ) else "not_run"
                    result_by_id[item.id] = skipped_result(item, status)

    results = [result_by_id[item.id] for item in selected]

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
        "jobs": {"requested": requested_jobs, "used": jobs},
        "cache": {
            "enabled": cache_enabled,
            "reason": cache_reason,
            "path": str(cache_store.root) if cache_store is not None else None,
            "hits": cache_hits,
            "misses": cache_misses,
            "writes": cache_writes,
        },
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
        f"Workers: **{summary['jobs']['used']}**",
        (
            f"Cache: **{summary['cache']['hits']} hits**, "
            f"{summary['cache']['misses']} misses"
            if summary["cache"]["enabled"]
            else f"Cache: **disabled** ({summary['cache']['reason']})"
        ),
        "",
        "| Task | Status | Duration |",
        "| --- | --- | ---: |",
    ]
    for result in summary["tasks"]:
        lines.append(f"| `{result['id']}` | {result['status']} | {result['duration_seconds']:.3f}s |")
    return "\n".join(lines) + "\n"


def print_summary(summary: dict[str, object], output: TextIO = sys.stdout) -> None:
    print("\nCheck task summary", file=output)
    print(
        f"Workers: {summary['jobs']['used']}"
        + (
            f" (requested {summary['jobs']['requested']})"
            if summary["jobs"]["requested"] != summary["jobs"]["used"]
            else ""
        ),
        file=output,
    )
    if summary["cache"]["enabled"]:
        print(
            f"Cache: {summary['cache']['hits']} hit(s), "
            f"{summary['cache']['misses']} miss(es), "
            f"{summary['cache']['writes']} write(s)",
            file=output,
        )
    else:
        print(f"Cache: disabled ({summary['cache']['reason']})", file=output)
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
    if package_scripts.get("check:parallel") != "python3 scripts/check_tasks.py run fast --jobs 4":
        raise AssertionError("check:parallel does not use the fast graph with four workers")

    public_aliases = {
        name: command for name, command in package_scripts.items()
        if name.startswith("check:") and not name.endswith(":legacy")
    }
    for alias, command in public_aliases.items():
        if alias in profile_aliases or alias == "check:parallel":
            continue
        task_id = alias.removeprefix("check:")
        if task_id not in registry:
            raise AssertionError(f"{alias} has no matching registered task")
        expected_command = f"python3 scripts/check_tasks.py run-task {task_id}"
        if command != expected_command:
            raise AssertionError(f"{alias} does not route through run-task {task_id}")
    missing_legacy = sorted(
        alias
        for alias in public_aliases
        if alias != "check:parallel" and f"{alias}:legacy" not in package_scripts
    )
    if missing_legacy:
        raise AssertionError(f"public check aliases are missing temporary legacy commands: {missing_legacy}")

    never_parallel = {
        "local-artifacts",
        "local-esphome",
        "pr-process",
        "pr-testing-guidance",
        "firmware-release",
        "release-confidence",
        "release-changelog",
        "web-browser-smoke",
        "docs-build",
    }
    unsafe = sorted(task_id for task_id in never_parallel if registry[task_id].parallel_safe)
    if unsafe:
        raise AssertionError(f"unsafe tasks are marked parallel-safe: {unsafe}")

    never_cached = {
        "generated",
        "local-artifacts",
        "local-esphome",
        "pr-process",
        "pr-testing-guidance",
        "firmware-release",
        "release-confidence",
        "release-changelog",
        "docs-build",
        "timezones",
    }
    cacheable_unsafe = sorted(task_id for task_id in never_cached if registry[task_id].cache != "never")
    if cacheable_unsafe:
        raise AssertionError(f"unsafe tasks are cacheable: {cacheable_unsafe}")
    browser = registry["web-browser-smoke"]
    if (
        browser.cache != "deterministic"
        or "src/webserver/**" not in browser.inputs
        or "docs/public/webserver/**" not in browser.generated_inputs
        or "PLAYWRIGHT_BROWSERS_PATH" not in browser.cache_env
    ):
        raise AssertionError("browser cache policy omits required web, layout, or environment inputs")
    for task_id in ("config", "backup-contract", "web-smoke"):
        if not {"scripts/web_source.js", "scripts/web_modules.json"} <= set(registry[task_id].inputs):
            raise AssertionError(f"{task_id} cache keys omit shared web-source helpers")
    for task_id in ("config", "model-contract"):
        if "compatibility/fixtures/product_compatibility.json" not in registry[task_id].inputs:
            raise AssertionError(f"{task_id} cache keys omit compatibility fixtures")
    if not {
        "common/**",
        "components/**",
        "compatibility/**",
        "devices/**",
        "package.json",
        ".github/workflows/**",
        "docs/**",
        "product/**",
        "scripts/**",
        "src/**",
    } <= set(registry["dev-docs"].inputs + registry["dev-docs"].cache_inputs):
        raise AssertionError("dev-docs cache keys omit runtime validation inputs")
    if not {"c++", "g++", "clang++"} <= set(registry["firmware-parser"].cache_tools):
        raise AssertionError("firmware parser cache keys omit compiler tool versions")
    if "components/artwork_image/artwork_image.cpp" not in registry["cover-art-contract"].inputs:
        raise AssertionError("cover-art cache keys omit the downloader source")
    if "common/config/*_card_normalization_fixtures.json" not in registry["firmware-parser"].inputs:
        raise AssertionError("firmware parser cache keys omit normalization fixtures")
    if "components/espcontrol/sun_calc.h" not in registry["timezones"].inputs:
        raise AssertionError("timezone cache keys omit the firmware timezone table")
    if registry["timezones"].cache != "never":
        raise AssertionError("timezone validation is cached without a stable host tzdata fingerprint")
    if not {"common/**", "scripts/generate_device_slots.py"} <= set(registry["firmware-modals"].inputs):
        raise AssertionError("firmware modal cache keys omit common or generator inputs")
    if "common/**" not in registry["firmware-ha-bindings"].inputs:
        raise AssertionError("firmware binding cache keys omit common configuration")
    if not {"common/**", "components/**", "src/webserver/**", "compatibility/**"} <= set(registry["device-profiles"].inputs):
        raise AssertionError("device profile cache keys omit cross-layer inputs")
    for task_id in ("device-manifest", "device-manifest-output", "device-matrix", "device-slots"):
        if "common/assets/**" not in registry[task_id].inputs:
            raise AssertionError(f"{task_id} cache keys omit common font assets")

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
    expect_invalid((Task("cache", (("true",),), cache="sometimes"),), "invalid cache policy")
    expect_invalid(
        (Task("parallel", (("true",),), parallel_safe="yes"),),  # type: ignore[arg-type]
        "invalid parallel safety",
    )
    expect_invalid(
        (Task("cache-env", (("true",),), cache_env=("VALUE", "VALUE")),),
        "duplicate cache environment declarations",
    )
    expect_invalid(
        (Task("cache-tools", (("true",),), cache_tools=("tool", "tool")),),
        "duplicate cache tool declarations",
    )
    expect_invalid(
        (Task("cache-inputs", (("true",),), cache_inputs=("path", "path")),),
        "duplicate cache input declarations",
    )

    first = Task("first", (("true",),), dependencies=("second",), inputs=("x",))
    second = Task("second", (("true",),), dependencies=("first",), inputs=("x",))
    expect_invalid((first, second), "dependency cycle")

    try:
        execute_tasks([], ROOT, profile="fast", domain=None, requested_task=None, jobs=0)
    except ConfigurationError:
        pass
    else:
        raise AssertionError("zero parallel workers were accepted")

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

        marker_a = root / "parallel-a"
        marker_b = root / "parallel-b"
        sync_command = (
            "from pathlib import Path; import sys, time; "
            "own, other = Path(sys.argv[1]), Path(sys.argv[2]); "
            "own.write_text('started'); print(sys.argv[3], flush=True); "
            "deadline = time.monotonic() + 2; "
            "exec(\"while not other.exists() and time.monotonic() < deadline:\\n time.sleep(0.01)\"); "
            "assert other.exists(); "
            "print(sys.argv[4], flush=True)"
        )
        parallel_tasks = [
            Task(
                "parallel-a",
                ((sys.executable, "-c", sync_command, str(marker_a), str(marker_b), "A-BEGIN", "A-END"),),
                parallel_safe=True,
            ),
            Task(
                "parallel-b",
                ((sys.executable, "-c", sync_command, str(marker_b), str(marker_a), "B-BEGIN", "B-END"),),
                parallel_safe=True,
            ),
        ]
        parallel_output = StringIO()
        with redirect_stdout(parallel_output):
            parallel_code, parallel_summary = execute_tasks(
                parallel_tasks,
                root,
                profile="fast",
                domain=None,
                requested_task=None,
                jobs=2,
            )
        captured = parallel_output.getvalue()
        if parallel_code != 0 or parallel_summary["jobs"] != {"requested": 2, "used": 2}:
            raise AssertionError("parallel-safe independent tasks did not run with two workers")
        if not (
            captured.index("A-BEGIN")
            < captured.index("A-END")
            < captured.index("B-BEGIN")
            < captured.index("B-END")
        ):
            raise AssertionError("parallel task output was not replayed in stable grouped blocks")

        exclusive_flag = root / "parallel-safe-running"
        exclusive_tasks = [
            Task(
                "parallel-safe-long",
                ((
                    sys.executable,
                    "-c",
                    "import time; from pathlib import Path; "
                    f"p=Path({str(exclusive_flag)!r}); "
                    "p.write_text('running'); time.sleep(0.15); p.unlink()",
                ),),
                parallel_safe=True,
            ),
            Task(
                "parallel-unsafe",
                ((
                    sys.executable,
                    "-c",
                    "import sys; from pathlib import Path; "
                    f"sys.exit(1 if Path({str(exclusive_flag)!r}).exists() else 0)",
                ),),
            ),
        ]
        with redirect_stdout(StringIO()):
            exclusive_code, _ = execute_tasks(
                exclusive_tasks,
                root,
                profile="fast",
                domain=None,
                requested_task=None,
                jobs=2,
            )
        if exclusive_code != 0:
            raise AssertionError("a task without parallel safety ran beside an active task")

        active_marker = root / "active-finished"
        failure_tasks = [
            Task(
                "parallel-fail",
                ((sys.executable, "-c", "print('failure output'); raise SystemExit(7)"),),
                parallel_safe=True,
            ),
            Task(
                "parallel-active",
                ((
                    sys.executable,
                    "-c",
                    "import time; from pathlib import Path; time.sleep(0.1); "
                    f"Path({str(active_marker)!r}).write_text('done')",
                ),),
                parallel_safe=True,
            ),
            Task(
                "parallel-blocked",
                ((sys.executable, "-c", "raise SystemExit(0)"),),
                dependencies=("parallel-fail",),
                parallel_safe=True,
            ),
            Task(
                "parallel-not-run",
                ((sys.executable, "-c", "raise SystemExit(0)"),),
                parallel_safe=True,
            ),
        ]
        with redirect_stdout(StringIO()):
            failure_code, failure_summary = execute_tasks(
                failure_tasks,
                root,
                profile="fast",
                domain=None,
                requested_task=None,
                jobs=2,
            )
        failure_statuses = {item["id"]: item["status"] for item in failure_summary["tasks"]}
        if failure_code != 1 or failure_statuses != {
            "parallel-fail": "failed",
            "parallel-active": "passed",
            "parallel-blocked": "blocked",
            "parallel-not-run": "not_run",
        }:
            raise AssertionError(f"parallel fail-stop execution is incorrect: {failure_statuses}")
        if active_marker.read_text() != "done":
            raise AssertionError("an active parallel task was not allowed to finish after failure")

        release_flag = root / "release-running"
        release_tasks = [
            Task(
                "release-first",
                ((
                    sys.executable,
                    "-c",
                    "import time; from pathlib import Path; "
                    f"p=Path({str(release_flag)!r}); "
                    "p.write_text('running'); time.sleep(0.15); p.unlink()",
                ),),
                parallel_safe=True,
            ),
            Task(
                "release-second",
                ((
                    sys.executable,
                    "-c",
                    "import sys, time; from pathlib import Path; time.sleep(0.03); "
                    f"sys.exit(1 if Path({str(release_flag)!r}).exists() else 0)",
                ),),
                parallel_safe=True,
            ),
        ]
        with redirect_stdout(StringIO()):
            release_code, release_summary = execute_tasks(
                release_tasks,
                root,
                profile="release",
                domain=None,
                requested_task=None,
                jobs=2,
            )
        if release_code != 0 or release_summary["jobs"] != {"requested": 2, "used": 1}:
            raise AssertionError("release execution was not forced to one worker")

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
            "child = \"import os, signal, time; "
            "signal.signal(signal.SIGINT, lambda *_: os._exit(130)); time.sleep(30)\"; "
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

        parallel_interrupt_test = (
            "import os, signal, sys, threading; "
            "from pathlib import Path; "
            "from check_tasks import Task, execute_tasks; "
            "child = \"import os, signal, time; "
            "signal.signal(signal.SIGINT, lambda *_: os._exit(130)); time.sleep(30)\"; "
            "tasks = [Task(f'sleep-{index}', ((sys.executable, '-c', child),), "
            "parallel_safe=True) for index in range(2)]; "
            "threading.Timer(0.2, lambda: os.kill(os.getpid(), signal.SIGINT)).start(); "
            "code, summary = execute_tasks(tasks, Path.cwd(), profile='fast', "
            "domain=None, requested_task=None, jobs=2); "
            "raise SystemExit(0 if code == 130 and summary['status'] == 'interrupted' else 1)"
        )
        parallel_interrupted = subprocess.run(
            [sys.executable, "-c", parallel_interrupt_test],
            cwd=ROOT,
            env={**os.environ, "PYTHONPATH": str(ROOT / "scripts")},
            capture_output=True,
            timeout=5,
        )
        if parallel_interrupted.returncode != 0:
            raise AssertionError("interrupts are not forwarded to all active parallel tasks")

    previous_ci = os.environ.pop("CI", None)
    try:
        with TemporaryDirectory() as tmp:
            base = Path(tmp)
            repo = base / "repo"
            linked = base / "linked"
            repo.mkdir()

            def cache_git(root: Path, *args: str) -> None:
                subprocess.run(["git", *args], cwd=root, check=True, capture_output=True)

            cache_git(repo, "init", "-b", "main")
            cache_git(repo, "config", "user.name", "Check Cache Test")
            cache_git(repo, "config", "user.email", "check-cache@example.invalid")
            (repo / "scripts").mkdir()
            (repo / "scripts" / "check_tasks.py").write_text("runner-v1\n")
            (repo / "scripts" / "check_tasks_data.py").write_text("registry-v1\n")
            (repo / "scripts" / "cache_entry.py").write_text("import cache_helper\n")
            (repo / "scripts" / "cache_helper.py").write_text("VALUE = 'helper-v1'\n")
            (repo / "input.txt").write_text("authored-v1\n")
            (repo / "cache-only.txt").write_text("cache-only-v1\n")
            (repo / "generated.txt").write_text("generated-v1\n")
            (repo / "package-lock.json").write_text("lock-v1\n")
            (repo / "nested" / "deep").mkdir(parents=True)
            (repo / "nested" / "deep" / "value.txt").write_text("nested-v1\n")
            cache_git(repo, "add", ".")
            cache_git(repo, "commit", "-m", "cache fixture")
            cache_git(repo, "worktree", "add", "-b", "linked", str(linked))

            execution_marker = base / "executed.txt"
            cache_task = Task(
                "cacheable",
                ((
                    sys.executable,
                    "-c",
                    f"from pathlib import Path; Path({str(execution_marker)!r}).write_text('ran')",
                ),),
                inputs=("input.txt",),
                generated_inputs=("generated.txt",),
                cache="deterministic",
                cache_env=("CHECK_TASK_TEST_ENV",),
                cache_inputs=("cache-only.txt",),
            )

            cache_only_key_v1 = CacheKeyBuilder(linked).key(cache_task, {})
            (linked / "cache-only.txt").write_text("cache-only-v2\n")
            cache_only_key_v2 = CacheKeyBuilder(linked).key(cache_task, {})
            if cache_only_key_v1 == cache_only_key_v2:
                raise AssertionError("a cache-only input did not invalidate the cache key")
            (linked / "cache-only.txt").write_text("cache-only-v1\n")

            helper_task = Task(
                "helper-import",
                (("python3", "scripts/cache_entry.py"),),
                inputs=("input.txt",),
                cache="deterministic",
            )
            helper_key_v1 = CacheKeyBuilder(linked).key(helper_task, {})
            (linked / "scripts" / "cache_helper.py").write_text("VALUE = 'helper-v2'\n")
            helper_key_v2 = CacheKeyBuilder(linked).key(helper_task, {})
            if helper_key_v1 == helper_key_v2:
                raise AssertionError("a repo-local imported Python helper did not invalidate the cache")

            os.environ["CHECK_TASK_TEST_ENV"] = "one"
            with redirect_stdout(StringIO()):
                first_code, first_cache_summary = execute_tasks(
                    [cache_task],
                    repo,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if (
                first_code != 0
                or first_cache_summary["cache"]["misses"] != 1
                or first_cache_summary["cache"]["writes"] != 1
                or not execution_marker.exists()
            ):
                raise AssertionError("a successful deterministic task was not cached")

            execution_marker.unlink()
            with redirect_stdout(StringIO()):
                shared_code, shared_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if (
                shared_code != 0
                or shared_summary["tasks"][0]["status"] != "cached"
                or shared_summary["cache"]["hits"] != 1
                or execution_marker.exists()
                or cache_directory(repo) != cache_directory(linked)
            ):
                raise AssertionError("cache entries are not shared safely across worktrees")

            (linked / "input.txt").write_text("authored-v2\n")
            with redirect_stdout(StringIO()):
                input_code, input_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if input_code != 0 or input_summary["tasks"][0]["status"] != "passed":
                raise AssertionError("an authored input change did not invalidate the cache")

            execution_marker.unlink()
            (linked / "generated.txt").write_text("generated-v2\n")
            with redirect_stdout(StringIO()):
                generated_code, generated_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if generated_code != 0 or generated_summary["tasks"][0]["status"] != "passed":
                raise AssertionError("a generated output change did not invalidate the cache")

            changed_command_task = Task(
                "cacheable",
                ((
                    sys.executable,
                    "-c",
                    f"from pathlib import Path; Path({str(execution_marker)!r}).write_text('new-command')",
                ),),
                inputs=("input.txt",),
                generated_inputs=("generated.txt",),
                cache="deterministic",
                cache_env=("CHECK_TASK_TEST_ENV",),
            )
            execution_marker.unlink()
            with redirect_stdout(StringIO()):
                command_code, command_summary = execute_tasks(
                    [changed_command_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if command_code != 0 or command_summary["tasks"][0]["status"] != "passed":
                raise AssertionError("a task command change did not invalidate the cache")

            failing_task = Task(
                "cache-failure",
                ((sys.executable, "-c", "raise SystemExit(7)"),),
                inputs=("input.txt",),
                cache="deterministic",
            )
            for _ in range(2):
                with redirect_stdout(StringIO()):
                    failure_code, failure_cache_summary = execute_tasks(
                        [failing_task],
                        linked,
                        profile="fast",
                        domain=None,
                        requested_task=None,
                    )
                if (
                    failure_code != 1
                    or failure_cache_summary["tasks"][0]["status"] != "failed"
                    or failure_cache_summary["cache"]["writes"] != 0
                ):
                    raise AssertionError("failed task status was cached")

            builder = CacheKeyBuilder(linked)
            corrupt_key = builder.key(cache_task, {})
            corrupt_store = CacheStore(cache_directory(linked))
            corrupt_path = corrupt_store.entry_path(cache_task.id, corrupt_key)
            corrupt_path.parent.mkdir(parents=True, exist_ok=True)
            corrupt_path.write_text("not json\n")
            execution_marker.unlink(missing_ok=True)
            with redirect_stdout(StringIO()):
                corrupt_code, corrupt_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            if (
                corrupt_code != 0
                or corrupt_summary["tasks"][0]["status"] != "passed"
                or not corrupt_store.contains(cache_task.id, corrupt_key)
            ):
                raise AssertionError("a corrupt cache entry was not treated as a miss")

            execution_marker.unlink()
            os.environ["CI"] = "true"
            with redirect_stdout(StringIO()):
                ci_code, ci_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                )
            os.environ.pop("CI")
            if (
                ci_code != 0
                or ci_summary["cache"]["enabled"]
                or ci_summary["tasks"][0]["status"] != "passed"
                or not execution_marker.exists()
            ):
                raise AssertionError("CI used an existing local cache entry")

            execution_marker.unlink()
            with redirect_stdout(StringIO()):
                no_cache_code, no_cache_summary = execute_tasks(
                    [cache_task],
                    linked,
                    profile="fast",
                    domain=None,
                    requested_task=None,
                    no_cache=True,
                )
            if (
                no_cache_code != 0
                or no_cache_summary["cache"]["enabled"]
                or not execution_marker.exists()
            ):
                raise AssertionError("--no-cache did not force fresh execution")

            dependency_v1 = Task(
                "cache-dependency",
                ((sys.executable, "-c", "raise SystemExit(0)"),),
                inputs=("input.txt",),
                cache="deterministic",
            )
            dependency_v2 = Task(
                "cache-dependency",
                ((sys.executable, "-c", "print('changed dependency')"),),
                inputs=("input.txt",),
                cache="deterministic",
            )
            consumer = Task(
                "cache-consumer",
                ((sys.executable, "-c", "raise SystemExit(0)"),),
                dependencies=("cache-dependency",),
                inputs=("input.txt",),
                cache="deterministic",
            )
            first_builder = CacheKeyBuilder(linked)
            dependency_key_v1 = first_builder.key(dependency_v1, {})
            consumer_key_v1 = first_builder.key(
                consumer,
                {dependency_v1.id: dependency_key_v1},
            )
            second_builder = CacheKeyBuilder(linked)
            dependency_key_v2 = second_builder.key(dependency_v2, {})
            consumer_key_v2 = second_builder.key(
                consumer,
                {dependency_v2.id: dependency_key_v2},
            )
            if dependency_key_v1 == dependency_key_v2 or consumer_key_v1 == consumer_key_v2:
                raise AssertionError("dependency changes do not invalidate consumer cache keys")

            baseline_builder = CacheKeyBuilder(linked)
            baseline_key = baseline_builder.key(cache_task, {})
            (linked / "scripts" / "check_tasks_data.py").write_text("registry-v2\n")
            registry_key = CacheKeyBuilder(linked).key(cache_task, {})
            (linked / "scripts" / "check_tasks.py").write_text("runner-v2\n")
            runner_key = CacheKeyBuilder(linked).key(cache_task, {})
            (linked / "package-lock.json").write_text("lock-v2\n")
            lock_key = CacheKeyBuilder(linked).key(cache_task, {})
            os.environ["CHECK_TASK_TEST_ENV"] = "two"
            environment_key = CacheKeyBuilder(linked).key(cache_task, {})
            if len({baseline_key, registry_key, runner_key, lock_key, environment_key}) != 5:
                raise AssertionError(
                    "runner, registry, lockfile, or environment changes do not invalidate cache keys"
                )

            broad_task = Task(
                "broad-input",
                ((sys.executable, "-c", "raise SystemExit(0)"),),
                inputs=("nested/**",),
                cache="deterministic",
            )
            broad_key_v1 = CacheKeyBuilder(linked).key(broad_task, {})
            (linked / "nested" / "deep" / "value.txt").write_text("nested-v2\n")
            broad_key_v2 = CacheKeyBuilder(linked).key(broad_task, {})
            if broad_key_v1 == broad_key_v2:
                raise AssertionError("nested files under a broad input glob do not invalidate cache keys")

            tool_builder_one = CacheKeyBuilder(linked)
            tool_builder_one._tool_versions["python3"] = {"version": "tool-one"}
            tool_key_one = tool_builder_one.key(cache_task, {})
            tool_builder_two = CacheKeyBuilder(linked)
            tool_builder_two._tool_versions["python3"] = {"version": "tool-two"}
            tool_key_two = tool_builder_two.key(cache_task, {})
            if tool_key_one == tool_key_two:
                raise AssertionError("tool version changes do not invalidate cache keys")

            status = cache_status(linked)
            if status["entries"] == 0 or status["corrupt_entries"] != 0:
                raise AssertionError("cache status does not report valid shared entries")
            cleared = clear_cache(linked)
            if cleared.exists():
                raise AssertionError("cache clear left shared entries behind")
    finally:
        os.environ.pop("CHECK_TASK_TEST_ENV", None)
        os.environ.pop("CI", None)
        if previous_ci is not None:
            os.environ["CI"] = previous_ci

    def task_ids(selected: list[Task]) -> set[str]:
        return {item.id for item in selected}

    docs_selected, _, docs_fallback = changed_plan(["dev-docs/README.md"])
    if docs_fallback is not None or not {"dev-docs", "docs-build"} <= task_ids(docs_selected):
        raise AssertionError("docs-only changes do not select documentation checks")

    for maintainer_doc in ("README.md", "DEVELOPERS.md", "product/README.md"):
        maintainer_selected, _, maintainer_fallback = changed_plan([maintainer_doc])
        if (
            maintainer_fallback is not None
            or not {"dev-docs", "docs-build"} <= task_ids(maintainer_selected)
        ):
            raise AssertionError(f"{maintainer_doc} does not select maintainer documentation checks")

    web_selected, _, web_fallback = changed_plan(["src/webserver/modules/example.js"])
    if web_fallback is not None or "web-smoke" not in task_ids(web_selected):
        raise AssertionError("web changes do not select web checks")

    firmware_selected, _, firmware_fallback = changed_plan(["components/espcontrol/example.h"])
    if firmware_fallback is not None or "firmware-parser" not in task_ids(firmware_selected):
        raise AssertionError("firmware changes do not select firmware checks")

    subpage_selected, _, subpage_fallback = changed_plan([
        "components/espcontrol/button_grid_subpages.h"
    ])
    if subpage_fallback is not None or "firmware-tests" not in task_ids(subpage_selected):
        raise AssertionError("subpage parser changes do not select compiled firmware tests")

    for saved_config_input in (
        "components/espcontrol/button_grid_config_parser.h",
        "common/config/card_normalization_fixtures.json",
        "scripts/generate_saved_config_parser_test.py",
        "src/webserver/modules/config_codec.js",
    ):
        saved_config_selected, _, _ = changed_plan([saved_config_input])
        if "firmware-tests" not in task_ids(saved_config_selected):
            raise AssertionError(
                f"saved configuration input {saved_config_input} does not select compiled firmware tests"
            )

    generated_selected, _, generated_fallback = changed_plan(["components/espcontrol/i18n_generated.h"])
    if generated_fallback is not None or "generated" not in task_ids(generated_selected):
        raise AssertionError("generated inputs do not select their validation task")

    unknown_selected, _, unknown_fallback = changed_plan(["unexpected-area/file.xyz"])
    if unknown_fallback is None or task_ids(unknown_selected) != task_ids(plan("fast")):
        raise AssertionError("unknown paths do not fall back to the full fast profile")

    workflow_selected, _, workflow_fallback = changed_plan([".github/workflows/example.yml"])
    if workflow_fallback is None or task_ids(workflow_selected) != task_ids(plan("fast")):
        raise AssertionError("workflow changes do not fall back to the full fast profile")

    for safety_script in (
        "scripts/build.py",
        "scripts/check_device_manifest.py",
        "scripts/check_web_smoke.js",
        "scripts/generate_device_manifest.py",
    ):
        script_selected, _, script_fallback = changed_plan([safety_script])
        if script_fallback is None or task_ids(script_selected) != task_ids(plan("fast")):
            raise AssertionError(
                f"generator or validator change {safety_script} does not select the full fast profile"
            )

    helper_selected, _, helper_fallback = changed_plan(["scripts/device_matrix.py"])
    if helper_fallback is None or task_ids(helper_selected) != task_ids(plan("fast")):
        raise AssertionError("shared script helpers matched only by a catch-all do not select fast")

    esphome_selected, _, esphome_fallback = changed_plan([".github/esphome.env"])
    if esphome_fallback is not None or "firmware-release" not in task_ids(esphome_selected):
        raise AssertionError("ESPHome version changes do not select firmware release checks")

    broadened_selected, _, broadened_fallback = changed_plan([
        "docs/reference/faq.md",
        "unexpected-area/file.xyz",
    ])
    broadened_ids = task_ids(broadened_selected)
    if (
        broadened_fallback is None
        or not task_ids(plan("fast")) <= broadened_ids
        or "docs-build" not in broadened_ids
    ):
        raise AssertionError("fallback discards directly matched tasks outside the fast profile")

    lock_selected, _, lock_fallback = changed_plan(["package-lock.json"])
    lock_ids = task_ids(lock_selected)
    if lock_fallback is None or not {"docs-build", "web-browser-smoke", "types"} <= lock_ids:
        raise AssertionError("package lock fallback discards declared consumers")

    clean_selected, clean_reasons, clean_fallback = changed_plan([])
    if clean_selected or clean_reasons or clean_fallback is not None:
        raise AssertionError("clean trees should not select changed-file checks")

    if not task_ids(plan("ci", "web")) < task_ids(plan("ci")):
        raise AssertionError("profile and domain combinations do not narrow direct selection")

    with TemporaryDirectory() as tmp:
        repo = Path(tmp)

        def run_git(*args: str) -> None:
            subprocess.run(["git", *args], cwd=repo, check=True, capture_output=True)

        run_git("init", "-b", "main")
        run_git("config", "user.name", "Check Graph Test")
        run_git("config", "user.email", "check-graph@example.invalid")
        initial = {
            "docs/guide.md": "initial\n",
            "docs/staged-then-reverted.md": "initial\n",
            "src/webserver/old.js": "initial\n",
            "components/espcontrol/example.h": "initial\n",
            "devices/catalog.json": "{}\n",
        }
        for relative, content in initial.items():
            destination = repo / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_text(content)
        run_git("add", ".")
        run_git("commit", "-m", "initial")
        run_git("update-ref", "refs/remotes/origin/main", "main")

        base_ref, merge_base = resolve_changed_base(repo)
        if base_ref != "origin/main" or changed_paths(repo, merge_base):
            raise AssertionError("clean repository base discovery is incorrect")
        run_git("update-ref", "-d", "refs/remotes/origin/main")
        fallback_ref, fallback_merge_base = resolve_changed_base(repo)
        if fallback_ref != "main" or fallback_merge_base != merge_base:
            raise AssertionError("local main is not used when origin/main is unavailable")
        run_git("update-ref", "refs/remotes/origin/main", "main")

        run_git("switch", "-c", "feature")
        (repo / "docs/guide.md").write_text("committed\n")
        run_git("add", "docs/guide.md")
        run_git("commit", "-m", "docs change")
        run_git("restore", "--source=main", "--staged", "--worktree", "docs/guide.md")
        run_git("mv", "src/webserver/old.js", "src/webserver/new.js")
        (repo / "components/espcontrol/example.h").unlink()
        (repo / "devices/catalog.json").write_text('{"changed": true}\n')
        run_git("add", "devices/catalog.json")
        staged_then_reverted = repo / "docs/staged-then-reverted.md"
        staged_then_reverted.write_text("staged\n")
        run_git("add", "docs/staged-then-reverted.md")
        staged_then_reverted.write_text("initial\n")
        (repo / "untracked.txt").write_text("untracked\n")

        _, merge_base = resolve_changed_base(repo)
        discovered = set(changed_paths(repo, merge_base))
        expected_paths = {
            "docs/guide.md",
            "docs/staged-then-reverted.md",
            "src/webserver/old.js",
            "src/webserver/new.js",
            "components/espcontrol/example.h",
            "devices/catalog.json",
            "untracked.txt",
        }
        if not expected_paths <= discovered:
            raise AssertionError(f"changed paths omit workspace states: {sorted(expected_paths - discovered)}")

    with TemporaryDirectory() as tmp:
        repo = Path(tmp)
        subprocess.run(["git", "init", "-b", "feature"], cwd=repo, check=True, capture_output=True)
        subprocess.run(["git", "config", "user.name", "Check Graph Test"], cwd=repo, check=True)
        subprocess.run(["git", "config", "user.email", "check-graph@example.invalid"], cwd=repo, check=True)
        (repo / "README.md").write_text("initial\n")
        subprocess.run(["git", "add", "README.md"], cwd=repo, check=True)
        subprocess.run(["git", "commit", "-m", "initial"], cwd=repo, check=True, capture_output=True)
        try:
            resolve_changed_base(repo)
        except ConfigurationError as error:
            if "origin/main or local main" not in str(error):
                raise
        else:
            raise AssertionError("missing default changed-file bases were accepted")

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
    changed_parser = subparsers.add_parser("changed", help="plan checks from branch and workspace changes")
    changed_parser.add_argument("--base")
    changed_parser.add_argument("--explain", action="store_true")
    changed_parser.add_argument("--json", action="store_true")
    cache_parser = subparsers.add_parser("cache", help="inspect or clear local check results")
    cache_subparsers = cache_parser.add_subparsers(dest="cache_command", required=True)
    cache_subparsers.add_parser("status", help="show local check cache status")
    cache_subparsers.add_parser("clear", help="remove all local check cache entries")
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
            selected = plan(args.profile, args.domain)
            exit_code, summary = execute_tasks(
                selected,
                ROOT,
                profile=args.profile,
                domain=args.domain,
                requested_task=None,
                jobs=args.jobs,
                no_cache=args.no_cache,
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
        if args.command == "changed":
            base_ref, merge_base = resolve_changed_base(ROOT, args.base)
            paths = changed_paths(ROOT, merge_base)
            selected, reasons, fallback = changed_plan(paths)
            if args.json:
                print(json.dumps({
                    "base": base_ref,
                    "merge_base": merge_base,
                    "paths": paths,
                    "fallback": fallback,
                    "tasks": [
                        {**task_json(item), "reasons": reasons.get(item.id, [])}
                        for item in selected
                    ],
                }, indent=2))
            else:
                print(f"Base: {base_ref} ({merge_base[:12]})")
                if not paths:
                    print("No changed files; no tasks selected.")
                    return 0
                print("Changed files:")
                for path in paths:
                    print(f"  {path}")
                if fallback:
                    print(f"Fallback: {fallback}")
                print("Selected tasks:")
                for index, item in enumerate(selected, 1):
                    print(f"{index:2}. {item.id}")
                    if args.explain:
                        for reason in reasons.get(item.id, []):
                            print(f"    - {reason}")
            return 0
        if args.command == "cache":
            if args.cache_command == "status":
                status = cache_status(ROOT)
                print(f"Cache: {status['path']}")
                print(f"Entries: {status['entries']} across {status['tasks']} tasks")
                print(f"Corrupt entries: {status['corrupt_entries']}")
                print(f"Size: {status['size_bytes']} bytes")
                return 0
            directory = clear_cache(ROOT)
            print(f"Cleared cache: {directory}")
            return 0
        print("choose 'list', 'plan', 'run', 'run-task', 'changed', or 'cache', or use --self-test", file=sys.stderr)
        return 2
    except ConfigurationError as error:
        print(f"check task configuration error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
