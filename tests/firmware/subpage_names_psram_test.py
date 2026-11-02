"""Exercise production subpage name storage and click callbacks across rebuilds."""
from pathlib import Path
import os
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "components/espcontrol/button_grid_grid.h"
source = HEADER.read_text()
assert "std::string *sp_entity_ids = subpage_toggle_entity_ids();" in source
assert "static std::string sp_entity_ids[MAX_SUBPAGE_ITEMS]" not in source
line, storage = definition(source, "subpage_toggle_entity_ids")
start = source.index("    auto add_subpage_toggle_click =")
end = source.index("\n    };", start) + len("\n    };")
callback = source[start:end]
harness = r'''
#include <cassert>
#include <string>
#include <vector>
#include "external_memory_allocator.h"
#include "button_grid_limits.h"
struct lv_obj_t;
struct lv_event_t { lv_obj_t *target; void *data; };
struct lv_obj_t {
  void (*callback)(lv_event_t *) = nullptr;
  void *data = nullptr;
  bool checked = false;
};
constexpr int LV_EVENT_CLICKED = 1;
void lv_obj_add_event_cb(lv_obj_t *obj, void (*callback)(lv_event_t *), int, void *data) {
  obj->callback = callback; obj->data = data;
}
void *lv_event_get_target(lv_event_t *event) { return event->target; }
void *lv_event_get_user_data(lv_event_t *event) { return event->data; }
void set_card_checked_state(lv_obj_t *obj, bool checked) { obj->checked = checked; }
std::vector<std::string> actions;
void send_toggle_action(const std::string &entity) { actions.push_back(entity); }
#define ESP_LOGW(...) ((void) 0)
'''
harness += f'\n#line {line} "{HEADER.as_posix()}"\n{storage}\n'
harness += r'''
int main(int argc, char **) {
  const bool fallback = argc > 1;
  fake_esphome_allocator::external_available = !fallback;
  fake_esphome_allocator::internal_available = fallback;
  std::string *sp_entity_ids = subpage_toggle_entity_ids();
  assert(esp_ptr_external_ram(sp_entity_ids) == !fallback);
  const size_t allocated = EspControlExternalAllocatorStats::external_bytes +
                           EspControlExternalAllocatorStats::internal_bytes;
  assert(allocated == sizeof(std::string) * MAX_SUBPAGE_ITEMS);
  for (int i = 0; i < MAX_SUBPAGE_ITEMS; ++i) assert(sp_entity_ids[i].empty());
  int sp_entity_alloc_idx = 0;
'''
harness += callback + r'''
  std::vector<lv_obj_t> buttons(MAX_SUBPAGE_ITEMS + 1);
  for (int i = 0; i < MAX_SUBPAGE_ITEMS; ++i) {
    add_subpage_toggle_click(&buttons[i], "switch.card_" + std::to_string(i), i % 2 == 0);
    assert(buttons[i].data == &sp_entity_ids[i]);
  }
  add_subpage_toggle_click(&buttons.back(), "switch.overflow", false);
  assert(buttons.back().callback == nullptr);
  for (int i = 0; i < MAX_SUBPAGE_ITEMS; ++i) {
    lv_event_t event{&buttons[i], buttons[i].data};
    buttons[i].callback(&event);
    assert(actions.back() == "switch.card_" + std::to_string(i));
    assert(buttons[i].checked == (i % 2 == 0));
  }
  // Grid rebuilds rebind names into the same storage and reuse the index.
  sp_entity_alloc_idx = 0;
  add_subpage_toggle_click(&buttons[0], "switch.rebuilt_" + std::string(200, 'x'), false);
  assert(subpage_toggle_entity_ids() == sp_entity_ids);
  assert(buttons[0].data == &sp_entity_ids[0]);
  lv_event_t event{&buttons[0], buttons[0].data};
  buttons[0].callback(&event);
  assert(actions.back() == "switch.rebuilt_" + std::string(200, 'x'));
  sp_entity_ids[0].clear();
  const size_t sent = actions.size();
  buttons[0].callback(&event);
  assert(actions.size() == sent);
  assert(EspControlExternalAllocatorStats::external_bytes +
         EspControlExternalAllocatorStats::internal_bytes == allocated);
}
'''
with tempfile.TemporaryDirectory(prefix="subpage-psram-") as directory:
    cpp, binary = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(harness)
    for slots in (6, 9, 16, 25):
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-DUSE_ESP32", f"-DESPCONTROL_MAX_GRID_SLOTS={slots}",
                        "-I", str(ROOT / "tests/firmware/stubs"),
                        "-I", str(ROOT / "components/espcontrol"), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        subprocess.run([str(binary), "fallback"], check=True)
print("Subpage PSRAM placement, internal fallback, capacity, click targets and rebuild lifetimes passed.")
