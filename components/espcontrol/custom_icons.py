"""Detect icons added to the main icon font beyond EspControl's built-in set.

People who compile their own firmware can extend font_icon_main with extra
Material Design Icons. The firmware reports exactly which ones it has, so the
web icon picker only offers glyphs the display can draw, and offers none on
official prebuilt firmware.
"""
import re
from pathlib import Path

ICON_FONT_ID = "font_icon_main"
# The cog is the fallback icon and is always in the font (see icon_glyphs.yaml).
FALLBACK_ICON_CODEPOINT = 0xF0493
# Matches the custom icon range accepted by find_icon() in icons.h.
CUSTOM_ICON_MIN = 0xF0000
CUSTOM_ICON_MAX = 0x10FFFD
_ICONS_H = Path(__file__).with_name("icons.h")


def builtin_icon_codepoints(icons_h_text=None):
    """Codepoints of the generated built-in icon table plus the cog fallback."""
    text = _ICONS_H.read_text(encoding="utf-8") if icons_h_text is None else icons_h_text
    try:
        block = text.split("GENERATED:ICONS START", 1)[1].split("GENERATED:ICONS END", 1)[0]
    except IndexError as exc:
        raise ValueError("icons.h is missing the generated icon table") from exc
    codepoints = {int(value, 16) for value in re.findall(r"\\U([0-9A-Fa-f]{8})", block)}
    if not codepoints:
        raise ValueError("icons.h generated icon table is empty")
    return codepoints | {FALLBACK_ICON_CODEPOINT}


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
