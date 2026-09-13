#!/usr/bin/env python3
"""Clean-build the experimental V3 factory image with a traceable source revision."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SLUG = "guition-esp32-p4-jc8012p4a1-v3-test"
CONFIG = f"/config/builds/{SLUG}.factory.yaml"


def git(*args: str) -> str:
    return subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()


def main() -> None:
    if git("status", "--porcelain", "--untracked-files=normal"):
        raise SystemExit("Commit the test sources first so the binary matches its recorded commit.")
    revision = git("rev-parse", "HEAD")
    version = f"dev-v3-{revision[:12]}"
    pinned = (ROOT / ".github/esphome.env").read_text().strip().removeprefix("ESPHOME_VERSION=")
    image = f"ghcr.io/esphome/esphome:{pinned}"
    output = ROOT / "build" / "v3-test" / revision
    output.mkdir(parents=True, exist_ok=True)
    # A worktree's .git points outside /config; preserve that path in the container.
    git_dir = git("rev-parse", "--path-format=absolute", "--git-common-dir")
    docker = ["docker", "run", "--rm", "--user"]
    docker += [f"{os.getuid()}:{os.getgid()}", "-e", "HOME=/tmp",
               "-v", f"{ROOT}:/config", "-v", f"{git_dir}:{git_dir}:ro"]

    def run(args: list[str], log_name: str, *, python: bool = False) -> str:
        command = docker + (["--entrypoint", "python"] if python else []) + [image] + args
        with (output / log_name).open("w") as log:
            result = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, text=True)
        content = (output / log_name).read_text()
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
    run(options + ["config", CONFIG], "config.log")
    run(options + ["clean", CONFIG], "clean.log")
    run(options + ["compile", CONFIG], "compile.log")

    build = ROOT / "builds/.esphome/build/espctl-v3-test"
    sdk_files = list(build.rglob("sdkconfig*"))
    sdk = next((p for p in sdk_files if p.is_file() and "CONFIG_IDF_TARGET=" in p.read_text()), None)
    if sdk is None:
        raise SystemExit(f"Cannot find generated sdkconfig in {build}")
    sdk_text = sdk.read_text()
    assert "CONFIG_ESP32P4_SELECTS_REV_LESS_V3=y" not in sdk_text
    assert ("# CONFIG_ESP32P4_SELECTS_REV_LESS_V3 is not set" in sdk_text
            or "CONFIG_ESP32P4_SELECTS_REV_LESS_V3=n" in sdk_text)
    defines = (build / "src/esphome/core/defines.h").read_text()
    assert "#define USE_WEBSERVER_OTA_DISABLED" in defines
    # Confirm the external component was fetched from this commit, including the upload guard.
    server = (build / "src/esphome/components/web_server_idf/web_server_idf.cpp").read_text()
    assert "Browser firmware uploads are disabled" in server
    # Reject stale upstream sources: their legacy PHY default aborts on P4 v3.
    display = (build / "src/esphome/components/mipi_dsi/mipi_dsi.cpp").read_text()
    assert display == (ROOT / "components/mipi_dsi/mipi_dsi.cpp").read_text()
    assert ".phy_clk_src = {}," in display
    generated = (build / "src/main.cpp").read_text()
    assert "esphome::ESPHomeOTAComponent" in generated
    assert "->set_port(3232)" in generated
    for switch_id in ("auto_update_switch", "c6_auto_update_switch"):
        assert f"{switch_id}->set_restore_mode(switch_::SWITCH_ALWAYS_OFF);" in generated
    assert f"/firmware/{SLUG}/manifest.json" in generated
    assert "/firmware/guition-esp32-p4-jc8012p4a1-v2/manifest.json" not in generated

    # Use the native IDF flash plan to identify the actual app and bootloader.
    flash_plan_path = build / "build/flasher_args.json"
    flash_plan = json.loads(flash_plan_path.read_text())
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
    shutil.copy2(application, output / f"{SLUG}.ota.bin")
    shutil.copy2(application.with_suffix(".elf"), output / "firmware.elf")
    shutil.copy2(flash_plan_path, output / "flasher_args.json")
    for role, binary in (("bootloader", bootloader), ("application", application)):
        container_path = "/config/" + str(binary.relative_to(ROOT))
        run(["-c", "import json,sys; from esptool.bin_image import LoadFirmwareImage; "
             "i=LoadFirmwareImage('esp32p4',sys.argv[1]); "
             "assert 300 <= i.min_rev_full <= 302 <= i.max_rev_full, "
             "(i.min_rev_full,i.max_rev_full); "
             "print(json.dumps({'chip_id':i.chip_id,'min_revision':i.min_rev_full,"
             "'max_revision':i.max_rev_full}))", container_path], f"{role}-header.json", python=True)
        run(["-m", "esptool", "--chip", "esp32p4", "image-info", container_path],
            f"{role}-image-info.txt", python=True)
    factory = output / f"{SLUG}.factory.bin"
    shutil.copy2(factories[0], factory)
    checksum = hashlib.sha256(factory.read_bytes()).hexdigest()
    (output / "SHA256SUMS").write_text(f"{checksum}  {factory.name}\n")
    shutil.copy2(sdk, output / "sdkconfig")
    (output / "build-info.json").write_text(json.dumps({
        "source_revision": revision, "firmware_version": version, "esphome": pinned,
        **json.loads(toolchain), "factory_image": factory.name, "sha256": checksum,
        "chip_target": "ESP32-P4 v3.x", "physical_device_tested": False,
    }, indent=2) + "\n")
    print(f"Factory image: {factory}\nSHA-256: {checksum}")


if __name__ == "__main__":
    main()
