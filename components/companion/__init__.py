"""Secure local Companion connection for the 4848S040 panel.

The component deliberately runs a small, separate TLS WebSocket server.  It
does not use Home Assistant's API connection and it never initiates a network
connection to the Mac.
"""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, network, socket
from esphome.const import CONF_ID, CONF_PORT

DEPENDENCIES = ["network"]
CODEOWNERS = ["@jtenniswood"]
MULTI_CONF = False

companion_ns = cg.esphome_ns.namespace("companion")
CompanionService = companion_ns.class_("CompanionService", cg.Component)


def _reserve_network_resources(config):
    """Reserve the listener and client sockets before ESPHome sizes lwIP."""
    socket.consume_sockets(
        1, "companion_secure_websocket", socket.SocketType.TCP_LISTEN
    )(config)
    socket.consume_sockets(2, "companion_secure_websocket")(config)
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(CompanionService),
            cv.Optional(CONF_PORT, default=8443): cv.port,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    cv.only_on_esp32,
    _reserve_network_resources,
)


async def to_code(config):
    esp32.add_idf_sdkconfig_option("CONFIG_HTTPD_WS_SUPPORT", True)
    esp32.add_idf_sdkconfig_option("CONFIG_ESP_HTTPS_SERVER_ENABLE", True)
    esp32.add_idf_sdkconfig_option("CONFIG_MBEDTLS_X509_CRT_WRITE_C", True)

    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_port(config[CONF_PORT]))
