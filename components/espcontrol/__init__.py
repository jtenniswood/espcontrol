"""ESPHome external component stub for espcontrol.

Registers this directory as an include path so public C++ headers
(button_grid.h, icons.h, sun_calc.h, temperature_unit.h) are available to
lambdas in device YAML configs. button_grid.h is the compatibility facade for
the smaller button_grid_*.h implementation headers.
No YAML schema — all config is handled by the YAML packages.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
import os

# Enable LV_USE_CHART in the generated lv_conf.h so that sensor_chart cards
# can create lv_chart sparkline widgets at runtime.  This must run at import
# time (before any to_code() is called) so that the LVGL component sees the
# flag when it writes lv_conf.h.
try:
    from esphome.components.lvgl.helpers import add_lv_use
    add_lv_use("CHART")
except Exception:
    pass

CODEOWNERS = ["@jtenniswood"]

CONF_ACTION_RESPONSES = "action_responses"

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_ACTION_RESPONSES, default=True): cv.boolean,
    }
)


async def to_code(config):
    comp_dir = os.path.dirname(os.path.abspath(__file__))
    cg.add_build_flag(f"-I{comp_dir}")
    if config[CONF_ACTION_RESPONSES]:
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES")
        cg.add_define("USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON")
