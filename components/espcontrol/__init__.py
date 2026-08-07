"""ESPHome external component stub for espcontrol.

Registers the central EspControlApp component and this directory as an include
path so public C++ compatibility headers remain available to device YAML.
EspControlApp owns long-lived firmware services while YAML continues to supply
device-specific wiring.
"""
import esphome.codegen as cg
from esphome.components.esp32 import VARIANT_ESP32S3, get_esp32_variant
import esphome.config_validation as cv
from esphome.const import CONF_ID
import os

CODEOWNERS = ["@jtenniswood"]
AUTO_LOAD = ["card_image_store"]

CONF_ACTION_RESPONSES = "action_responses"

espcontrol_ns = cg.global_ns.namespace("espcontrol")
EspControlApp = espcontrol_ns.class_("EspControlApp", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(EspControlApp),
        cv.Optional(CONF_ACTION_RESPONSES, default=True): cv.boolean,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # ESPHome's native ESP-IDF generator only forwards -D and -W entries from
    # esphome.build_flags. Route this required S3 compiler option through the
    # dedicated C++ flag channel as well so generated main.cpp receives it.
    if get_esp32_variant() == VARIANT_ESP32S3:
        cg.add_cxx_build_flag("-mtext-section-literals")

    comp_dir = os.path.dirname(os.path.abspath(__file__))
    comp_include_dir = comp_dir.replace("\\", "/")
    cg.add_global(
        cg.RawStatement('#include "esphome/components/espcontrol/espcontrol_app.h"'),
        prepend=True,
    )
    cg.add_build_flag(f"-I{comp_dir}")
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/clock_bar.h"'), prepend=True)
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/backlight.h"'), prepend=True)
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/cover_art.h"'), prepend=True)
    if config[CONF_ACTION_RESPONSES]:
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON")
