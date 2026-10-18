from esphome.components.esp32 import (
    add_idf_sdkconfig_option,
    include_builtin_idf_component,
)
import esphome.config_validation as cv

CODEOWNERS = ["@dentra"]

CONFIG_SCHEMA = cv.All(
    cv.Schema({}),
    cv.only_on_esp32,
)

AUTO_LOAD = ["web_server"]


def _web_ota_source_filter(previous):
    # Keep the upstream schema/header and replace only the implementation that
    # lacks AsyncWebHandler's explicit cancellation hook.
    def excluded_sources():
        return [*(previous() if previous else []), "ota_web_server.cpp"]

    return excluded_sources


async def to_code(config):
    from esphome.components.web_server import ota as web_server_ota

    web_server_ota.FILTER_SOURCE_FILES = _web_ota_source_filter(
        getattr(web_server_ota, "FILTER_SOURCE_FILES", None)
    )
    # ESPHome excludes these IDF components from native builds unless a
    # component explicitly requests them. This component includes their
    # headers directly and links against their implementations.
    include_builtin_idf_component("esp_http_server")
    include_builtin_idf_component("esp-tls")

    # Increase the maximum supported size of headers section in HTTP request packet to be processed by the server
    add_idf_sdkconfig_option("CONFIG_HTTPD_MAX_REQ_HDR_LEN", 4096)
