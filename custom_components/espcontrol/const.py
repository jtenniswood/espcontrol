"""Constants for the EspControl proof-of-concept integration."""

from __future__ import annotations

from dataclasses import dataclass

DOMAIN = "espcontrol"
CONF_DEVICE_ID = "device_id"
CONF_HOST = "host"
CONF_PORT = "port"
CONF_WEB_PORT = "web_port"
CONF_MODEL = "model"
CONF_PROTOCOL = "protocol"
CONF_MAC = "mac"

DEFAULT_PORT = 6053
PROTOCOL_VERSION = 1
PAIRING_TOKEN_HEADER = "Authorization"
PAIRING_TOKEN_TTL_SECONDS = 600
MAX_CATALOG_LIMIT = 100
DEFAULT_CATALOG_LIMIT = 50

SERVICE_CREATE_PAIRING_TOKEN = "create_pairing_token"


@dataclass(frozen=True, slots=True)
class EntityRule:
    """The Home Assistant entities accepted by one EspControl field."""

    domains: frozenset[str] | None = None
    capabilities: frozenset[str] = frozenset()
    allow_any_domain: bool = False


# This is deliberately field-oriented rather than card-oriented. Every entity
# reference in the configurator can point at one of these rules, including
# secondary media, action, cover and subpage references.
ENTITY_RULES: dict[str, EntityRule] = {
    "entity": EntityRule(allow_any_domain=True),
    "state_entity": EntityRule(allow_any_domain=True),
    "sensor": EntityRule(
        frozenset({"sensor", "binary_sensor", "text_sensor", "input_number"})
    ),
    "binary_sensor": EntityRule(frozenset({"binary_sensor"})),
    "text_sensor": EntityRule(frozenset({"text_sensor"})),
    "light": EntityRule(frozenset({"light"})),
    "fan": EntityRule(frozenset({"fan"})),
    "climate": EntityRule(frozenset({"climate"})),
    "cover": EntityRule(frozenset({"cover"})),
    "lock": EntityRule(frozenset({"lock"})),
    "alarm": EntityRule(frozenset({"alarm_control_panel"})),
    "media_player": EntityRule(frozenset({"media_player"})),
    "vacuum": EntityRule(frozenset({"vacuum"})),
    "lawn_mower": EntityRule(frozenset({"lawn_mower"})),
    "weather": EntityRule(frozenset({"weather"})),
    "camera": EntityRule(frozenset({"camera", "image"})),
    "select": EntityRule(frozenset({"select", "input_select"})),
    "number": EntityRule(frozenset({"number", "input_number"})),
    "switch": EntityRule(frozenset({"switch", "input_boolean"})),
    "person": EntityRule(frozenset({"person"})),
    "device_tracker": EntityRule(frozenset({"device_tracker"})),
    "scene": EntityRule(frozenset({"scene"})),
    "script": EntityRule(frozenset({"script"})),
    "automation": EntityRule(frozenset({"automation"})),
    "button": EntityRule(frozenset({"button", "input_button"})),
    "action": EntityRule(
        frozenset(
            {
                "scene",
                "script",
                "automation",
                "button",
                "input_button",
                "input_boolean",
                "number",
                "input_number",
                "select",
                "input_select",
            }
        )
    ),
}
