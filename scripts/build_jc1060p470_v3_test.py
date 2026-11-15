#!/usr/bin/env python3
"""Clean-build the isolated 7-inch V3 experiment and verify its target and updates."""

from build_jc8012p4a1_v3_test import main


if __name__ == "__main__":
    main(slug="guition-esp32-p4-jc1060p470-v3-test", name="espctl-7v3-test",
         version_prefix="dev-7v3", updates_disabled=True)
