#!/usr/bin/env python3
"""Generate access-card transient-history parity checks from current production helpers."""
import argparse
import re
from pathlib import Path
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[2]
def function(file, name):
    source = (root / 'components/espcontrol' / file).read_text()
    result = re.findall(rf'^inline [^\n]*\b{name}\([^;]*?\{{\n.*?^\}}', source, re.M | re.S)
    assert len(result) == 1, name
    return result[0]
reference = r'''
struct lv_obj_t { std::string text; };
struct lv_timer_t { void* user{nullptr}; uint32_t reset_ms{0}; bool paused{true}; };
void lv_label_set_display_text(lv_obj_t* label,const char* text){label->text=text;}
void lv_timer_pause(lv_timer_t* timer){timer->paused=true;}
void lv_timer_resume(lv_timer_t* timer){timer->paused=false;}
void lv_timer_reset(lv_timer_t* timer){timer->reset_ms=tick;}
void* lv_timer_get_user_data(lv_timer_t* timer){return timer->user;}
'''
source = (root / 'components/espcontrol/button_grid_access_cards.h').read_text()
reference += re.search(r'^struct TransientStatusLabel \{.*?^\};', source, re.M | re.S)[0]
for name in ['transient_status_label_revert_cb','transient_status_label_show_if_changed']:
    reference += '\n' + function('button_grid_access_cards.h', name)
for name in ['garage_state_label','garage_state_releases_label','garage_state_is_active','garage_state_uses_open_icon','lock_state_label','lock_state_is_active','lock_state_uses_unlocked_icon','lock_state_releases_label']:
    reference += '\n' + function('button_grid_access_cards.h', name)
template = Path(__file__).with_name('page_access_state_runtime_test.cpp.in').read_text()
assert template.count('// @legacy-access-reference') == 1
args.output.write_text(template.replace('// @legacy-access-reference', reference))
