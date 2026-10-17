#pragma once

#include "lvgl.h"
#include "page_store_runtime.h"

namespace espcontrol::pages {
struct PageLabelRefreshMemory {
  size_t arena_bytes{0}, live_labels{0}, retained_callbacks{0};
  size_t descriptor_adds{0}, reused_adds{0}, retained_removals{0};
};

// A selected renderer's fixed, initially blank bank of labels. Initialize and
// shut down outside LVGL dispatch. The bank stays on one screen/display; moving
// it to another display requires shutdown first. The allocator and owner outlive
// all bank objects or shutdown(). This does not cache text or alter label flags.
class PageLabelRefresh {
 public:
  explicit PageLabelRefresh(PageArenaAllocator &allocator) : allocator_(allocator) {}
  ~PageLabelRefresh();
  PageLabelRefresh(const PageLabelRefresh &) = delete;
  PageLabelRefresh &operator=(const PageLabelRefresh &) = delete;
  bool initialize(lv_obj_t *const *labels, size_t count);
  bool shutdown();
  PageLabelRefreshMemory memory() const;

  // Linker-interposition boundary. Unrecognized callbacks/objects/displays
  // always use LVGL's original implementation.
  bool retain_add(lv_display_t *, lv_event_cb_t, lv_event_code_t, void *);
  bool retain_remove(lv_display_t *, lv_event_cb_t, void *);

 private:
  struct Entry { lv_obj_t *label{nullptr}; bool live{false}, registered{false}; };
  Entry *find(void *) const;
  static void object_deleted(lv_event_t *);
  static void display_deleted(lv_event_t *);
  bool registered(lv_obj_t *) const;
  PageArenaAllocator &allocator_;
  Entry *entries_{nullptr};
  size_t count_{0}, attached_{0};
  lv_display_t *display_{nullptr};
  lv_event_cb_t callback_{nullptr};
  PageLabelRefreshMemory memory_{};
};
}  // namespace espcontrol::pages
