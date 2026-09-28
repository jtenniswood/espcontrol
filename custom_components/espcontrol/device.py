"""Small HTTP probe for the existing EspControl web API."""

from __future__ import annotations

from dataclasses import dataclass

from aiohttp import ClientSession, ClientTimeout


@dataclass(slots=True)
class EspControlRuntime:
    """Runtime state kept per config entry."""

    host: str
    web_port: int = 80
    available: bool = False
    identity: dict[str, object] | None = None

    async def async_probe(self, session: ClientSession) -> bool:
        """Probe identity without failing setup when the display is offline."""

        url = f"http://{self.host}:{self.web_port}/api/v1/identity"
        try:
            async with session.get(url, timeout=ClientTimeout(total=3)) as response:
                if response.status != 200:
                    self.available = False
                    return False
                payload = await response.json(content_type=None)
        except Exception:  # noqa: BLE001 - an offline LAN device is expected.
            self.available = False
            return False
        if not isinstance(payload, dict):
            self.available = False
            return False
        self.identity = payload
        self.available = True
        return True
