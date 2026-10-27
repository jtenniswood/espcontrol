"""Resolve real factory packages to protect SDK-owned subscription storage."""
from pathlib import Path

from esphome import yaml_util
from esphome.components import packages, substitutions
from esphome.core import CORE

ROOT = Path(__file__).resolve().parents[2]
profiles = sorted((ROOT / "builds").glob("*.factory.yaml"))
assert profiles, "No factory profiles found"

for path in profiles:
    CORE.reset()
    CORE.config_path = path
    config = packages.do_packages_pass(yaml_util.load_yaml(path))
    config = substitutions.do_substitution_pass(config)
    config = packages.merge_packages(config)
    options = config["esp32"]["framework"]["sdkconfig_options"]
    assert options.get("CONFIG_SPIRAM_USE_MALLOC") in (True, "y"), path.name
    assert options.get("CONFIG_SPIRAM_USE_CAPS_ALLOC") not in (True, "y"), path.name
    assert int(options["CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL"]) == 4096, path.name
    assert int(options["CONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL"]) == 32768, path.name

print(f"PSRAM ordinary-allocation policy resolves in all {len(profiles)} factory profiles.")
