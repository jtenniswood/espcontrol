"""LVGL Screenshot Component - serves a JPEG snapshot of the LVGL framebuffer via HTTP.

Wires the screensaver_wake script so that GET /screenshot and POST /touch
automatically wake the display before operating.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_PORT

CODEOWNERS = ["@dcgrove"]
DEPENDENCIES = ["lvgl"]

lvgl_screenshot_ns = cg.esphome_ns.namespace("lvgl_screenshot")
LvglScreenshot = lvgl_screenshot_ns.class_("LvglScreenshot", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(LvglScreenshot),
        cv.Optional(CONF_PORT, default=8080): cv.port,
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    # Force-enable LV_USE_SNAPSHOT so that lv_snapshot_take() and
    # lv_snapshot_take_to_draw_buf() are compiled into the LVGL library.
    # The sdkconfig option CONFIG_LV_USE_SNAPSHOT is not reliably propagated
    # to LVGL's internal config on all ESPHome/ESP-IDF combinations, so we
    # set it explicitly as a compile definition.
    cg.add_build_flag("-DLV_USE_SNAPSHOT=1")

    # Inject the header that declares the screensaver_wake script pointers
    # so that the lvgl_screenshot C++ code can call it directly to wake
    # the display. The header uses void* to avoid include-order issues
    # with the script namespace.
    cg.add_global(
        cg.RawStatement('#include "esphome/components/lvgl_screenshot/screen_wake_extern.h"'),
        prepend=True,
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
