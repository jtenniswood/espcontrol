"""Home Assistant media-source websocket client.

Browsing a Home Assistant media-source folder is a websocket-only API, so this
component keeps an authenticated websocket connection to Home Assistant and turns
a folder into an ordered list of downloadable photo URLs for the photo
screensaver. It is only built for the ESP-IDF framework, which provides
esp_websocket_client.
"""

from esphome import automation
from esphome.components import esp32
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_URL

CODEOWNERS = ["@jtenniswood"]
DEPENDENCIES = ["esp32", "network"]
AUTO_LOAD = ["json"]

CONF_TOKEN = "token"

# esp_websocket_client is not part of the base ESP-IDF install; pull the managed
# component from the Espressif component registry. A range lets the IDF component
# manager resolve a build compatible with the ESP-IDF version ESPHome bundles,
# across the supported P4 and S3 targets.
ESP_WEBSOCKET_CLIENT_REF = ">=1.2.0"

ha_media_source_ns = cg.esphome_ns.namespace("ha_media_source")
HaMediaSource = ha_media_source_ns.class_("HaMediaSource", cg.Component)

CONF_FOLDER = "folder"

CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HaMediaSource),
            cv.Optional(CONF_FOLDER, default=""): cv.string,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_with_framework("esp-idf"),
)


async def to_code(config):
    esp32.add_idf_component(
        name="espressif/esp_websocket_client",
        ref=ESP_WEBSOCKET_CLIENT_REF,
    )

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if config[CONF_FOLDER]:
        cg.add(var.set_folder(config[CONF_FOLDER]))


# ── Actions ──────────────────────────────────────────────────────────────
# set_connection wires the home_assistant_url / home_assistant_token text
# entities to the client; set_folder points it at a media-source folder;
# refresh re-browses on demand.

SET_CONNECTION_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(HaMediaSource),
        cv.Required(CONF_URL): cv.templatable(cv.string),
        cv.Required(CONF_TOKEN): cv.templatable(cv.string),
    }
)

SetConnectionAction = ha_media_source_ns.class_("SetConnectionAction", automation.Action)
SetFolderAction = ha_media_source_ns.class_("SetFolderAction", automation.Action)
RefreshAction = ha_media_source_ns.class_("RefreshAction", automation.Action)


@automation.register_action(
    "ha_media_source.set_connection",
    SetConnectionAction,
    SET_CONNECTION_ACTION_SCHEMA,
    synchronous=True,
)
async def set_connection_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    url = await cg.templatable(config[CONF_URL], args, cg.std_string)
    cg.add(var.set_url(url))
    token = await cg.templatable(config[CONF_TOKEN], args, cg.std_string)
    cg.add(var.set_token(token))
    return var


SET_FOLDER_ACTION_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(HaMediaSource),
        cv.Required(CONF_FOLDER): cv.templatable(cv.string),
    }
)


@automation.register_action(
    "ha_media_source.set_folder", SetFolderAction, SET_FOLDER_ACTION_SCHEMA, synchronous=True
)
async def set_folder_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    folder = await cg.templatable(config[CONF_FOLDER], args, cg.std_string)
    cg.add(var.set_folder(folder))
    return var


REFRESH_ACTION_SCHEMA = automation.maybe_simple_id(
    {cv.GenerateID(): cv.use_id(HaMediaSource)}
)


@automation.register_action(
    "ha_media_source.refresh", RefreshAction, REFRESH_ACTION_SCHEMA, synchronous=True
)
async def refresh_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    await cg.register_parented(var, config[CONF_ID])
    return var
