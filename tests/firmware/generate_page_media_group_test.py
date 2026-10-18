#!/usr/bin/env python3
"""Compare the allocation-free group indicator with the production legacy parser."""
import argparse
from pathlib import Path
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',required=True,type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[2]
legacy=(root/'components/espcontrol/button_grid_media_group.h').read_text()
pure=legacy[legacy.index('constexpr uint64_t MEDIA_PLAYER_FEATURE_GROUPING'):legacy.index('inline uint32_t next_media_group_call_id')]
text=Path(__file__).with_name('page_media_group_test.cpp.in').read_text()
args.output.write_text('// Generated production grouping reference.\n'+text.replace('// @legacy-group-parser',pure))
