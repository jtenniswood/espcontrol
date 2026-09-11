#!/usr/bin/env python3
"""Load and validate Product Model v2, including its generated-source entries.

The model makes current product ownership explicit without moving stable source
files yet. Generators and validators can resolve their inputs through this
module, so later migrations change one declared boundary rather than many
unrelated paths.
"""

from __future__ import annotations

import copy
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parent.parent
PRODUCT_MODEL_V2_JSON = ROOT / "product" / "model_v2.json"
PRODUCT_MODEL_V2_VERSION = 2
REQUIRED_SOURCES = {
    "cardContract": ("file", "authored"),
    "deviceCatalog": ("file", "authored"),
    "deviceProfiles": ("file", "generated-legacy-adapter"),
    "entityNames": ("file", "authored"),
    "icons": ("file", "authored"),
    "translations": ("glob", "authored"),
    "compatibilityFixtures": ("file", "authored"),
}
PRODUCT_MODEL_STAGES = {"legacy-adapter", "generated-pilot", "generated-source"}


class ProductModelV2Error(RuntimeError):
    pass


@dataclass(frozen=True)
class ProductSource:
    identifier: str
    path: Path
    kind: str
    role: str

    def files(self) -> tuple[Path, ...]:
        if self.kind == "file":
            return (self.path,)
        return tuple(sorted(self.path.parent.glob(self.path.name)))


@dataclass(frozen=True)
class ProductModelV2:
    sources: dict[str, ProductSource]
    card_type: str
    device_slug: str
    pilot_cards: dict[str, Path]
    pilot_devices: dict[str, Path]

    def source_path(self, identifier: str) -> Path:
        source = self.sources.get(identifier)
        if source is None:
            raise ProductModelV2Error(f"Product Model v2 does not define source {identifier!r}")
        if source.kind != "file":
            raise ProductModelV2Error(f"Product Model v2 source {identifier!r} is not a file")
        return source.path

    def source_directory(self, identifier: str) -> Path:
        source = self.sources.get(identifier)
        if source is None:
            raise ProductModelV2Error(f"Product Model v2 does not define source {identifier!r}")
        return source.path.parent

    def source_json(self, identifier: str) -> dict[str, Any]:
        path = self.source_path(identifier)
        return _load_json(path)

    def pilot_json(self, kind: str, identifier: str) -> dict[str, Any]:
        if kind == "cards":
            sources = self.pilot_cards
        elif kind == "devices":
            sources = self.pilot_devices
        else:
            raise ProductModelV2Error(f"unknown Product Model v2 pilot kind {kind!r}")
        path = sources.get(identifier)
        if path is None:
            raise ProductModelV2Error(f"Product Model v2 does not define pilot {kind}.{identifier}")
        return _load_json(path)

    def card_contract_data(self) -> dict[str, Any]:
        data = self.source_json("cardContract")
        if not self.pilot_cards:
            return data
        cards = data.get("cards")
        if not isinstance(cards, dict):
            raise ProductModelV2Error("cardContract.cards must be an object")
        data = copy.deepcopy(data)
        for card_type in self.pilot_cards:
            if card_type not in cards:
                raise ProductModelV2Error(f"pilot card {card_type!r} is not present in cardContract.cards")
            data["cards"][card_type] = self.pilot_json("cards", card_type)
        return data

    def device_catalog_data(self) -> dict[str, Any]:
        data = self.source_json("deviceCatalog")
        if not self.pilot_devices:
            return data
        devices = data.get("devices")
        if not isinstance(devices, dict):
            raise ProductModelV2Error("deviceCatalog.devices must be an object")
        data = copy.deepcopy(data)
        for slug in self.pilot_devices:
            if slug not in devices:
                raise ProductModelV2Error(f"pilot device {slug!r} is not present in deviceCatalog.devices")
            data["devices"][slug] = self.pilot_json("devices", slug)
        return data

    def sample_card(self) -> dict[str, Any]:
        cards = self.card_contract_data().get("cards")
        if not isinstance(cards, dict) or self.card_type not in cards:
            raise ProductModelV2Error("equivalence sample card must exist in cardContract.cards")
        card = cards[self.card_type]
        if not isinstance(card, dict):
            raise ProductModelV2Error("equivalence sample card must be an object")
        return card

    def sample_device(self) -> dict[str, Any]:
        devices = self.device_catalog_data().get("devices")
        if not isinstance(devices, dict) or self.device_slug not in devices:
            raise ProductModelV2Error("equivalence sample device must exist in deviceCatalog.devices")
        device = devices[self.device_slug]
        if not isinstance(device, dict):
            raise ProductModelV2Error("equivalence sample device must be an object")
        return device


def _reject_duplicate_keys(path: Path, pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ProductModelV2Error(f"{path.relative_to(ROOT)} contains duplicate key {key!r}")
        result[key] = value
    return result


def _load_json(path: Path) -> dict[str, Any]:
    try:
        with path.open(encoding="utf-8") as handle:
            data = json.load(handle, object_pairs_hook=lambda pairs: _reject_duplicate_keys(path, pairs))
    except FileNotFoundError as exc:
        raise ProductModelV2Error(f"{path.relative_to(ROOT)} is missing") from exc
    except json.JSONDecodeError as exc:
        raise ProductModelV2Error(f"{path.relative_to(ROOT)} is not valid JSON: {exc}") from exc
    if not isinstance(data, dict):
        raise ProductModelV2Error(f"{path.relative_to(ROOT)} must contain an object")
    return data


def _source_path(identifier: str, source_data: dict[str, Any]) -> ProductSource:
    expected_kind, expected_role = REQUIRED_SOURCES[identifier]
    if set(source_data) != {"path", "kind", "role"}:
        raise ProductModelV2Error(f"sources.{identifier} must contain path, kind, and role")
    path_value = source_data["path"]
    if not isinstance(path_value, str) or not path_value:
        raise ProductModelV2Error(f"sources.{identifier}.path must be a non-empty string")
    path = (ROOT / path_value).resolve()
    if ROOT not in path.parents and path != ROOT:
        raise ProductModelV2Error(f"sources.{identifier}.path must remain inside the repository")
    if source_data["kind"] != expected_kind:
        raise ProductModelV2Error(f"sources.{identifier}.kind must be {expected_kind!r}")
    if source_data["role"] != expected_role:
        raise ProductModelV2Error(f"sources.{identifier}.role must be {expected_role!r}")
    source = ProductSource(identifier, path, expected_kind, expected_role)
    files = source.files()
    if not files:
        raise ProductModelV2Error(f"sources.{identifier}.path does not match any files")
    if expected_kind == "file" and not path.is_file():
        raise ProductModelV2Error(f"sources.{identifier}.path must name a file")
    return source


def _pilot_sources(kind: str, data: Any) -> dict[str, Path]:
    if not isinstance(data, dict) or not data:
        raise ProductModelV2Error(f"pilots.{kind} must be a non-empty object")
    sources: dict[str, Path] = {}
    for identifier, path_value in data.items():
        # The card contract's default switch is intentionally keyed by an
        # empty string. It is a fallback entry, not a user-selectable card
        # type, so allow it only for card pilots. Device identifiers must
        # remain explicit non-empty slugs.
        if not isinstance(identifier, str) or (not identifier and kind != "cards"):
            raise ProductModelV2Error(f"pilots.{kind} identifiers must be non-empty strings")
        if not isinstance(path_value, str) or not path_value:
            raise ProductModelV2Error(f"pilots.{kind}.{identifier} must be a non-empty path")
        path = (ROOT / path_value).resolve()
        if ROOT not in path.parents or not path.is_file():
            raise ProductModelV2Error(f"pilots.{kind}.{identifier} must name a file inside the repository")
        sources[identifier] = path
    return sources


def load_product_model_v2(path: Path = PRODUCT_MODEL_V2_JSON) -> ProductModelV2:
    data = _load_json(path)
    if data.get("modelVersion") != PRODUCT_MODEL_V2_VERSION:
        raise ProductModelV2Error(f"modelVersion must be {PRODUCT_MODEL_V2_VERSION}")
    stage = data.get("stage")
    if stage not in PRODUCT_MODEL_STAGES:
        raise ProductModelV2Error(
            "stage must be 'legacy-adapter', 'generated-pilot', or 'generated-source'"
        )
    if not isinstance(data.get("description"), str) or not data["description"].strip():
        raise ProductModelV2Error("description must be a non-empty string")
    sources_data = data.get("sources")
    if not isinstance(sources_data, dict) or set(sources_data) != set(REQUIRED_SOURCES):
        raise ProductModelV2Error("sources must define every Product Model v2 source exactly once")
    if not all(isinstance(value, dict) for value in sources_data.values()):
        raise ProductModelV2Error("every source must be an object")
    sources = {
        identifier: _source_path(identifier, sources_data[identifier])
        for identifier in REQUIRED_SOURCES
    }
    samples = data.get("equivalenceSamples")
    if not isinstance(samples, dict) or set(samples) != {"cardType", "deviceSlug"}:
        raise ProductModelV2Error("equivalenceSamples must define cardType and deviceSlug")
    card_type = samples["cardType"]
    device_slug = samples["deviceSlug"]
    if not isinstance(card_type, str) or not isinstance(device_slug, str) or not device_slug:
        raise ProductModelV2Error("equivalence sample values must be strings, with a non-empty deviceSlug")
    pilots = data.get("pilots")
    if stage == "legacy-adapter":
        if pilots is not None:
            raise ProductModelV2Error("legacy-adapter models must not declare pilots")
        pilot_cards: dict[str, Path] = {}
        pilot_devices: dict[str, Path] = {}
    else:
        if not isinstance(pilots, dict) or set(pilots) != {"cards", "devices"}:
            raise ProductModelV2Error("generated Product Model sources must declare cards and devices pilots")
        pilot_cards = _pilot_sources("cards", pilots["cards"])
        pilot_devices = _pilot_sources("devices", pilots["devices"])
        if card_type not in pilot_cards or device_slug not in pilot_devices:
            raise ProductModelV2Error("equivalence samples must be Product Model pilot sources")
        if stage == "generated-source":
            contract_cards = _load_json(sources["cardContract"].path).get("cards")
            catalog_devices = _load_json(sources["deviceCatalog"].path).get("devices")
            if not isinstance(contract_cards, dict) or set(pilot_cards) != set(contract_cards):
                raise ProductModelV2Error(
                    "generated-source cards must define every card-contract entry exactly once"
                )
            if not isinstance(catalog_devices, dict) or set(pilot_devices) != set(catalog_devices):
                raise ProductModelV2Error(
                    "generated-source devices must define every device-catalog entry exactly once"
                )
    return ProductModelV2(sources, card_type, device_slug, pilot_cards, pilot_devices)


def source_path(identifier: str) -> Path:
    return load_product_model_v2().source_path(identifier)


def source_directory(identifier: str) -> Path:
    return load_product_model_v2().source_directory(identifier)
