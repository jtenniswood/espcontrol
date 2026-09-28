"""HTTP API used by the POC configurator and pairing flow."""

from __future__ import annotations

import base64
import json
from http import HTTPStatus
from urllib.parse import urlsplit

from aiohttp import web
from homeassistant.components.http import HomeAssistantView
from homeassistant.config_entries import ConfigEntryState
from homeassistant.core import HomeAssistant, callback

from .catalog import build_entity_catalog
from .const import CONF_HOST, DEFAULT_CATALOG_LIMIT, DOMAIN, PAIRING_TOKEN_HEADER
from .pairing import PairingStore


class EspControlPairingView(HomeAssistantView):
    """Issue a temporary catalogue credential from an authenticated HA session."""

    url = "/api/espcontrol/{device_id}/pair"
    name = "api:espcontrol:pair"
    requires_auth = True

    def __init__(self, hass: HomeAssistant, pairing: PairingStore) -> None:
        self._hass = hass
        self._pairing = pairing

    async def post(self, request: web.Request, device_id: str) -> web.Response:
        """Issue a short-lived token; the long-lived HA token never leaves HA."""

        entry = next(
            (
                candidate
                for candidate in self._hass.config_entries.async_entries(DOMAIN)
                if candidate.data.get("device_id") == device_id
                and candidate.state is ConfigEntryState.LOADED
            ),
            None,
        )
        if entry is None:
            return web.json_response({"error": "unknown_device"}, status=HTTPStatus.NOT_FOUND)
        grant = self._pairing.issue(device_id)
        base_url = f"{request.scheme}://{request.host}"
        pairing_payload = base64.urlsafe_b64encode(
            json.dumps(
                {"baseUrl": base_url, "deviceId": device_id, "token": grant.token},
                separators=(",", ":"),
            ).encode()
        ).decode().rstrip("=")
        device_host = (
            entry.options.get(CONF_HOST, entry.data.get(CONF_HOST))
            if entry
            else None
        )
        return self.json(
            {
                "device_id": device_id,
                "token": grant.token,
                "expires_at": grant.expires_at,
                "header": PAIRING_TOKEN_HEADER,
                "catalog_path": f"/api/espcontrol/{device_id}/entities",
                "pairing_uri": (
                    f"http://{device_host}/#espcontrol-pairing={pairing_payload}"
                    if device_host
                    else None
                ),
            }
        )


class EspControlEntityView(HomeAssistantView):
    """Serve safe, filtered entity metadata to one paired display."""

    url = "/api/espcontrol/{device_id}/entities"
    name = "api:espcontrol:entities"
    requires_auth = False
    # Use Home Assistant's built-in CORS preflight handling. Defining an
    # explicit OPTIONS method conflicts with the handler registered by HA.
    cors_allowed = True

    def __init__(self, hass: HomeAssistant, pairing: PairingStore) -> None:
        self._hass = hass
        self._pairing = pairing

    def _allowed_origin(self, device_id: str, origin: str | None) -> str | None:
        if not origin:
            return None
        parsed = urlsplit(origin)
        if not parsed.hostname:
            return None
        for entry in self._hass.config_entries.async_entries(DOMAIN):
            if (
                entry.data.get("device_id") == device_id
                and entry.options.get(CONF_HOST, entry.data.get(CONF_HOST)) == parsed.hostname
            ):
                return origin
        return None

    async def get(self, request: web.Request, device_id: str) -> web.Response:
        """Return one catalogue page after validating the temporary grant."""

        authorization = request.headers.get(PAIRING_TOKEN_HEADER, "")
        token = authorization.removeprefix("Bearer ").strip()
        if not token:
            # Keep already-flashed POC firmware usable while it is updated to
            # the standard Authorization header used for CORS requests.
            token = request.headers.get("X-EspControl-Pairing-Token", "")
        if not self._pairing.validate(device_id, token):
            return web.json_response({"error": "invalid_pairing"}, status=HTTPStatus.UNAUTHORIZED)
        origin = self._allowed_origin(device_id, request.headers.get("Origin"))
        try:
            limit = int(request.query.get("limit", DEFAULT_CATALOG_LIMIT))
            cursor = int(request.query.get("cursor", "0"))
        except ValueError:
            return web.json_response({"error": "invalid_pagination"}, status=HTTPStatus.BAD_REQUEST)
        entities, next_cursor = build_entity_catalog(
            self._hass,
            query=request.query.get("q", "").strip(),
            field=request.query.get("field", "entity"),
            area=request.query.get("area") or None,
            include_hidden=request.query.get("include_hidden") == "1",
            include_disabled=request.query.get("include_disabled") == "1",
            limit=limit,
            cursor=cursor,
        )
        response = self.json(
            {
                "protocol": 1,
                "device_id": device_id,
                "entities": entities,
                "next_cursor": next_cursor,
            }
        )
        if origin:
            response.headers["Access-Control-Allow-Origin"] = origin
            response.headers["Vary"] = "Origin"
        return response


@callback
def register_views(hass: HomeAssistant, pairing: PairingStore) -> None:
    """Register views once during component setup."""

    hass.http.register_view(EspControlPairingView(hass, pairing))
    hass.http.register_view(EspControlEntityView(hass, pairing))
