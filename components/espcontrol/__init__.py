"""ESPHome external component stub for espcontrol.

Registers this directory as an include path so public C++ headers
(button_grid.h, clock_bar.h, icons.h, sun_calc.h, temperature_unit.h) are available to
lambdas in device YAML configs. button_grid.h is the compatibility facade for
the smaller button_grid_*.h implementation headers.
No YAML schema — all config is handled by the YAML packages.
"""
import esphome.codegen as cg
from esphome.components import wifi
from esphome.components.esp32 import VARIANT_ESP32S3, get_esp32_variant
import esphome.config_validation as cv
import os

CODEOWNERS = ["@jtenniswood"]

CONF_ACTION_RESPONSES = "action_responses"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_ACTION_RESPONSES, default=True): cv.boolean,
    }
)


async def to_code(config):
    # Power Mode uses ESPHome's balanced runtime request/release API so Normal
    # can remain responsive while Battery Saver restores WiFi modem sleep.
    wifi.enable_runtime_power_save_control()

    # ESPHome's native ESP-IDF generator only forwards -D and -W entries from
    # esphome.build_flags. Route this required S3 compiler option through the
    # dedicated C++ flag channel as well so generated main.cpp receives it.
    if get_esp32_variant() == VARIANT_ESP32S3:
        cg.add_cxx_build_flag("-mtext-section-literals")

    comp_dir = os.path.dirname(os.path.abspath(__file__))
    comp_include_dir = comp_dir.replace("\\", "/")
    cg.add_build_flag(f"-I{comp_dir}")
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/clock_bar.h"'), prepend=True)
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/backlight.h"'), prepend=True)
    cg.add_global(cg.RawStatement(f'#include "{comp_include_dir}/cover_art.h"'), prepend=True)
    if config[CONF_ACTION_RESPONSES]:
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON")
