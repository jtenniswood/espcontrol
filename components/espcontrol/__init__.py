"""ESPHome external component stub for espcontrol.

Registers the early panel identity component, central EspControlApp component,
and include path for compatibility headers used by device YAML.
EspControlApp owns long-lived firmware services while YAML continues to supply
device-specific wiring.
"""
import esphome.codegen as cg
from esphome.components.esp32 import VARIANT_ESP32S3, get_esp32_variant
from esphome.components import text
import esphome.config_validation as cv
from esphome.const import CONF_ID
from esphome.core import CORE, CoroPriority, coroutine_with_priority
import os

CODEOWNERS = ["@jtenniswood"]
AUTO_LOAD = ["card_image_store", "mdns", "json"]

CONF_ACTION_RESPONSES = "action_responses"
CONF_PANEL_CONFIG = "panel_config"
CONF_DEVICE_PROFILE = "device_profile"
CONF_BUTTON_ORDER = "button_order"
CONF_BUTTON_ON_COLOR = "button_on_color"
CONF_BUTTONS = "buttons"
CONF_CONFIG = "config"
CONF_SUBPAGE_CHUNKS = "subpage_chunks"
CONF_STORAGE = "storage"
CONF_WEB_AUTH_USERNAME = "web_auth_username"
CONF_WEB_AUTH_PASSWORD = "web_auth_password"

espcontrol_ns = cg.global_ns.namespace("espcontrol")
EspControlApp = espcontrol_ns.class_("EspControlApp", cg.Component)
PanelIdentity = espcontrol_ns.class_("PanelIdentity", cg.Component)

PANEL_CONFIG_BUTTON_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_CONFIG): cv.use_id(text.Text),
        cv.Required(CONF_SUBPAGE_CHUNKS): cv.All(
            cv.ensure_list(cv.use_id(text.Text)), cv.Length(min=4, max=8)
        ),
    }
)

PANEL_CONFIG_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_DEVICE_PROFILE): cv.string_strict,
        cv.Required(CONF_BUTTON_ORDER): cv.use_id(text.Text),
        cv.Optional(CONF_BUTTON_ON_COLOR): cv.use_id(text.Text),
        cv.Required(CONF_BUTTONS): cv.All(
            cv.ensure_list(PANEL_CONFIG_BUTTON_SCHEMA), cv.Length(min=1, max=32)
        ),
        cv.Optional(CONF_STORAGE, default="nvs"): cv.one_of(
            "nvs", "card_images", lower=True
        ),
    }
)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(CONF_ID): cv.declare_id(EspControlApp),
        cv.GenerateID("identity_id"): cv.declare_id(PanelIdentity),
        cv.Optional(CONF_ACTION_RESPONSES, default=True): cv.boolean,
        cv.Optional(CONF_PANEL_CONFIG): PANEL_CONFIG_SCHEMA,
        cv.Optional(CONF_WEB_AUTH_USERNAME, default=""): cv.string_strict,
        cv.Optional(CONF_WEB_AUTH_PASSWORD, default=""): cv.sensitive(cv.string_strict),
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(CoroPriority.DIAGNOSTICS)
async def to_code(config):
    # Run before safe_mode's early return and before any preference consumers.
    if CORE.config.get("preferences", {}).get("rtc_storage", False):
        raise cv.Invalid("EspControl reset requires flash preferences; remove preferences.rtc_storage")
    cg.add_define("USE_OTA_STATE_LISTENER")
    for operation in ("begin", "end", "abort"):
        cg.add_build_flag(f"-Wl,--wrap=esp_ota_{operation}")
    if "esp32_hosted" in CORE.config:
        cg.add_build_flag("-Wl,--wrap=esp_hosted_slave_ota_begin")
    cg.add_global(cg.RawStatement('#include "esphome/components/espcontrol/device_reset.h"'), prepend=True)
    compiled_networks = bool(CORE.config.get("wifi", {}).get("networks", []))
    # Directly configured ESPHome web authentication must protect our native
    # endpoints too, even when the convenience auth add-on was not included.
    web_auth = CORE.config.get("web_server", {}).get("auth", {})
    username = web_auth.get("username", config[CONF_WEB_AUTH_USERNAME])
    password = web_auth.get("password", config[CONF_WEB_AUTH_PASSWORD])
    cg.add(espcontrol_ns.namespace("reset").early_startup(
        compiled_networks, username, password))
    identity = cg.new_Pvariable(config["identity_id"])
    await cg.register_component(identity, config)
    cg.add(identity.set_web_auth_credentials(username, password))
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_web_auth_credentials(username, password))

    panel_config = config.get(CONF_PANEL_CONFIG)
    if panel_config is not None:
        if panel_config[CONF_STORAGE] == "card_images":
            cg.add(var.set_panel_config_card_images_storage(True))
        cg.add(var.set_panel_config_device_profile(panel_config[CONF_DEVICE_PROFILE]))
        button_order = await cg.get_variable(panel_config[CONF_BUTTON_ORDER])
        cg.add(var.set_panel_config_button_order(button_order))
        if CONF_BUTTON_ON_COLOR in panel_config:
            button_on_color = await cg.get_variable(panel_config[CONF_BUTTON_ON_COLOR])
            cg.add(var.set_panel_config_button_on_color(button_on_color))
        for slot, button_sources in enumerate(panel_config[CONF_BUTTONS], start=1):
            button = await cg.get_variable(button_sources[CONF_CONFIG])
            subpages = [
                await cg.get_variable(source)
                for source in button_sources[CONF_SUBPAGE_CHUNKS]
            ]
            cg.add(var.set_panel_config_button(slot, button, *subpages))

    for update_config in CORE.config.get("update", []):
        update_entity = await cg.get_variable(update_config[CONF_ID])
        cg.add(espcontrol_ns.namespace("reset").watch_update(update_entity))

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
