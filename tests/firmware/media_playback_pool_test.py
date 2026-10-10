#!/usr/bin/env python3
"""Exercise the production state allocator at full Cover Art card capacity."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

root = Path(__file__).resolve().parents[2]
media = (root / 'components/espcontrol/button_grid_media.h').read_text()
capacity = re.search(r'^constexpr int MEDIA_PLAYBACK_STATE_MAX = [^;]+;', media, re.M)
assert capacity
source = r'''
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include "button_grid_limits.h"
struct MediaPlaybackState {
  bool used = false;
  uint32_t generation = 0;
  std::string entity_id;
};
uint32_t generation = 1;
uint32_t ha_subscription_generation() { return generation; }
void media_playback_reset_state(MediaPlaybackState *state, const std::string &entity) {
  state->used = true;
  state->generation = generation;
  state->entity_id = entity;
}
#define ESP_LOGW(...) ((void)0)
'''
source += '\n' + capacity.group() + '\n'
for name in ('media_playback_states', 'media_playback_find_state', 'media_playback_ensure_state'):
    line, body = definition(media, name)
    source += f'\n#line {line} "button_grid_media.h"\n{body}\n'
source += r'''
int main() {
  assert(media_playback_ensure_state("") == nullptr);
  // Every main-grid and subpage card has three distinct entities.
  const int cards = MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS;
  for (int card = 0; card < cards; ++card) {
    for (const auto &role : {"main", "artwork", "power"}) {
      const auto entity = std::string(role) + ".card_" + std::to_string(card);
      auto *state = media_playback_ensure_state(entity);
      assert(state && state->entity_id == entity);
      assert(media_playback_ensure_state(entity) == state); // Repeated entities share a slot.
    }
  }
  assert(media_playback_states().size() == static_cast<size_t>(cards * 3));
  // Both the last artwork and power subscriptions remain independently available.
  assert(media_playback_find_state("artwork.card_" + std::to_string(cards - 1)));
  assert(media_playback_find_state("power.card_" + std::to_string(cards - 1)));
  assert(media_playback_ensure_state("beyond_capacity") == nullptr);
  // A configuration generation change reuses the allocated slots.
  ++generation;
  for (int entity = 0; entity < cards * 3; ++entity) {
    assert(media_playback_ensure_state("replacement." + std::to_string(entity)));
  }
  assert(media_playback_states().size() == static_cast<size_t>(cards * 3));
  for (auto *state : media_playback_states()) delete state;
}
'''
with tempfile.TemporaryDirectory(prefix='media-playback-pool-') as directory:
    path = Path(directory)
    (path / 'test.cpp').write_text(source)
    for grid_slots in (25, 16):
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                        f'-DESPCONTROL_MAX_GRID_SLOTS={grid_slots}',
                        '-I', str(root / 'components/espcontrol'), str(path / 'test.cpp'),
                        '-o', str(path / 'test')], check=True)
        subprocess.run([str(path / 'test')], check=True)
print('Production media state pool fits three entities for every card')
