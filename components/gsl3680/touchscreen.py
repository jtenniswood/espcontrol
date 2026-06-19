"""ESPHome code-generation wrapper for the GSL3680 touchscreen.

This validates the YAML pins and connects them to the C++ driver that performs
the actual I2C setup, firmware load, and touch reporting.
"""

from esphome import pins
import esphome.codegen as cg
from esphome.components import i2c, touchscreen
import esphome.config_validation as cv
from esphome.const import (
    CONF_ID,
    CONF_INTERRUPT_PIN,
    CONF_RESET_PIN,
)

ns_ = cg.esphome_ns.namespace("gsl3680")

cls_ = ns_.class_(
    "GSL3680",
    touchscreen.Touchscreen,
    i2c.I2CDevice,
)

CONFIG_SCHEMA = (
    touchscreen.touchscreen_schema()
    .extend(
        {
            cv.GenerateID(): cv.declare_id(cls_),
            cv.Required(CONF_INTERRUPT_PIN): pins.internal_gpio_input_pin_schema,
            cv.Required(CONF_RESET_PIN): pins.internal_gpio_output_pin_schema,
        }
    )
    .extend(i2c.i2c_device_schema(0x40))
)


async def to_code(config):
    # Build the C++ object and attach both ESPHome base classes: touchscreen for
    # event reporting and I2CDevice for register access.
    var = cg.new_Pvariable(config[CONF_ID])
    await touchscreen.register_touchscreen(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_interrupt_pin(await cg.gpio_pin_expression(config[CONF_INTERRUPT_PIN])))
    cg.add(var.set_reset_pin(await cg.gpio_pin_expression(config[CONF_RESET_PIN])))
