"""Pure tests for the security-sensitive parts of the EspControl POC."""

from __future__ import annotations

import time
import unittest
from unittest.mock import patch

from custom_components.espcontrol.catalog_model import EntityRecord, entity_matches_rule
from custom_components.espcontrol.const import ENTITY_RULES
from custom_components.espcontrol.pairing import PairingStore


def _record(domain: str, capabilities: tuple[str, ...] = ()) -> EntityRecord:
    return EntityRecord(
        entity_id=f"{domain}.example",
        domain=domain,
        device_id="device-1",
        name="Example",
        device_name="Panel",
        area_name="Office",
        device_class=None,
        icon=None,
        unit=None,
        state="on",
        available=True,
        disabled=False,
        hidden=False,
        capabilities=capabilities,
    )


class PairingTests(unittest.TestCase):
    def test_pairing_grant_is_scoped_and_revocable(self) -> None:
        store = PairingStore()
        grant = store.issue("panel-1", ttl=30)
        self.assertTrue(store.validate("panel-1", grant.token))
        self.assertFalse(store.validate("panel-2", grant.token))
        store.revoke("panel-1")
        self.assertFalse(store.validate("panel-1", grant.token))

    def test_pairing_grant_expiry_is_enforced(self) -> None:
        now = int(time.time())
        with patch("custom_components.espcontrol.pairing.time.time", return_value=now):
            store = PairingStore()
            grant = store.issue("panel-1", ttl=30)
        with patch("custom_components.espcontrol.pairing.time.time", return_value=now + 31):
            self.assertFalse(store.validate("panel-1", grant.token))

    def test_field_rules_cover_known_and_future_domains(self) -> None:
        self.assertTrue(entity_matches_rule(_record("light"), ENTITY_RULES["light"]))
        self.assertFalse(entity_matches_rule(_record("switch"), ENTITY_RULES["light"]))
        self.assertTrue(entity_matches_rule(_record("future_domain"), ENTITY_RULES["entity"]))
