#!/usr/bin/env python3
"""Keep the production Now Playing layout as the native comparison reference."""
import argparse
from pathlib import Path
from generate_climate_update_test import definition
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
source = (root / 'components/espcontrol/button_grid_media.h').read_text()
text = '''#pragma once
namespace legacy_now_playing_reference {
struct CardPadding { lv_coord_t left{0},top{0},right{0},bottom{0}; };
inline void apply_push_button_transition(lv_obj_t*) {}
inline void lv_label_set_display_text(lv_obj_t* label,const char* text) {
  const auto display=normalize_display_text(text);lv_label_set_text(label,display.c_str());
}
'''
text += definition(source, 'setup_media_now_playing_layout') + '\n}\n'
args.output.write_text(text)
