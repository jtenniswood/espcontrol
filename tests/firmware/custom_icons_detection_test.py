"""Check detection of icons added to font_icon_main beyond the built-in set."""
import importlib.util
import re
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("custom_icons", ROOT / "components/espcontrol/custom_icons.py")
custom_icons = importlib.util.module_from_spec(spec)
spec.loader.exec_module(custom_icons)


class FontId:
    """Mimics ESPHome's ID object, which exposes the configured name as .id."""

    def __init__(self, value):
        self.id = value


def builtin_glyphs():
    text = (ROOT / "common/assets/icon_glyphs.yaml").read_text(encoding="utf-8")
    return [chr(int(value, 16)) for value in re.findall(r"\\U([0-9A-Fa-f]{8})", text)]


class CustomIconDetectionTest(unittest.TestCase):
    def test_builtin_set_matches_the_shipped_icon_font(self):
        shipped = {ord(glyph) for glyph in builtin_glyphs()}
        self.assertEqual(custom_icons.builtin_icon_codepoints(), shipped)

    def test_official_font_has_no_custom_icons(self):
        fonts = [{"id": FontId("font_icon_main"), "glyphs": builtin_glyphs()}]
        self.assertEqual(custom_icons.custom_icon_codepoints(fonts), [])

    def test_extended_font_reports_each_added_icon(self):
        fonts = [{"id": FontId("font_icon_main"),
                  "glyphs": builtin_glyphs() + ["\U000F194B", "\U000F00E6", "\U000F00E6"]}]
        self.assertEqual(custom_icons.custom_icon_codepoints(fonts), [0xF00E6, 0xF194B])

    def test_extra_font_glyphs_are_reported(self):
        fonts = [{
            "id": FontId("font_icon_main"),
            "glyphs": builtin_glyphs() + ["\U000F194B"],
            "extras": [{
                "file": "mdi.ttf",  # Detector-only fixture; no font loading.
                "glyphs": ["\U000F0EFA"],
            }],
        }]
        self.assertEqual(
            custom_icons.custom_icon_codepoints(fonts),
            [0xF0EFA, 0xF194B],
        )

    def test_multi_character_glyph_strings_are_checked(self):
        fonts = [{"id": "font_icon_main", "glyphs": ["\U000F0493\U000F0EFA"]}]
        self.assertEqual(custom_icons.custom_icon_codepoints(fonts), [0xF0EFA])

    def test_glyphs_outside_the_icon_range_are_not_offered(self):
        fonts = [{"id": "font_icon_main", "glyphs": ["A", "\ue000", "\U000F0EFA", "\U0010FFFD"]}]
        self.assertEqual(custom_icons.custom_icon_codepoints(fonts), [0xF0EFA, 0x10FFFD])

    def test_other_fonts_are_ignored(self):
        fonts = [
            {"id": FontId("font_icon_status"), "glyphs": ["\U000F0EFA"]},
            {"id": FontId("font_icon_main"), "glyphs": builtin_glyphs()},
        ]
        self.assertEqual(custom_icons.custom_icon_codepoints(fonts), [])

    def test_missing_icon_font_reports_no_custom_icons(self):
        self.assertEqual(custom_icons.custom_icon_codepoints([]), [])
        self.assertEqual(custom_icons.custom_icon_codepoints(None), [])

    def test_missing_generated_table_is_an_error(self):
        with self.assertRaises(ValueError):
            custom_icons.builtin_icon_codepoints("no table here")


if __name__ == "__main__":
    unittest.main()
