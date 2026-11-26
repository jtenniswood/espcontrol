"""Detect icons added to the main icon font beyond EspControl's built-in set.

People who compile their own firmware can extend font_icon_main with extra
Material Design Icons. The firmware reports exactly which ones it has, so the
web icon picker only offers glyphs the display can draw, and offers none on
official prebuilt firmware.
"""
import re
from pathlib import Path

ICON_FONT_ID = "font_icon_main"
# Matches the custom icon range accepted by find_icon() in icons.h.
CUSTOM_ICON_MIN = 0xF0000
CUSTOM_ICON_MAX = 0x10FFFD
# Every device builds font_icon_main from this list: the icon table plus a few
# glyphs the firmware draws itself, such as the screensaver's play and pause.
_ICON_GLYPHS_YAML = Path(__file__).resolve().parents[2] / "common" / "assets" / "icon_glyphs.yaml"


def builtin_icon_codepoints(icon_glyphs_text=None):
    """Codepoints of every glyph the official icon font already contains."""
    if icon_glyphs_text is None:
        try:
            icon_glyphs_text = _ICON_GLYPHS_YAML.read_text(encoding="utf-8")
        except OSError as exc:
            raise ValueError(f"Cannot read the shipped icon glyph list: {_ICON_GLYPHS_YAML}") from exc
    codepoints = {int(value, 16) for value in re.findall(r"\\U([0-9A-Fa-f]{8})", icon_glyphs_text)}
    if not codepoints:
        raise ValueError("The shipped icon glyph list is empty")
    return codepoints


def _font_id(font):
    font_id = font.get("id")
    return getattr(font_id, "id", font_id)


def custom_icon_codepoints(fonts, builtin=None):
    """Sorted codepoints in font_icon_main that are not built-in icons.

    Only codepoints in the range the picker and find_icon() accept
    (U+F0000-10FFFD) are returned, since only those can be chosen as icons.
    """
    builtin = builtin_icon_codepoints() if builtin is None else builtin
    for font in fonts or []:
        if _font_id(font) != ICON_FONT_ID:
            continue
        # ESPHome compiles glyphs from extra fonts into the same font, so
        # icons added through `extras` count as well.
        glyphs = list(font.get("glyphs", []) or [])
        for extra in font.get("extras", []) or []:
            glyphs.extend(extra.get("glyphs", []) or [])
        return sorted({
            ord(character)
            for glyph in glyphs
            for character in str(glyph)
            if ord(character) not in builtin
            and CUSTOM_ICON_MIN <= ord(character) <= CUSTOM_ICON_MAX
        })
    return []
