"""ESPHome external component stub for espcontrol.

Registers this directory as an include path so public C++ headers
(button_grid.h, icons.h, sun_calc.h, temperature_unit.h) are available to
lambdas in device YAML configs. button_grid.h is the compatibility facade for
the smaller button_grid_*.h implementation headers.
No YAML schema — all config is handled by the YAML packages.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.core import coroutine_with_priority
import os

CODEOWNERS = ["@jtenniswood"]

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    comp_dir = os.path.dirname(os.path.abspath(__file__))
    cg.add_build_flag(f"-I{comp_dir}")
    cg.add_build_flag("-DUSE_API_USER_DEFINED_ACTIONS")
    cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
    cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON")
