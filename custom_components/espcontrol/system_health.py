"""System health information for the EspControl proof of concept."""

from __future__ import annotations

from homeassistant.components import system_health
from homeassistant.core import HomeAssistant, callback

from .const import DOMAIN


@callback
def async_register(hass: HomeAssistant, register: system_health.SystemHealthRegistration) -> None:
    """Register a small, non-sensitive health summary."""

    @callback
    def info() -> dict[str, object]:
        entries = [
            value
            for key, value in hass.data.get(DOMAIN, {}).items()
            if key != "pairing" and hasattr(value, "available")
        ]
        return {
            "configured_displays": len(entries),
            "available_displays": sum(1 for value in entries if value.available),
            "pairing_grants": "short-lived",
        }

    register.async_register_info(info)
