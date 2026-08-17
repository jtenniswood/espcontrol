from esphome.components.mipi import DriverChip
from esphome.const import CONF_SWAP_XY


class RgbDriverChip(DriverChip):
    """A driver chip for MIPI RGB displays."""

    @property
    def transforms(self) -> set[str]:
        """RGB displays support mirroring, but not swapping axes."""
        return super().transforms - {CONF_SWAP_XY}
