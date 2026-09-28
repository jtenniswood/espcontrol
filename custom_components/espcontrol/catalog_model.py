"""Dependency-free entity catalogue models used by the integration and tests."""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from .const import EntityRule


@dataclass(frozen=True, slots=True)
class EntityRecord:
    """Small, safe-to-send representation of a Home Assistant entity."""

    entity_id: str
    domain: str
    device_id: str | None
    name: str
    device_name: str | None
    area_name: str | None
    device_class: str | None
    icon: str | None
    unit: str | None
    state: str
    available: bool
    disabled: bool
    hidden: bool
    capabilities: tuple[str, ...]

    def as_dict(self) -> dict[str, Any]:
        """Return selection metadata without forwarding the live state value."""

        return {
            "entity_id": self.entity_id,
            "domain": self.domain,
            "device_id": self.device_id,
            "name": self.name,
            "device_name": self.device_name,
            "area_name": self.area_name,
            "device_class": self.device_class,
            "icon": self.icon,
            "unit": self.unit,
            "available": self.available,
            "disabled": self.disabled,
            "hidden": self.hidden,
            "capabilities": list(self.capabilities),
        }


def entity_matches_rule(record: EntityRecord, rule: EntityRule) -> bool:
    """Return whether an entity is valid for a picker field."""

    if not rule.allow_any_domain and rule.domains and record.domain not in rule.domains:
        return False
    return rule.capabilities.issubset(set(record.capabilities))
