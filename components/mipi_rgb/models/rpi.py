from esphome.components.mipi import DriverChip
from esphome.const import CONF_MIRROR_X, CONF_MIRROR_Y

# A driver chip for Raspberry Pi MIPI RGB displays. These require no init sequence
DriverChip(
    "RPI",
    transforms={CONF_MIRROR_X, CONF_MIRROR_Y},
    initsequence=(),
)
