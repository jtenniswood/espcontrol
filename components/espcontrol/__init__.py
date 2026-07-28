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

# config_api.h includes json_util.h to parse request bodies. web_server v3 already
# pulls json in transitively, but depending on that is fragile - declare it.
AUTO_LOAD = ["json"]

CONF_ACTION_RESPONSES = "action_responses"
CONF_CONFIG_API_MAX_BODY = "config_api_max_body"

espcontrol_ns = cg.global_ns.namespace("espcontrol")
EspControlApp = espcontrol_ns.class_("EspControlApp", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(EspControlApp),
        cv.Optional(CONF_ACTION_RESPONSES, default=True): cv.boolean,
        # Largest request body the config API will read. The default covers every
        # shipping profile - the widest (20 slots, 8 subpage chunks) tops out
        # around 20 KB - and is here as an escape hatch, not a per-device knob.
        # See the ESPCONTROL_CONFIG_API_MAX_BODY comment in config_api.h.
        cv.Optional(CONF_CONFIG_API_MAX_BODY, default=32768): cv.int_range(
            min=1024, max=131072
        ),
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
    # config_api.h is NOT added as a global include: ESPHome already copies every
    # component header into the build tree and includes it from src/esphome.h.
    # Adding it here too would include it a second time under a different path,
    # which defeats the include guard.
    cg.add_define("ESPCONTROL_CONFIG_API_MAX_BODY", config[CONF_CONFIG_API_MAX_BODY])
    if config[CONF_ACTION_RESPONSES]:
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON")
