#!/usr/bin/env python3
"""Apply registered mutations in temporary copies and require their checks to fail."""

from __future__ import annotations

import json
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REGISTRY = ROOT / "tests" / "mutations" / "registry.json"


def copy_tracked_files(destination: Path) -> None:
    tracked = subprocess.run(
        ["git", "ls-files", "-z"], cwd=ROOT, check=True, capture_output=True
    ).stdout.split(b"\0")
    for raw_path in tracked:
        if not raw_path:
            continue
        relative = Path(raw_path.decode("utf-8"))
        target = destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT / relative, target)
    dependencies = ROOT / "node_modules"
    if dependencies.is_dir():
        (destination / "node_modules").symlink_to(dependencies, target_is_directory=True)


def main() -> int:
    entries = json.loads(REGISTRY.read_text(encoding="utf-8"))
    if not isinstance(entries, list):
        raise SystemExit("Mutation registry must contain a JSON list")

    seen: set[str] = set()
    for entry in entries:
        mutation_id = entry["id"]
        if mutation_id in seen:
            raise SystemExit(f"Duplicate mutation id: {mutation_id}")
        seen.add(mutation_id)
        patch = ROOT / "tests" / "mutations" / entry["patch"]
        if not patch.is_file() or not entry.get("command"):
            raise SystemExit(f"Invalid mutation entry: {mutation_id}")

        with tempfile.TemporaryDirectory(prefix=f"espcontrol-{mutation_id}-") as directory:
            commands = [("replacement", entry["command"])]
            if entry.get("legacy_command"):
                commands.append(("legacy", entry["legacy_command"]))
            baseline_copy = Path(directory) / "baseline"
            copy_tracked_files(baseline_copy)
            for label, command in commands:
                baseline = subprocess.run(command, cwd=baseline_copy, check=False)
                if baseline.returncode != 0:
                    raise SystemExit(
                        f"{mutation_id}: {label} check failed before mutation "
                        f"(exit {baseline.returncode})"
                    )

            mutated_copy = Path(directory) / "mutated"
            copy_tracked_files(mutated_copy)
            applied = subprocess.run(
                ["git", "apply", str(patch)],
                cwd=mutated_copy,
                text=True,
                capture_output=True,
                check=False,
            )
            if applied.returncode:
                raise SystemExit(f"{mutation_id}: patch did not apply\n{applied.stderr}")
            for label, command in commands:
                mutated = subprocess.run(command, cwd=mutated_copy, check=False)
                if mutated.returncode == 0:
                    raise SystemExit(f"{mutation_id}: {label} check did not catch mutation")
            print(f"{mutation_id}: caught")

    print(f"Mutation checks passed ({len(entries)} registered).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
