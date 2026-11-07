#!/usr/bin/env python3
"""Generate real media contexts, snapshot policy and control events for native LVGL."""
import argparse
import re
from pathlib import Path
from generate_climate_update_test import definition
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
media = (root/'components/espcontrol/button_grid_media.h').read_text()
sliders = (root/'components/espcontrol/button_grid_sliders.h').read_text()
group = (root/'components/espcontrol/button_grid_media_group.h').read_text()
text = Path(__file__).with_name('page_media_control_modal_test.cpp.in').read_text()
text = text.replace('// @group-policy', group[group.index('constexpr uint64_t MEDIA_PLAYER_FEATURE_GROUPING'):group.index('inline uint32_t next_media_group_call_id')])
contexts = []
for name in ('MediaControlCtx', 'MediaSpeakerRowState', 'MediaControlModalUi', 'MediaControlStateSnapshot'):
    contexts.append(re.search(r'^struct '+name+r' \{\n.*?^\};', media, re.MULTILINE|re.DOTALL)[0])
text = text.replace('// @contexts', '\n'.join(contexts))
text = text.replace('// @production-function create_media_control_context', definition((root/'components/espcontrol/button_grid_actions.h').read_text(), 'media_card_mode')+'\n// @production-function create_media_control_context')
for name in re.findall(r'// @production-function (\w+)', text):
    source = sliders if name in ('media_clamp_percent', 'media_volume_set_button_enabled', 'media_format_time') else media
    text = text.replace('// @production-function '+name, definition(source, name))
args.output.write_text('// Generated from production media panel helpers.\n'+text)
