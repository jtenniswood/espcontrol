"""Exercise production subpage cleanup with a live Timer runtime."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / 'components/espcontrol/button_grid_navigation.h').read_text()
start = header.index('inline void navigation_clear_subpages()')
end = header.index('\ninline void navigation_register_home_target', start)
# Reuse the Timer test's LVGL/HA doubles, retaining the production Timer class.
stubs = (ROOT / 'tests/firmware/timer_card_test.cpp').read_text().split('int main() {')[0]
source = stubs + r'''
#include <vector>
struct NavigationSubpageEntry { lv_obj_t *screen; TimerCardCtx *timer; };
std::vector<NavigationSubpageEntry> entries;
auto &navigation_subpages() { return entries; }
lv_obj_t home;
lv_obj_t *lv_scr_act() { return &home; }
int deleted = 0;
void navigation_release_subpage_runtime(NavigationSubpageEntry &entry) {
  delete entry.timer;
  entry.timer = nullptr;
}
void lv_obj_del(lv_obj_t *screen) {
  // LVGL must never delete labels while a timer can still refresh them.
  assert(timers == 0 && callbacks.empty());
  ++deleted;
  delete screen;
}
struct NavigationService { void clear_subpages() { entries.clear(); } } service;
auto &grid_navigation_service() { return service; }
void clock_bar_clear_button_grid_pages() {}
''' + header[start:end] + r'''
int main() {
  for (int rebuild = 0; rebuild < 5; ++rebuild) {
    auto *screen = new lv_obj_t;
    auto *timer = new TimerCardCtx;
    timer->entity_id = "timer.kitchen";
    timer->value_lbl = screen;
    subscribe_timer_card(timer);
    timer->tick_timer = lv_timer_create(timer_card_tick_cb, 250, timer);
    timer->confirm.timeout_timer = lv_timer_create(confirmation_timeout_cb, 3000, &timer->confirm);
    entries.push_back({screen, timer});
    navigation_clear_subpages();
    assert(entries.empty() && timers == 0 && callbacks.empty());
    assert(deleted == rebuild + 1);
  }
}
'''
with tempfile.TemporaryDirectory(prefix='timer-subpage-test-') as directory:
    cpp = Path(directory) / 'test.cpp'
    binary = Path(directory) / 'test'
    cpp.write_text(source)
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-I', str(ROOT / 'components/espcontrol'), str(cpp), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('Timer subpage cleanup regression passed.')
