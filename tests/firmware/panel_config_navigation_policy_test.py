"""Verify the distinct flat-format and nested-page release gates."""

from pathlib import Path
from copy import deepcopy
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "scripts"))
from panel_config_navigation_policy import (
    navigation_depths, reader_floor_releases, validate_navigation_reader_floor,
)
from check_release_contract import validate_release_history


def reader_stage():
    return {
        "panelConfigDocumentVersion": 1,
        "panelConfigReadVersions": [1, 2],
        "panelConfigWriteVersion": 1,
        "panelConfigReadNavigationDepth": 2,
        "panelConfigWriteNavigationDepth": 1,
        "panelConfigMinimumReaderRelease": None,
        "panelConfigCompatibility": {"phase": "dual-write", "dualWriteStableReleases": 2,
                                     "readImportOnlyStableReleases": 1},
    }


def nested_stage():
    contract = reader_stage()
    contract.update(panelConfigDocumentVersion=2, panelConfigWriteVersion=2,
                    panelConfigWriteNavigationDepth=2, panelConfigMinimumReaderRelease="v9.9.9")
    contract["panelConfigCompatibility"]["phase"] = "read-import-only"
    return contract


def published(tag="v9.9.9", **declarations):
    # Synthetic history fixtures, never actual releases or a proposed floor.
    contract = {"panelConfigReadVersions": [1, 2], "panelConfigReadNavigationDepth": 2}
    contract.update(declarations)
    return {"tagName": tag, "manifest": {"releaseContract": contract}}


class NavigationPolicyTest(unittest.TestCase):
    def test_reader_bridge_then_two_flat_releases_allows_nested_release(self):
        bridge = reader_stage()
        history = []
        validate_release_history(bridge, history)
        history.insert(0, published("v9.9.9", **bridge))

        flat = deepcopy(bridge)
        flat.update(panelConfigDocumentVersion=2, panelConfigWriteVersion=2,
                    panelConfigMinimumReaderRelease="v9.9.9")
        validate_release_history(flat, history)
        history.insert(0, published("v9.10.0", **flat))
        with self.assertRaisesRegex(AssertionError, "2 consecutive published dual-write"):
            validate_release_history(nested_stage(), history)

        validate_release_history(flat, history)
        history.insert(0, published("v9.11.0", **flat))
        validate_release_history(nested_stage(), history)
        self.assertEqual(navigation_depths(nested_stage()), (2, 2))

    def test_actual_release_history_gate_requires_nested_reader_proof(self):
        older = nested_stage()
        older["panelConfigWriteNavigationDepth"] = 1
        older["panelConfigReadNavigationDepth"] = 1
        older["panelConfigCompatibility"]["phase"] = "dual-write"
        newer = deepcopy(older)
        newer["panelConfigReadNavigationDepth"] = 2
        history = [{"tagName": "v9.10.0", "manifest": {"releaseContract": newer}},
                   {"tagName": "v9.9.9", "manifest": {"releaseContract": older}}]
        with self.assertRaisesRegex(AssertionError, "depth-two page reader"):
            validate_release_history(nested_stage(), history)
        older["panelConfigReadNavigationDepth"] = 2
        validate_release_history(nested_stage(), history)

    def test_dual_reader_can_ship_without_enabling_nested_writes(self):
        contract = reader_stage()
        self.assertEqual(navigation_depths(contract), (2, 1))
        validate_navigation_reader_floor(contract, [])

    def test_flat_durable_two_does_not_enable_nested_writes(self):
        contract = reader_stage()
        contract.update(panelConfigDocumentVersion=2, panelConfigWriteVersion=2,
                        panelConfigReadNavigationDepth=1, panelConfigMinimumReaderRelease="v9.9.9")
        history = [published(panelConfigReadNavigationDepth=1)]
        self.assertEqual(navigation_depths(contract), (1, 1))
        self.assertEqual(reader_floor_releases(contract, history), history)
        validate_navigation_reader_floor(contract, history)

    def test_nested_writer_accepts_a_qualified_reader_floor(self):
        validate_navigation_reader_floor(nested_stage(), [published()])

    def test_format_two_support_does_not_prove_nested_reading(self):
        with self.assertRaisesRegex(AssertionError, "depth-two page reader"):
            validate_navigation_reader_floor(nested_stage(), [published(panelConfigReadNavigationDepth=1)])

    def test_every_release_in_supported_downgrade_range_must_read_nested_pages(self):
        history = [published("v9.10.0", panelConfigReadNavigationDepth=1), published()]
        with self.assertRaisesRegex(AssertionError, "depth-two page reader"):
            validate_navigation_reader_floor(nested_stage(), history)
        history[0] = published("v9.10.0")
        validate_navigation_reader_floor(nested_stage(), history)

    def test_releases_older_than_the_floor_are_outside_the_supported_range(self):
        history = [published("v9.10.0"), published(), {"tagName": "v9.9.8", "manifest": {}}]
        self.assertEqual(len(reader_floor_releases(nested_stage(), history)), 2)
        validate_navigation_reader_floor(nested_stage(), history)

    def test_undeclared_or_malformed_historical_depth_is_not_reader_proof(self):
        for depth in (None, False, True, 0, 1, "2", 2.0):
            with self.subTest(depth=depth), self.assertRaises(AssertionError):
                validate_navigation_reader_floor(nested_stage(), [published(panelConfigReadNavigationDepth=depth)])
        undeclared = published()
        del undeclared["manifest"]["releaseContract"]["panelConfigReadNavigationDepth"]
        with self.assertRaises(AssertionError):
            validate_navigation_reader_floor(nested_stage(), [undeclared])

    def test_manifest_and_format_reader_proof_are_required(self):
        for manifest in ({}, None, [], {"releaseContract": None}):
            with self.subTest(manifest=manifest), self.assertRaises(AssertionError):
                validate_navigation_reader_floor(nested_stage(), [{"tagName": "v9.9.9", "manifest": manifest}])
        with self.assertRaises(AssertionError):
            validate_navigation_reader_floor(nested_stage(), [published(panelConfigReadVersions=[1])])

    def test_floor_must_exist_and_be_a_stable_release(self):
        for floor in (None, "", "v9.9.9-rc1", "9.9.9", True):
            contract = nested_stage()
            contract["panelConfigMinimumReaderRelease"] = floor
            with self.subTest(floor=floor), self.assertRaises(AssertionError):
                validate_navigation_reader_floor(contract, [published()])
        with self.assertRaisesRegex(AssertionError, "not published"):
            validate_navigation_reader_floor(nested_stage(), [published("v9.10.0")])

    def test_nested_writer_requires_mirror_retirement(self):
        for phase in ("dual-write", None, "unknown"):
            contract = nested_stage()
            contract["panelConfigCompatibility"]["phase"] = phase
            with self.subTest(phase=phase), self.assertRaisesRegex(AssertionError, "legacy write mirror"):
                validate_navigation_reader_floor(contract, [published()])

    def test_nested_writer_requires_native_two_at_both_policy_points(self):
        for field in ("panelConfigDocumentVersion", "panelConfigWriteVersion"):
            contract = nested_stage()
            contract[field] = 1
            with self.subTest(field=field), self.assertRaisesRegex(AssertionError, "durable document version two"):
                navigation_depths(contract)

    def test_current_declared_depths_are_bounded_and_have_real_readers(self):
        for field in ("panelConfigReadNavigationDepth", "panelConfigWriteNavigationDepth"):
            for depth in (None, False, True, 0, 3, "2", 2.0):
                contract = reader_stage()
                contract[field] = depth
                with self.subTest(field=field, depth=depth), self.assertRaises(AssertionError):
                    navigation_depths(contract)
        contract = nested_stage()
        contract["panelConfigReadNavigationDepth"] = 1
        with self.assertRaises(AssertionError):
            navigation_depths(contract)
        contract = reader_stage()
        contract["panelConfigReadVersions"] = [1]
        with self.assertRaisesRegex(AssertionError, "version-two document reader"):
            navigation_depths(contract)


if __name__ == "__main__":
    unittest.main()
