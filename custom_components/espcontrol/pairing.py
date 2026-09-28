"""Short-lived pairing credentials for the EspControl proof of concept."""

from __future__ import annotations

import hashlib
import secrets
import time
from dataclasses import dataclass

from .const import PAIRING_TOKEN_TTL_SECONDS


@dataclass(frozen=True, slots=True)
class PairingGrant:
    """A token that expires and is scoped to one device."""

    token: str
    expires_at: int


class PairingStore:
    """In-memory grants; a restart revokes all POC grants by design."""

    def __init__(self) -> None:
        self._grants: dict[str, tuple[str, int]] = {}

    def issue(self, device_id: str, ttl: int = PAIRING_TOKEN_TTL_SECONDS) -> PairingGrant:
        token = secrets.token_urlsafe(32)
        expires_at = int(time.time()) + max(30, ttl)
        self._grants[device_id] = (hashlib.sha256(token.encode()).hexdigest(), expires_at)
        return PairingGrant(token, expires_at)

    def validate(self, device_id: str, token: str) -> bool:
        grant = self._grants.get(device_id)
        if grant is None:
            return False
        digest, expires_at = grant
        if expires_at <= int(time.time()):
            self._grants.pop(device_id, None)
            return False
        return secrets.compare_digest(digest, hashlib.sha256(token.encode()).hexdigest())

    def revoke(self, device_id: str) -> None:
        self._grants.pop(device_id, None)
