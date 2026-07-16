import logging
import os

from esphome import automation
from esphome.components import esp32
import esphome.codegen as cg
from esphome.components.const import CONF_BYTE_ORDER, CONF_REQUEST_HEADERS
from esphome.components.http_request import CONF_HTTP_REQUEST_ID, HttpRequestComponent
from esphome.components.image import (
    IMAGE_TYPE,
    Image_,
    get_image_type_enum,
    get_transparency_enum,
    validate_settings,
    validate_transparency,
    validate_type,
)
import esphome.config_validation as cv
from esphome.const import (
    CONF_BUFFER_SIZE,
    CONF_FORMAT,
    CONF_ID,
    CONF_ON_ERROR,
    CONF_RESIZE,
    CONF_TYPE,
    CONF_URL,
)
from esphome.core import Lambda

try:
    from esphome.components.lvgl import defines as lvgl_defines
except ImportError:
    lvgl_defines = None

try:
    from esphome.components.image import add_metadata
except ImportError:

    def add_metadata(*args, **kwargs):
        pass

AUTO_LOAD = ["image", "socket"]
DEPENDENCIES = ["display", "http_request"]
CODEOWNERS = ["@jtenniswood"]
MULTI_CONF = True

CONF_ON_DOWNLOAD_FINISHED = "on_download_finished"
CONF_ALLOW_INSECURE_LOCAL_URLS = "allow_insecure_local_urls"
CONF_PLACEHOLDER = "placeholder"
CONF_TRANSPARENCY = "transparency"
CONF_UPDATE = "update"
CONF_RESIZE_MODE = "resize_mode"
CONF_PRIORITY = "priority"
CONF_HARDWARE_ACCELERATION = "hardware_acceleration"
CONF_P4_PIPELINE = "p4_pipeline"

_LOGGER = logging.getLogger(__name__)

artwork_image_ns = cg.esphome_ns.namespace("artwork_image")

ImageFormat = artwork_image_ns.enum("ImageFormat")
ImageResizeMode = artwork_image_ns.enum("ImageResizeMode")
ImageRequestPriority = artwork_image_ns.enum("ImageRequestPriority", is_class=True)
P4PipelinePriority = artwork_image_ns.enum("P4PipelinePriority")


class Format:
    def __init__(self, image_type):
        self.image_type = image_type

    @property
    def enum(self):
        return getattr(ImageFormat, self.image_type)

    def actions(self):
        pass


class JPEGFormat(Format):
    def __init__(self):
        super().__init__("JPEG")

    def actions(self):
        cg.add_define("USE_ARTWORK_IMAGE_JPEG_SUPPORT")
        import shutil
        from esphome.core import CORE

        # Copy libjpeg-turbo as an IDF component into the build directory.
        # Skip only when the generated copy is newer than all source files.
        src_path = os.path.join(
            os.path.dirname(__file__), "..", "libjpeg-turbo-esp32"
        )
        dest_path = str(
            CORE.relative_build_path("components", "libjpeg-turbo-esp32")
        )
        required_files = (
            "CMakeLists.txt",
            os.path.join("src", "jdapimin.c"),
            os.path.join("src", "wrapper", "jdapistd-8.c"),
        )
        src_cmake = os.path.join(src_path, "CMakeLists.txt")
        dest_cmake = os.path.join(dest_path, "CMakeLists.txt")

        def newest_mtime(path):
            newest = 0
            if os.path.isfile(path):
                return os.path.getmtime(path)
            for dirpath, _, filenames in os.walk(path):
                for filename in filenames:
                    newest = max(newest, os.path.getmtime(os.path.join(dirpath, filename)))
            return newest

        missing_required_file = any(
            not os.path.exists(os.path.join(dest_path, file_name))
            for file_name in required_files
        )
        needs_copy = missing_required_file or (
            os.path.exists(dest_cmake)
            and newest_mtime(src_path) > newest_mtime(dest_path)
        )
        if needs_copy:
            if os.path.exists(dest_path):
                shutil.rmtree(dest_path)
            shutil.copytree(src_path, dest_path)


class BMPFormat(Format):
    def __init__(self):
        super().__init__("BMP")

    def actions(self):
        cg.add_define("USE_ARTWORK_IMAGE_BMP_SUPPORT")


class PNGFormat(Format):
    def __init__(self):
        super().__init__("PNG")

    def actions(self):
        cg.add_define("USE_ARTWORK_IMAGE_PNG_SUPPORT")
        cg.add_library("pngle", "1.1.0")


class AutoFormat(Format):
    def __init__(self):
        super().__init__("AUTO")

    def actions(self):
        JPEGFormat().actions()
        PNGFormat().actions()
        BMPFormat().actions()


IMAGE_FORMATS = {
    x.image_type: x
    for x in (
        AutoFormat(),
        BMPFormat(),
        JPEGFormat(),
        PNGFormat(),
    )
}
IMAGE_FORMATS.update({"JPG": IMAGE_FORMATS["JPEG"]})

ArtworkImage = artwork_image_ns.class_("ArtworkImage", cg.PollingComponent, Image_)

# Actions
SetUrlAction = artwork_image_ns.class_(
    "ArtworkImageSetUrlAction", automation.Action, cg.Parented.template(ArtworkImage)
)
ReleaseImageAction = artwork_image_ns.class_(
    "ArtworkImageReleaseAction", automation.Action, cg.Parented.template(ArtworkImage)
)

ARTWORK_IMAGE_SCHEMA = (
    cv.Schema(
        {
            cv.Required(CONF_ID): cv.declare_id(ArtworkImage),
            cv.Required(CONF_TYPE): validate_type(IMAGE_TYPE),
            cv.Optional(CONF_RESIZE): cv.dimensions,
            cv.Optional(CONF_RESIZE_MODE, default="FIT"): cv.one_of(
                "FIT", "COVER", upper=True
            ),
            cv.Optional(CONF_PRIORITY, default="BACKGROUND"): cv.one_of(
                "BACKGROUND", "TILE", "COVER_ART", "MODAL", upper=True
            ),
            cv.Optional(CONF_BYTE_ORDER): cv.one_of(
                "BIG_ENDIAN", "LITTLE_ENDIAN", upper=True
            ),
            cv.Optional(CONF_TRANSPARENCY, default="OPAQUE"): validate_transparency(),
            cv.GenerateID(CONF_HTTP_REQUEST_ID): cv.use_id(HttpRequestComponent),
            cv.Required(CONF_URL): cv.url,
            cv.Optional(CONF_REQUEST_HEADERS): cv.All(
                cv.Schema({cv.string: cv.templatable(cv.string)})
            ),
            cv.Optional(CONF_FORMAT, default="AUTO"): cv.one_of(
                *IMAGE_FORMATS, upper=True
            ),
            cv.Optional(CONF_PLACEHOLDER): cv.use_id(Image_),
            cv.Optional(CONF_BUFFER_SIZE, default=65536): cv.int_range(256, 524288),
            cv.Optional(CONF_ALLOW_INSECURE_LOCAL_URLS, default=False): cv.boolean,
            cv.Optional(CONF_HARDWARE_ACCELERATION, default=True): cv.boolean,
            cv.Optional(CONF_P4_PIPELINE, default="DISABLED"): cv.one_of(
                "DISABLED", "TILE", "MODAL", upper=True
            ),
            cv.Optional(CONF_ON_DOWNLOAD_FINISHED): automation.validate_automation({}),
            cv.Optional(CONF_ON_ERROR): automation.validate_automation({}),
        }
    )
    .extend(cv.polling_component_schema("never"))
)


_SOCKET_RESERVED = False


def _consume_sockets(config):
    """Reserve the one outbound socket owned by the shared image service."""
    global _SOCKET_RESERVED
    if _SOCKET_RESERVED:
        return config
    try:
        from esphome.components import socket
    except ImportError:
        return config

    consume_sockets = getattr(socket, "consume_sockets", None)
    if consume_sockets is not None:
        consume_sockets(1, "artwork_image_service")(config)
        _SOCKET_RESERVED = True
    return config


CONFIG_SCHEMA = cv.Schema(
    cv.All(
        ARTWORK_IMAGE_SCHEMA,
        _consume_sockets,
        cv.require_framework_version(
            # esp8266 not supported yet; if enabled in the future, minimum version of 2.7.0 is needed
            # esp8266_arduino=cv.Version(2, 7, 0),
            esp32_arduino=cv.Version(0, 0, 0),
            esp_idf=cv.Version(4, 0, 0),
            rp2040_arduino=cv.Version(0, 0, 0),
            host=cv.Version(0, 0, 0),
        ),
        validate_settings,
    )
)

SET_URL_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.use_id(ArtworkImage),
        cv.Required(CONF_URL): cv.templatable(cv.url),
        cv.Optional(CONF_UPDATE, default=True): cv.templatable(bool),
    }
)

RELEASE_IMAGE_SCHEMA = automation.maybe_simple_id(
    {
        cv.GenerateID(): cv.use_id(ArtworkImage),
    }
)


_CALLBACK_AUTOMATIONS = (
    automation.CallbackAutomation(
        CONF_ON_DOWNLOAD_FINISHED, "add_on_finished_callback", [(bool, "cached")]
    ),
    automation.CallbackAutomation(CONF_ON_ERROR, "add_on_error_callback"),
)


@automation.register_action(
    "artwork_image.set_url", SetUrlAction, SET_URL_SCHEMA, synchronous=True
)
@automation.register_action(
    "artwork_image.release", ReleaseImageAction, RELEASE_IMAGE_SCHEMA, synchronous=True
)
async def artwork_image_action_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)

    if CONF_URL in config:
        template_ = await cg.templatable(config[CONF_URL], args, cg.std_string)
        cg.add(var.set_url(template_))
    if CONF_UPDATE in config:
        template_ = await cg.templatable(config[CONF_UPDATE], args, bool)
        cg.add(var.set_update(template_))
    return var


async def to_code(config):
    image_format = IMAGE_FORMATS[config[CONF_FORMAT]]
    image_format.actions()
    if config[CONF_ALLOW_INSECURE_LOCAL_URLS]:
        cg.add_define("USE_ARTWORK_IMAGE_INSECURE_LOCAL_URLS")
        try:
            from esphome.core import CORE

            if CORE.is_esp32 and not CORE.using_arduino:
                esp32.add_idf_sdkconfig_option("CONFIG_ESP_TLS_INSECURE", True)
                esp32.add_idf_sdkconfig_option(
                    "CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY", True
                )
        except Exception as err:
            _LOGGER.debug("Could not enable ESP-IDF insecure TLS options: %s", err)
    if lvgl_defines is not None:
        lvgl_defines.add_define("LV_DRAW_SW_SUPPORT_RGB565", "1")
        lvgl_defines.add_define("LV_DRAW_SW_SUPPORT_RGB565A8", "1")

    url = config[CONF_URL]
    width, height = config.get(CONF_RESIZE, (0, 0))
    transparent = get_transparency_enum(config[CONF_TRANSPARENCY])
    add_metadata(
        config[CONF_ID],
        width,
        height,
        config[CONF_TYPE],
        config[CONF_TRANSPARENCY],
    )

    var = cg.new_Pvariable(
        config[CONF_ID],
        url,
        width,
        height,
        image_format.enum,
        getattr(ImageResizeMode, config[CONF_RESIZE_MODE]),
        get_image_type_enum(config[CONF_TYPE]),
        transparent,
        config[CONF_BUFFER_SIZE],
        config.get(CONF_BYTE_ORDER) != "LITTLE_ENDIAN",
        config[CONF_ALLOW_INSECURE_LOCAL_URLS],
    )
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_HTTP_REQUEST_ID])
    cg.add(var.set_request_priority(getattr(ImageRequestPriority, config[CONF_PRIORITY])))
    cg.add(var.set_hardware_acceleration(config[CONF_HARDWARE_ACCELERATION]))
    cg.add(
        var.set_p4_pipeline_priority(
            getattr(P4PipelinePriority, f"P4_PIPELINE_{config[CONF_P4_PIPELINE]}")
        )
    )

    for key, value in config.get(CONF_REQUEST_HEADERS, {}).items():
        if isinstance(value, Lambda):
            template_ = await cg.templatable(value, [], cg.std_string)
            cg.add(var.add_request_header(key, template_))
        else:
            cg.add(var.add_request_header(key, value))

    if placeholder_id := config.get(CONF_PLACEHOLDER):
        placeholder = await cg.get_variable(placeholder_id)
        cg.add(var.set_placeholder(placeholder))

    await automation.build_callback_automations(var, config, _CALLBACK_AUTOMATIONS)
