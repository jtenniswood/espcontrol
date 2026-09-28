"""EspControl Home Assistant integration proof of concept."""

from __future__ import annotations

from typing import Any

import voluptuous as vol
from homeassistant.config_entries import ConfigEntry, ConfigEntryState
from homeassistant.core import HomeAssistant, ServiceCall, ServiceResponse, SupportsResponse
from homeassistant.exceptions import ServiceValidationError
from homeassistant.helpers import device_registry as dr
from homeassistant.helpers.aiohttp_client import async_get_clientsession
from homeassistant.helpers.device_registry import format_mac

from .api import register_views
from .const import (
    CONF_DEVICE_ID,
    CONF_HOST,
    CONF_WEB_PORT,
    DOMAIN,
    SERVICE_CREATE_PAIRING_TOKEN,
)
from .pairing import PairingStore
from .device import EspControlRuntime

type EspControlConfigEntry = ConfigEntry[EspControlRuntime]


async def async_setup(hass: HomeAssistant, config: dict[str, Any]) -> bool:
    """Set up the integration and its authenticated pairing API."""

    pairing = PairingStore()
    hass.data[DOMAIN] = {"pairing": pairing}
    register_views(hass, pairing)

    async def create_pairing_token(call: ServiceCall) -> ServiceResponse:
        device_id = call.data[CONF_DEVICE_ID]
        entry = next(
            (
                candidate
                for candidate in hass.config_entries.async_entries(DOMAIN)
                if candidate.data.get(CONF_DEVICE_ID) == device_id
                and candidate.state is ConfigEntryState.LOADED
            ),
            None,
        )
        if entry is None:
            raise ServiceValidationError("The EspControl display is not configured")
        grant = pairing.issue(device_id)
        return {"device_id": device_id, "token": grant.token, "expires_at": grant.expires_at}

    hass.services.async_register(
        DOMAIN,
        SERVICE_CREATE_PAIRING_TOKEN,
        create_pairing_token,
        schema=vol.Schema({vol.Required(CONF_DEVICE_ID): str}),
        supports_response=SupportsResponse.ONLY,
    )
    return True


async def async_setup_entry(hass: HomeAssistant, entry: EspControlConfigEntry) -> bool:
    """Set up one discovered display."""

    device_id = entry.data[CONF_DEVICE_ID]
    mac = format_mac(entry.data["mac"]) if entry.data.get("mac") else None
    dr.async_get(hass).async_get_or_create(
        config_entry_id=entry.entry_id,
        identifiers={(DOMAIN, device_id)},
        name=entry.title,
        manufacturer="EspControl",
        model=entry.data.get("model"),
        connections={("mac", mac)} if mac else set(),
    )
    runtime = EspControlRuntime(
        host=entry.options.get(CONF_HOST, entry.data[CONF_HOST]),
        web_port=entry.options.get(CONF_WEB_PORT, entry.data.get(CONF_WEB_PORT, 80)),
    )
    hass.data[DOMAIN][entry.entry_id] = runtime
    await runtime.async_probe(async_get_clientsession(hass))
    return True


async def async_unload_entry(hass: HomeAssistant, entry: EspControlConfigEntry) -> bool:
    """Unload one display runtime."""

    hass.data[DOMAIN].pop(entry.entry_id, None)
    return True
