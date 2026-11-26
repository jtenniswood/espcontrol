#!/usr/bin/env python3
"""Build the shared panel and browser icon font with stable EspControl codepoints."""

import argparse
import re
import tempfile
from pathlib import Path
from urllib.request import urlretrieve

from fontTools.fontBuilder import FontBuilder
from fontTools.misc.roundTools import otRound
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

from build import ASSET_ICON_FONT, ICONS_JSON, MDI_VERSION, ROOT, load_json, web_mdi_icon_codepoints


GOOGLE_REVISION = "10ecc14790612ca8fd164e3f4ffe9c16fc50d5b7"
GOOGLE_SOURCE = (
    f"https://raw.githubusercontent.com/google/material-design-icons/{GOOGLE_REVISION}"
    "/variablefont/MaterialSymbolsOutlined%5BFILL%2CGRAD%2Copsz%2Cwght%5D"
)
ASSETS = ROOT / "common" / "assets"
AXES = {"FILL": 0, "wght": 400, "GRAD": 0, "opsz": 24}
FILLED_SYMBOLS = {0xF5000: "play_arrow", 0xF5001: "pause"}


def icon_slots():
    icons = web_mdi_icon_codepoints(load_json(ICONS_JSON))
    for filename in ("icon_glyphs.yaml", "climate_card_icon_glyphs.yaml", "network_status_glyphs.yaml", "icons.yaml"):
        text = (ASSETS / filename).read_text()
        for codepoint, name in re.findall(
            r'\\U000(F[0-9A-Fa-f]{4})"\s+#\s+mdi-([a-z0-9-]+)',
            text,
        ):
            icons.setdefault(name, codepoint)
        # Include glyphs even when the YAML has no corresponding MDI comment.
        for codepoint in re.findall(r'\\U000(F[0-9A-Fa-f]{4})', text):
            icons.setdefault(codepoint, codepoint)
    return {name: int(codepoint, 16) for name, codepoint in icons.items()}


def generate(source, output):
    slots = icon_slots()
    mapping = load_json(ASSETS / "material_symbols.json")
    google_codepoints = {
        name: int(codepoint, 16)
        for name, codepoint in (line.split() for line in (source / "symbols.codepoints").read_text().splitlines())
    }
    replacements = {}
    for name, symbol in mapping.items():
        slot = slots[name]
        if slot in replacements:
            raise ValueError(f"Multiple Material Symbols mapped to U+{slot:X}")
        replacements[slot] = google_codepoints[symbol]

    mdi = TTFont(ASSETS / "fonts" / f"materialdesignicons-webfont-{MDI_VERSION}.ttf")
    variable_font = TTFont(source / "symbols.ttf")
    google = instantiateVariableFont(variable_font, AXES)
    filled = instantiateVariableFont(variable_font, {**AXES, "FILL": 1}, inplace=True)
    mdi_cmap = mdi.getBestCmap()
    google_cmap = google.getBestCmap()
    google_glyphs = google.getGlyphSet()
    filled_glyphs = filled.getGlyphSet()
    em = mdi["head"].unitsPerEm
    descent = mdi["hhea"].descent
    scale = em / google["head"].unitsPerEm

    glyphs = {".notdef": TTGlyphPen(None).glyph()}
    metrics = {".notdef": (em, 0)}
    cmap = {}
    for codepoint in sorted(set(slots.values())):
        if codepoint in replacements or codepoint in FILLED_SYMBOLS:
            name = mdi_cmap.get(codepoint, f"materialSymbol{codepoint:X}")
            if codepoint in FILLED_SYMBOLS:
                glyph_set = filled_glyphs
                symbol_codepoint = google_codepoints[FILLED_SYMBOLS[codepoint]]
            else:
                glyph_set = google_glyphs
                symbol_codepoint = replacements[codepoint]
            pen = TTGlyphPen(glyph_set)
            symbol = glyph_set[google_cmap[symbol_codepoint]]
            # Google draws from the bottom of the em square. Match the panel
            # font's baseline and advance so existing label layouts still fit.
            symbol.draw(TransformPen(pen, (scale, 0, 0, scale, 0, descent)))
            glyph = pen.glyph()
            glyph.recalcBounds(None)
            metric = (otRound(symbol.width * scale), glyph.xMin)
        else:
            name = mdi_cmap[codepoint]
            glyph = mdi["glyf"][name]
            if glyph.isComposite():
                raise ValueError(f"Fallback glyph {name} must be a simple outline")
            metric = mdi["hmtx"][name]
        glyphs[name] = glyph
        metrics[name] = metric
        cmap[codepoint] = name

    family = "EspControl Icons"
    font = FontBuilder(em, isTTF=True)
    font.setupGlyphOrder(list(glyphs))
    font.setupCharacterMap(cmap)
    font.setupGlyf(glyphs)
    font.setupHorizontalMetrics(metrics)
    font.setupHorizontalHeader(ascent=mdi["hhea"].ascent, descent=descent)
    font.setupNameTable({
        "familyName": family,
        "styleName": "Regular",
        "fullName": family,
        "psName": "EspControlIcons",
        "uniqueFontIdentifier": "EspControlIcons",
        "copyright": "Material Symbols by Google LLC and Material Design Icons by Pictogrammers.",
        "licenseDescription": "Apache License, Version 2.0",
        "licenseInfoURL": "https://www.apache.org/licenses/LICENSE-2.0",
    })
    font.setupOS2(
        sTypoAscender=mdi["OS/2"].sTypoAscender,
        sTypoDescender=mdi["OS/2"].sTypoDescender,
        sTypoLineGap=mdi["OS/2"].sTypoLineGap,
        fsSelection=mdi["OS/2"].fsSelection,
        usWinAscent=mdi["OS/2"].usWinAscent,
        usWinDescent=mdi["OS/2"].usWinDescent,
    )
    font.setupPost()
    # A fixed timestamp keeps the checked-in font reproducible.
    font.font["head"].created = 3786912000
    font.font["head"].modified = 3786912000
    font.font.recalcTimestamp = False
    font.save(output)
    material_count = len(replacements) + len(FILLED_SYMBOLS)
    print(f"{material_count} Material Symbols and {len(cmap) - material_count} MDI glyphs")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, help="Use a previously downloaded symbols.ttf and symbols.codepoints")
    parser.add_argument("--check", action="store_true", help="Verify the checked-in asset is current")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as directory:
        scratch = Path(directory)
        source = args.source_dir or scratch
        if args.source_dir is None:
            for extension in ("ttf", "codepoints"):
                urlretrieve(f"{GOOGLE_SOURCE}.{extension}", source / f"symbols.{extension}")
        output = scratch / ASSET_ICON_FONT.name
        generate(source, output)
        if args.check:
            if output.read_bytes() != ASSET_ICON_FONT.read_bytes():
                parser.exit(1, "Icon font is out of date. Run scripts/generate_icon_font.py.\n")
        else:
            ASSET_ICON_FONT.write_bytes(output.read_bytes())


if __name__ == "__main__":
    main()
