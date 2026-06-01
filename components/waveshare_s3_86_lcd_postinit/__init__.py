import esphome.codegen as cg
from esphome.components import pca9554
import esphome.config_validation as cv
from esphome.const import CONF_ID

CONF_PCA9554_ID = "pca9554_id"

DEPENDENCIES = ["pca9554"]

waveshare_s3_86_lcd_postinit_ns = cg.esphome_ns.namespace("waveshare_s3_86_lcd_postinit")
WaveshareS386LcdPostinit = waveshare_s3_86_lcd_postinit_ns.class_(
    "WaveshareS386LcdPostinit",
    cg.Component,
)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(WaveshareS386LcdPostinit),
            cv.Required(CONF_PCA9554_ID): cv.use_id(pca9554.PCA9554Component),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    expander = await cg.get_variable(config[CONF_PCA9554_ID])
    cg.add(var.set_pca9554(expander))
