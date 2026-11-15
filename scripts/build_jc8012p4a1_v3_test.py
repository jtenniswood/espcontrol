#!/usr/bin/env python3
"""Clean-build the experimental V3 factory image with a traceable source revision."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SLUG = "guition-esp32-p4-jc8012p4a1-v3-test"


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def docker_command(root: Path, cache: Path, git_common_dir: Path, git_dir: Path,
                   git_file: Path) -> list[str]:
    """Map host paths and Git metadata into the Linux build container."""
    command = ["docker", "run", "--rm"]
    # Windows has no POSIX user IDs; Docker Desktop uses the image's default user.
    if os.name != "nt":
        command += ["--user", f"{os.getuid()}:{os.getgid()}"]
    command += ["-e", "HOME=/tmp", "-v", f"{root}:/config",
                "-v", f"{cache}:/idf-cache", "-e", "ESPHOME_ESP_IDF_PREFIX=/idf-cache"]
    if git_dir != git_common_dir:
        # Overlay the linked worktree's host-specific .git pointer with a Linux
        # path. Its commondir is relative, so mounting the common metadata is enough.
        # Avoid global GIT_DIR overrides: ESPHome also clones other repositories.
        command += ["-v", f"{git_common_dir}:/git-metadata:ro",
                    "-v", f"{git_file}:/config/.git:ro"]
    return command


def main(*, slug: str = SLUG, name: str = "espctl-v3-test",
         version_prefix: str = "dev-v3", updates_disabled: bool = False) -> None:
    if git("status", "--porcelain", "--untracked-files=normal"):
        raise SystemExit("Commit the test sources first so the binary matches its recorded commit.")
    revision = git("rev-parse", "HEAD")
    version = f"{version_prefix}-{revision[:12]}"
    config = f"/config/builds/{slug}.factory.yaml"
    pinned = (ROOT / ".github/esphome.env").read_text(encoding="utf-8").strip().removeprefix("ESPHOME_VERSION=")
    image = f"ghcr.io/esphome/esphome:{pinned}"
    output = ROOT / "build" / "v3-test" / slug / revision
    output.mkdir(parents=True, exist_ok=True)
    # Keep native ESP-IDF tools between containers and subsequent runs.
    cache = Path(os.environ.get("ESPCONTROL_IDF_CACHE", ROOT / ".esphome/idf-cache")).resolve()
    cache.mkdir(parents=True, exist_ok=True)
    git_common_dir = Path(git("rev-parse", "--path-format=absolute", "--git-common-dir"))
    git_dir = Path(git("rev-parse", "--absolute-git-dir"))
    git_file = output / "container.git"
    if git_dir != git_common_dir:
        relative_git_dir = git_dir.relative_to(git_common_dir).as_posix()
        git_file.write_text(f"gitdir: /git-metadata/{relative_git_dir}\n", encoding="utf-8")
    docker = docker_command(ROOT, cache, git_common_dir, git_dir, git_file)

    def run(args: list[str], log_name: str, *, python: bool = False) -> str:
        # Invoke the CLI directly: the image's shell entrypoint overwrites the
        # explicit ESPHOME_ESP_IDF_PREFIX with its own default cache location.
        command = docker + ["--entrypoint", "python", image]
        command += args if python else ["-m", "esphome", *args]
        with (output / log_name).open("w", encoding="utf-8") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, text=True)
        content = (output / log_name).read_text(encoding="utf-8")
        if result.returncode:
            print(content[-12000:])
            raise SystemExit(f"Failed; full log: {output / log_name}")
        print(f"Passed: {log_name}", flush=True)
        return content

    toolchain = run(["-c", "import json; from esphome.components.esp32 import ESP_IDF_FRAMEWORK_VERSION_LOOKUP as v; "
                     "r=v['recommended']; assert (r.major,r.minor)==(5,5) and r.patch>=5, r; "
                     "print(json.dumps({'esp_idf':str(r)}))"], "toolchain.json", python=True)
    options = ["-s", "firmware_version", version]
    print(f"Building {version}; logs: {output}", flush=True)
    run(options + ["config", config], "config.log")
    run(options + ["clean", config], "clean.log")
    run(options + ["compile", config], "compile.log")

    build = ROOT / "builds/.esphome/build" / name
    sdk_files = list(build.rglob("sdkconfig*"))
    sdk = next((p for p in sdk_files if p.is_file() and "CONFIG_IDF_TARGET=" in p.read_text(encoding="utf-8")), None)
    if sdk is None:
        raise SystemExit(f"Cannot find generated sdkconfig in {build}")
    sdk_text = sdk.read_text(encoding="utf-8")
    assert "CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y" not in sdk_text
    assert ("# CONFIG_ESP32P4_SELECTS_REV_LESS_V3 is not set" in sdk_text
            or "CONFIG_ESP32P4_SELECTS_REV_LESS_V3=n" in sdk_text)
    defines = (build / "src/esphome/core/defines.h").read_text(encoding="utf-8")
    assert "#define USE_WEBSERVER_OTA_DISABLED" in defines
    # Confirm the external component was fetched from this commit, including the upload guard.
    server = (build / "src/esphome/components/web_server_idf/web_server_idf.cpp").read_text(encoding="utf-8")
    assert "Browser firmware uploads are disabled" in server
    # Reject stale upstream sources: their legacy PHY default aborts on P4 v3.
    display = (build / "src/esphome/components/mipi_dsi/mipi_dsi.cpp").read_text(encoding="utf-8")
    assert display == (ROOT / "components/mipi_dsi/mipi_dsi.cpp").read_text(encoding="utf-8")
    assert ".phy_clk_src = {}," in display
    generated = (build / "src/main.cpp").read_text(encoding="utf-8")
    assert "esphome::ESPHomeOTAComponent" in generated
    assert "->set_port(3232)" in generated
    if updates_disabled:
        # Verify the merged configuration really omitted both updater packages;
        # an OFF switch alone would still allow a saved setting or API to enable it.
        for updater_id in ("auto_update_switch", "c6_auto_update_switch", "esp32_c6_update"):
            assert not re.search(rf"\b{updater_id}\b", generated), updater_id
        assert "/firmware/" not in generated
        assert "#define USE_ESP32_HOSTED" in defines
    else:
        for switch_id in ("auto_update_switch", "c6_auto_update_switch"):
            assert f"{switch_id}->set_restore_mode(switch_::SWITCH_ALWAYS_OFF);" not in generated
        assert f"/firmware/{slug}/manifest.json" in generated
        assert "/firmware/guition-esp32-p4-jc8012p4a1-v2/manifest.json" not in generated

    # Use the native IDF flash plan to identify the actual app and bootloader.
    flash_plan_path = build / "build/flasher_args.json"
    flash_plan = json.loads(flash_plan_path.read_text(encoding="utf-8"))
    image_dir = flash_plan_path.parent
    application = image_dir / flash_plan["app"]["file"]
    bootloader = image_dir / flash_plan["bootloader"]["file"]
    factories = list(build.rglob("firmware.factory.bin"))
    if len(factories) != 1:
        raise SystemExit(f"Expected one factory image, found {factories}")
    factory_bytes = factories[0].read_bytes()
    for offset, relative_path in flash_plan["flash_files"].items():
        payload = (image_dir / relative_path).read_bytes()
        start = int(offset, 0)
        assert factory_bytes[start:start + len(payload)] == payload, relative_path
    shutil.copy2(application, output / f"{slug}.ota.bin")
    shutil.copy2(application.with_suffix(".elf"), output / "firmware.elf")
    shutil.copy2(flash_plan_path, output / "flasher_args.json")
    for role, binary in (("bootloader", bootloader), ("application", application)):
        container_path = "/config/" + binary.relative_to(ROOT).as_posix()
        run(["-c", "import json,sys; from esptool.bin_image import LoadFirmwareImage; "
             "i=LoadFirmwareImage('esp32p4',sys.argv[1]); "
             "assert 300 <= i.min_rev_full <= 302 <= i.max_rev_full, "
             "(i.min_rev_full,i.max_rev_full); "
             "print(json.dumps({'chip_id':i.chip_id,'min_revision':i.min_rev_full,"
             "'max_revision':i.max_rev_full}))", container_path], f"{role}-header.json", python=True)
        run(["-m", "esptool", "--chip", "esp32p4", "image-info", container_path],
            f"{role}-image-info.txt", python=True)
    factory = output / f"{slug}.factory.bin"
    shutil.copy2(factories[0], factory)
    checksum = hashlib.sha256(factory.read_bytes()).hexdigest()
    (output / "SHA256SUMS").write_text(f"{checksum}  {factory.name}\n", encoding="utf-8")
    shutil.copy2(sdk, output / "sdkconfig")
    (output / "build-info.json").write_text(json.dumps({
        "source_revision": revision, "firmware_version": version, "esphome": pinned,
        **json.loads(toolchain), "factory_image": factory.name, "sha256": checksum,
        "device": slug, "chip_target": "ESP32-P4 v3.x",
        "updaters_omitted": updates_disabled, "physical_device_tested": False,
    }, indent=2) + "\n", encoding="utf-8")
    print(f"Factory image: {factory}\nSHA-256: {checksum}")


if __name__ == "__main__":
    main()
