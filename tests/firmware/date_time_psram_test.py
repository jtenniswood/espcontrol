"""Exercise production registry allocation, capacity, compaction and reset."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
from generate_media_lifecycle_integration import definition

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "components/espcontrol/button_grid_datetime_cards.h"
source = HEADER.read_text()
harness = r'''
#include <cassert>
#include <ctime>
#include <string>
#include <vector>
#include "external_memory_allocator.h"
#include "button_grid_limits.h"
struct lv_obj_t { bool valid = true; int renders = 0; };
bool lv_obj_is_valid(lv_obj_t *obj) { return obj && obj->valid; }
#define ESP_LOGW(...) ((void) 0)
'''
for name in ("CalendarCardRef", "CalendarDateState"):
    match = re.search(r"struct " + name + r" \{[\s\S]*?\n\};", source)
    assert match, name
    harness += match.group() + "\n"
harness += r'''
void apply_calendar_card_text(const CalendarCardRef &ref,
                              const CalendarDateState &) { ++ref.value_lbl->renders; }
'''
for name in ('calendar_card_refs', 'calendar_card_count', 'calendar_date_state', 'calendar_card_ref_ready', 'compact_calendar_card_refs', 'reset_calendar_cards', 'register_calendar_card'):
    line, body = definition(source, name)
    harness += f'\n#line {line} "{HEADER.as_posix()}"\n{body}\n'
harness += r'''
int main(int argc, char **) {
  const bool fallback = argc > 1;
  fake_esphome_allocator::external_available = !fallback;
  fake_esphome_allocator::internal_available = fallback;
  auto *refs = calendar_card_refs();
  const int capacity = MAX_GRID_SLOTS + MAX_SUBPAGE_ITEMS;
  assert(esp_ptr_external_ram(refs) == !fallback);
  const size_t allocated = EspControlExternalAllocatorStats::external_bytes +
                           EspControlExternalAllocatorStats::internal_bytes;
  assert(allocated == capacity * sizeof(CalendarCardRef));
  std::vector<lv_obj_t> labels(capacity + 1);
  for (int i = 0; i < capacity; ++i) {
    assert(refs[i].value_lbl == nullptr);
    assert(refs[i].unit_lbl == nullptr);
    assert(refs[i].label_lbl == nullptr);
    assert(!refs[i].show_time);
    register_calendar_card(&labels[i], &labels[i], &labels[i], true);
    assert(labels[i].renders == 1);
  }
  assert(calendar_card_count() == capacity);
  register_calendar_card(&labels.back(), &labels.back(), &labels.back(), true);
  assert(calendar_card_count() == capacity && labels.back().renders == 0);
  labels[0].valid = false;
  compact_calendar_card_refs();
  assert(calendar_card_count() == capacity - 1);
  assert(refs[0].value_lbl == &labels[1]);
  assert(refs[capacity - 2].value_lbl == &labels[capacity - 1]);
  register_calendar_card(&labels.back(), &labels.back(), &labels.back(), true);
  assert(calendar_card_count() == capacity && labels.back().renders == 1);
  reset_calendar_cards();
  assert(calendar_card_count() == 0);
  assert(calendar_card_refs() == refs);
  register_calendar_card(&labels.back(), &labels.back(), &labels.back(), true);
  assert(calendar_card_count() == 1 && refs[0].value_lbl == &labels.back());
  assert(EspControlExternalAllocatorStats::external_bytes +
         EspControlExternalAllocatorStats::internal_bytes == allocated);
}
'''
with tempfile.TemporaryDirectory(prefix="registry-psram-") as directory:
    cpp, binary = Path(directory) / "test.cpp", Path(directory) / "test"
    cpp.write_text(harness)
    for slots in (6, 9, 16, 25):
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-DUSE_ESP32", f"-DESPCONTROL_MAX_GRID_SLOTS={slots}",
                        "-I", str(ROOT / "tests/firmware/stubs"),
                        "-I", str(ROOT / "components/espcontrol"), str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        subprocess.run([str(binary), "fallback"], check=True)
print("Registry PSRAM placement, internal fallback, full capacity, compaction, reset and stable addresses passed.")
