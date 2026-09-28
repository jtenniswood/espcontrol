"""Entity catalogue and matching helpers used by the EspControl API."""

from __future__ import annotations

from collections.abc import Iterable
from typing import Any

from homeassistant.core import HomeAssistant, State
from homeassistant.helpers import area_registry as ar
from homeassistant.helpers import device_registry as dr, entity_registry as er

from .const import (
    DEFAULT_CATALOG_LIMIT,
    ENTITY_RULES,
    MAX_CATALOG_LIMIT,
)
from .catalog_model import EntityRecord, entity_matches_rule


def _entry_value(entry: Any, name: str, default: Any = None) -> Any:
    """Read registry values across supported Home Assistant versions."""

    return getattr(entry, name, default)


def _capabilities(state: State) -> tuple[str, ...]:
    """Extract capability names without exposing raw entity attributes."""

    attributes = state.attributes
    result: set[str] = set()
    supported = attributes.get("supported_features")
    if supported is not None:
        result.add("supported_features")
        result.add(f"supported_features:{supported}")
    for key in (
        "brightness",
        "color_temp",
        "effect",
        "fan_modes",
        "preset_modes",
        "swing_modes",
        "hvac_modes",
        "target_temp_step",
        "min_temp",
        "max_temp",
        "current_position",
        "current_tilt_position",
        "media_title",
        "volume_level",
        "source_list",
        "options",
        "percentage",
    ):
        if key in attributes:
            result.add(key)
    return tuple(sorted(result))


def _record_for_state(
    state: State,
    entity_registry: er.EntityRegistry,
    device_registry: dr.DeviceRegistry,
    area_registry: ar.AreaRegistry,
) -> EntityRecord:
    """Build one catalogue item from state and registry metadata."""

    entry = entity_registry.async_get(state.entity_id)
    device = None
    if entry and entry.device_id:
        device = device_registry.async_get(entry.device_id)
    attributes = state.attributes
    name = (
        _entry_value(entry, "name")
        or attributes.get("friendly_name")
        or state.entity_id
    )
    device_name = _entry_value(device, "name") if device else None
    area_id = _entry_value(entry, "area_id") or _entry_value(device, "area_id")
    area = area_registry.async_get_area(area_id) if area_id else None
    return EntityRecord(
        entity_id=state.entity_id,
        domain=state.entity_id.split(".", 1)[0],
        device_id=entry.device_id if entry else None,
        name=str(name),
        device_name=device_name,
        area_name=area.name if area else None,
        device_class=attributes.get("device_class"),
        icon=attributes.get("icon"),
        unit=attributes.get("unit_of_measurement"),
        state=state.state,
        available=state.state not in {"unknown", "unavailable"},
        disabled=bool(_entry_value(entry, "disabled_by")),
        hidden=bool(_entry_value(entry, "hidden_by")),
        capabilities=_capabilities(state),
    )


def _matches_query(record: EntityRecord, query: str) -> bool:
    if not query:
        return True
    haystack = " ".join(
        value or ""
        for value in (record.entity_id, record.name, record.device_name, record.area_name)
    ).casefold()
    return query.casefold() in haystack


def build_entity_catalog(
    hass: HomeAssistant,
    *,
    query: str = "",
    field: str = "entity",
    area: str | None = None,
    device_id: str | None = None,
    include_hidden: bool = False,
    include_disabled: bool = False,
    limit: int = DEFAULT_CATALOG_LIMIT,
    cursor: int = 0,
) -> tuple[list[dict[str, Any]], int | None]:
    """Return a sorted, filtered page of entities and the next offset."""

    rule = ENTITY_RULES.get(field, ENTITY_RULES["entity"])
    limit = max(1, min(limit, MAX_CATALOG_LIMIT))
    entity_registry = er.async_get(hass)
    device_registry = dr.async_get(hass)
    area_registry = ar.async_get(hass)
    records: Iterable[EntityRecord] = (
        _record_for_state(state, entity_registry, device_registry, area_registry)
        for state in hass.states.async_all()
    )
    filtered = [
        record
        for record in records
        if entity_matches_rule(record, rule)
        and _matches_query(record, query)
        and (area is None or record.area_name == area)
        and (device_id is None or record.device_id == device_id)
        and (include_hidden or not record.hidden)
        and (include_disabled or not record.disabled)
    ]
    filtered.sort(key=lambda item: (item.name.casefold(), item.entity_id))
    start = max(0, cursor)
    page = filtered[start : start + limit]
    next_cursor = start + limit if start + limit < len(filtered) else None
    return [record.as_dict() for record in page], next_cursor
