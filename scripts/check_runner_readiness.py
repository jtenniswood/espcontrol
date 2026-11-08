#!/usr/bin/env python3
"""Check the current trusted RUNNER contract, without Docker mutations.

This checks visible filesystem capacity and configured cleanup, not physical
host placement, post-job hook execution, Docker storage limits or isolation.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import stat
import subprocess
import tempfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def command(args):
    # Do not relay arbitrary command output/errors: jobs must not expose secrets.
    result = subprocess.run(args, capture_output=True, text=True, timeout=30, check=False)
    require(result.returncode == 0, "Required read-only command failed: " + args[0])
    return result.stdout.strip()


def is_socket(path):
    try:
        return stat.S_ISSOCK(Path(path).stat().st_mode)
    except OSError:
        return False


def filesystem(path):
    rows = json.loads(command(["findmnt", "--json", "--bytes", "--output",
                              "FSTYPE,SOURCE,SIZE,MAJ:MIN", "--target", str(path)]))["filesystems"]
    require(len(rows) == 1, "Expected one filesystem")
    fs = rows[0]
    require(fs["fstype"] == "ext4", "Workspace/temp must be disk-backed ext4")
    require(fs["source"].startswith("/dev/loop"), "Expected a dedicated loop filesystem")
    require(20_000_000_000 <= int(fs["size"]) <= 21_474_836_480,
            "Workspace/temp filesystem is outside the 20 GiB contract")
    return fs["maj:min"]


def verify(repository, runner):
    env = os.environ
    require(env.get("GITHUB_REPOSITORY") == repository, "Wrong repository")
    require(env.get("ACTUAL_RUNNER_NAME") == runner, "Wrong runner instance")
    require(env.get("ACTUAL_RUNNER_OS") == platform.system() == "Linux", "Expected Linux")
    require(env.get("ACTUAL_RUNNER_ARCH") == "X64" and platform.machine() == "x86_64", "Expected X64")
    require(set(env.get("RUNNER_LABELS", "").split(",")) ==
            {repository.split("/")[1], "runner", runner}, "Unexpected runner labels")
    require("ACCESS_TOKEN" not in env, "Registration bootstrap token must be absent from jobs")
    for key, value in [("RUNNER_CLEANUP_WORKSPACE", "true"), ("RUNNER_CLEANUP_TMP", "true"),
                       ("RUNNER_CLEANUP_DOCKER", "false")]:
        require(env.get(key) == value, "Unexpected cleanup contract: " + key)
    hook = Path(env["ACTIONS_RUNNER_HOOK_JOB_COMPLETED"])
    require(hook.is_file() and os.access(hook, os.X_OK), "Cleanup hook must be executable")
    work = Path(env["GITHUB_WORKSPACE"]).resolve(strict=True)
    temp = Path(env["RUNNER_TEMP"]).resolve(strict=True)
    require(Path(command(["git", "-C", str(work), "rev-parse", "--show-toplevel"])).resolve() == work,
            "Checkout root mismatch")
    require(command(["git", "-C", str(work), "rev-parse", "HEAD"]) == env["GITHUB_SHA"],
            "Checkout revision mismatch")
    require(filesystem(work) == filesystem(temp), "Work and temp must share one capped device")
    # Only write small uniquely named files, and remove only those same files.
    for directory in [work, temp]:
        with tempfile.NamedTemporaryFile(prefix=".runner-readiness-", dir=directory) as probe:
            probe.write(b"runner-readiness\n")
            probe.flush()
            probe.seek(0)
            require(probe.read() == b"runner-readiness\n", "Probe read-back failed")
    require(env.get("DOCKER_HOST") == "unix:///var/run/docker.sock", "Expected host-socket Docker endpoint")
    require(is_socket("/var/run/docker.sock"), "Host Docker socket is unavailable")
    require(bool(command(["docker", "--host", "unix:///var/run/docker.sock", "version",
                          "--format", "{{.Server.Version}}"])), "Docker server is unavailable")
    print("PASS: exact runner, checkout, bounded work/temp, cleanup configuration, bootstrap absence and Docker connectivity")
    print("Not an isolation, Docker-storage-cap or post-job-cleanup proof; host socket is host-root-equivalent.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", required=True)
    parser.add_argument("--runner", required=True)
    args = parser.parse_args()
    try:
        verify(args.repository, args.runner)
    except (RuntimeError, KeyError, ValueError, OSError, subprocess.TimeoutExpired):
        # Deliberately avoid arbitrary exception text or environment values.
        raise SystemExit("FAIL: runner readiness contract not satisfied; inspect the nonsecret checks") from None
