"""Detect icons added to the main icon font beyond EspControl's built-in set.

People who compile their own firmware can extend font_icon_main with extra
Material Design Icons. The web icon picker only offers custom icons when the
firmware reports that it has some, so official prebuilt firmware never does.
"""
import re
from pathlib import Path

ICON_FONT_ID = "font_icon_main"
# The cog is the fallback icon and is always in the font (see icon_glyphs.yaml).
FALLBACK_ICON_CODEPOINT = 0xF0493
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


def icon_font_has_custom_glyphs(fonts, builtin=None):
    """True when font_icon_main lists any glyph outside the built-in icons."""
    builtin = builtin_icon_codepoints() if builtin is None else builtin
    for font in fonts or []:
        if _font_id(font) != ICON_FONT_ID:
            continue
        return any(
            ord(character) not in builtin
            for glyph in font.get("glyphs", []) or []
            for character in str(glyph)
        )
    return False
