"""Offline ESP32-C6 recovery component for ESP32-P4 factory images."""

from __future__ import annotations

import hashlib
from pathlib import Path
import re

import esphome.codegen as cg
from esphome.components import esp32
from esphome.components.const import CONF_SHA256
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PATH, CONF_VERSION
from esphome.core import CORE, HexInt, ID


CODEOWNERS = ["@jtenniswood"]
AUTO_LOAD = ["sha256", "watchdog"]
DEPENDENCIES = ["esp32_hosted"]

VERSION_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")

c6_recovery_ns = cg.esphome_ns.namespace("c6_recovery")
C6RecoveryComponent = c6_recovery_ns.class_("C6RecoveryComponent", cg.Component)


def validate_sha256(value: str) -> str:
    value = cv.string_strict(value).lower()
    if len(value) != 64:
        raise cv.Invalid("SHA256 must be 64 hexadecimal characters")
    try:
        bytes.fromhex(value)
    except ValueError as exc:
        raise cv.Invalid(f"SHA256 must be valid hexadecimal: {exc}") from exc
    return value


def validate_version(value: str) -> str:
    value = cv.string_strict(value)
    if not VERSION_RE.fullmatch(value):
        raise cv.Invalid("version must use major.minor.patch format")
    return value


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(CONF_ID): cv.declare_id(C6RecoveryComponent),
            cv.Required(CONF_PATH): cv.file_,
            cv.Required(CONF_SHA256): validate_sha256,
            cv.Required(CONF_VERSION): validate_version,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    esp32.only_on_variant(supported=[esp32.VARIANT_ESP32P4]),
)


def final_validate(config: dict) -> None:
    path = CORE.relative_config_path(config[CONF_PATH])
    with path.open("rb") as handle:
        firmware = handle.read()
    actual = hashlib.sha256(firmware).hexdigest()
    if actual != config[CONF_SHA256]:
        raise cv.Invalid(
            f"SHA256 mismatch for {config[CONF_PATH]}: "
            f"expected {config[CONF_SHA256]}, got {actual}"
        )


FINAL_VALIDATE_SCHEMA = final_validate


async def to_code(config: dict) -> None:
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    path = Path(CORE.relative_config_path(config[CONF_PATH]))
    firmware = path.read_bytes()
    array_id = ID(
        f"{config[CONF_ID]}_firmware_data", is_declaration=True, type=cg.uint8
    )
    firmware_array = cg.progmem_array(array_id, [HexInt(byte) for byte in firmware])

    cg.add(var.set_firmware_data(firmware_array))
    cg.add(var.set_firmware_size(len(firmware)))
    cg.add(var.set_target_version(config[CONF_VERSION]))
    cg.add(
        var.set_firmware_sha256(
            [HexInt(byte) for byte in bytes.fromhex(config[CONF_SHA256])]
        )
    )
