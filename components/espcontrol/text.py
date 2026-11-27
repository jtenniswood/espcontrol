"""Private layout fields for the native PanelConfig renderer."""

from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_INITIAL_VALUE, CONF_ON_VALUE

from . import PanelConfigField

DEPENDENCIES = ["espcontrol"]

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(PanelConfigField),
        cv.Optional(CONF_INITIAL_VALUE): cv.string_strict,
        cv.Optional(CONF_ON_VALUE): automation.validate_automation(),
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if initial_value := config.get(CONF_INITIAL_VALUE):
        cg.add(var.set_initial_value(initial_value))
    for trigger in config.get(CONF_ON_VALUE, []):
        await automation.build_callback_automation(
            var, "add_on_state_callback", [(cg.std_string, "x")], trigger
        )
