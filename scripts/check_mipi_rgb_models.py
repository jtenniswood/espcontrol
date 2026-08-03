#!/usr/bin/env python3
"""Check bundled MIPI RGB models against the installed ESPHome API."""

from __future__ import annotations

import importlib
from pathlib import Path
import pkgutil
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from esphome.components.mipi import DriverChip
import esphome.config_validation as cv
from esphome.const import (
    CONF_MIRROR_X,
    CONF_MIRROR_Y,
    CONF_SWAP_XY,
    __version__ as ESPHOME_VERSION,
)

from components.mipi_rgb import models


ALL_TRANSFORMS = (CONF_MIRROR_X, CONF_MIRROR_Y, CONF_SWAP_XY)


def load_models() -> dict[str, DriverChip]:
    """Import every bundled model module and return its registrations."""
    for module_info in pkgutil.iter_modules(models.__path__):
        importlib.import_module(f"components.mipi_rgb.models.{module_info.name}")
    return DriverChip.get_models()


def main() -> int:
    registered_models = load_models()
    if not registered_models:
        print("No MIPI RGB models were registered.")
        return 1

    failures: list[str] = []
    disabled_transforms = {name: False for name in ALL_TRANSFORMS}

    for name, model in sorted(registered_models.items()):
        try:
            supported = set(model.transforms)
            schema = model.transform_schema()
            schema(disabled_transforms)
        except Exception as error:  # noqa: BLE001 - report model API failures together
            failures.append(f"{name}: could not load transform support: {error}")
            continue

        if CONF_SWAP_XY in supported:
            failures.append(f"{name}: RGB displays must not advertise swap_xy support")

        try:
            schema({**disabled_transforms, CONF_SWAP_XY: True})
        except cv.Invalid:
            pass
        else:
            failures.append(f"{name}: transform schema accepted swap_xy: true")

    if failures:
        print("MIPI RGB model compatibility check failed:")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print(
        f"MIPI RGB model compatibility passed for {len(registered_models)} models "
        f"with ESPHome {ESPHOME_VERSION}."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
