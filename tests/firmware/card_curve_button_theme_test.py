"""Keep ordinary controls at their existing radii when card curves change."""
from pathlib import Path
import argparse
from string import Template
import sys

import yaml

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from device_profiles import load_device_profiles, web_config

# These are the ordinary button sizes before introducing larger grid curves.
BUTTON_RADII = {
    "guition-esp32-p4-jc1060p470": 10,
    "guition-esp32-p4-jc1060p470-v2": 10,
    "guition-esp32-p4-jc4880p443": 10,
    "guition-esp32-p4-jc4880p443-v3": 10,
    "guition-esp32-p4-jc8012p4a1": 11,
    "guition-esp32-p4-jc8012p4a1-v2": 11,
    "guition-esp32-p4-jc8012p4a1-v3": 11,
    "esp32-p4-86": 12,
    "guition-esp32-s3-4848s040": 8,
}
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--theme", type=Path, default=ROOT / "common/theme/button.yaml")
args = parser.parse_args()
theme = yaml.safe_load(args.theme.read_text())
profiles = load_device_profiles()
assert set(profiles) == set(BUTTON_RADII)
for slug, profile in profiles.items():
    substitutions = {**theme["substitutions"], **profile["firmware"]["package"]["substitutions"]}
    radius = theme["lvgl"]["theme"]["button"]["radius"]
    for _ in range(len(substitutions)):
        if "$" not in radius:
            break
        radius = Template(radius).substitute(substitutions)
    assert radius == f"{BUTTON_RADII[slug]}px", (slug, radius)
    # Card presets and browser previews continue using the larger card base.
    card_radius = int(substitutions["radius"].removesuffix("px"))
    assert card_radius > BUTTON_RADII[slug], slug
    assert web_config(profile)["cardRadiusPixels"] == card_radius, slug

s3 = profiles["guition-esp32-s3-4848s040"]
assert web_config(s3)["cardRadiusPixels"] == 26
assert web_config(s3)["cardCurveRadii"] == {"Small": 14, "Medium": 20}
print("Ordinary button radii stay independent of card curves on all nine profiles.")
