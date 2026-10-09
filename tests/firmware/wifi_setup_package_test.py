"""Check real package expansion includes a portal in all WiFi setup profiles."""
from io import StringIO
from pathlib import Path
from esphome.core import CORE
from esphome import yaml_util
from esphome.components.packages import resolve_packages

ROOT = Path(__file__).resolve().parents[2]
configs = sorted((ROOT / "devices").glob("*/dev.yaml")) + sorted((ROOT / "builds").glob("*.factory.yaml"))
assert configs
for path in configs:
    CORE.reset()
    CORE.config_path = path
    # Do not require or read a user's secrets for this package contract check.
    text = path.read_text().replace("!secret wifi_ssid", "test-network").replace("!secret wifi_password", "test-password")
    config = yaml_util.parse_yaml(path, StringIO(text))
    merged = resolve_packages(config)
    wifi = merged.get("wifi", {})
    if "ap" in wifi:
        assert "captive_portal" in merged, f"{path.relative_to(ROOT)}: setup AP needs a captive portal"
        assert "improv_serial" in merged, f"{path.relative_to(ROOT)}: USB provisioning missing"
        print(f"{path.relative_to(ROOT)}: setup AP, captive portal and USB provisioning present")
print("WiFi setup package expansion checks passed.")
