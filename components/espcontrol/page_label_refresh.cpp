#include "esphome/core/defines.h"

#ifdef USE_LVGL
#include "page_label_refresh.h"
#include "page_lvgl_dispatch.h"

#include <algorithm>
#include <cassert>
#include <limits>
#include <new>

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 5 || LVGL_VERSION_PATCH != 0
#error "Audit the pooled label refresh callback contract for this LVGL version"
#endif

extern "C" {
void __real_lv_display_add_event_cb(lv_display_t *, lv_event_cb_t, lv_event_code_t, void *);
uint32_t __real_lv_display_remove_event_cb_with_user_data(lv_display_t *, lv_event_cb_t, void *);
#ifdef ESPCONTROL_NATIVE_LABEL_REFRESH
// Native Darwin tests route the pinned lv_label.c calls to the wrappers at
// compile time. Firmware uses GNU ld --wrap and these names resolve to LVGL.
void __real_lv_display_add_event_cb(lv_display_t *d, lv_event_cb_t cb, lv_event_code_t filter, void *user) {
  lv_display_add_event_cb(d, cb, filter, user);
}
uint32_t __real_lv_display_remove_event_cb_with_user_data(lv_display_t *d, lv_event_cb_t cb, void *user) {
  return lv_display_remove_event_cb_with_user_data(d, cb, user);
}
#endif
}

namespace espcontrol::pages {
namespace { PageLabelRefresh *selected = nullptr; }

PageLabelRefresh::~PageLabelRefresh() { const bool closed = shutdown(); assert(closed); (void) closed; }

PageLabelRefresh::Entry *PageLabelRefresh::find(void *object) const {
  const auto key = reinterpret_cast<uintptr_t>(object);
  size_t low = 0, high = count_;
  while (low < high) {
    const auto middle = low + (high - low) / 2;
    if (reinterpret_cast<uintptr_t>(entries_[middle].label) < key) low = middle + 1;
    else high = middle;
  }
  return low < count_ && entries_[low].label == object && entries_[low].live ? entries_ + low : nullptr;
}

bool PageLabelRefresh::registered(lv_obj_t *label) const {
  if (!display_) return false;
  for (uint32_t i = 0; i < lv_display_get_event_count(display_); ++i) {
    auto *event = lv_display_get_event_dsc(display_, i);
    if (event && lv_event_dsc_get_cb(event) == callback_ && lv_event_dsc_get_user_data(event) == label) return true;
  }
  return false;
}

bool PageLabelRefresh::initialize(lv_obj_t *const *labels, size_t count) {
  if (entries_ || selected || !lv_is_initialized() || page_lvgl_dispatching() ||
      !labels || !count || count > SIZE_MAX / sizeof(Entry)) return false;
  auto *display = labels[0] ? lv_obj_get_display(labels[0]) : nullptr;
  auto *screen = labels[0] ? lv_obj_get_screen(labels[0]) : nullptr;
  if (!display || !screen) return false;
  for (size_t i = 0; i < count; ++i) {
    if (!labels[i] || !lv_obj_check_type(labels[i], &lv_label_class) ||
        lv_obj_get_display(labels[i]) != display || lv_obj_get_screen(labels[i]) != screen) return false;
    for (size_t j = 0; j < i; ++j) if (labels[i] == labels[j]) return false;
  }
  auto *storage = static_cast<Entry *>(allocator_.allocate(count * sizeof(Entry)));
  if (!storage) return false;
  entries_ = storage; count_ = count; display_ = display;
  for (size_t i = 0; i < count; ++i) new (entries_ + i) Entry{labels[i], true, false};
  std::sort(entries_, entries_ + count_, [](const Entry &a, const Entry &b) {
    return reinterpret_cast<uintptr_t>(a.label) < reinterpret_cast<uintptr_t>(b.label);
  });
  // Drain constructor/style callbacks, then discover the pinned library's own
  // refresh callback via its public event-descriptor API. No private flag or
  // function address is patched. Blank bank labels are owned by initialization.
  lv_obj_update_layout(labels[0]);
  lv_label_set_text_static(labels[0], "");
  size_t matches = 0;
  for (uint32_t i = 0; i < lv_display_get_event_count(display_); ++i) {
    auto *event = lv_display_get_event_dsc(display_, i);
    if (event && lv_event_dsc_get_user_data(event) == labels[0]) {
      callback_ = lv_event_dsc_get_cb(event); ++matches;
    }
  }
  lv_obj_update_layout(labels[0]);
  if (matches != 1 || !callback_) { shutdown(); return false; }
  for (size_t i = 0; i < count_; ++i) {
    if (!lv_obj_add_event_cb(entries_[i].label, object_deleted, LV_EVENT_DELETE, this)) {
      shutdown(); return false;
    }
    ++attached_;
  }
  const auto before = lv_display_get_event_count(display_);
  __real_lv_display_add_event_cb(display_, display_deleted, LV_EVENT_DELETE, this);
  if (lv_display_get_event_count(display_) != before + 1) { shutdown(); return false; }
  selected = this;
  for (size_t i = 0; i < count_; ++i) {
    lv_label_set_text_static(entries_[i].label, "");
    if (!entries_[i].registered) { shutdown(); return false; }
  }
  lv_obj_update_layout(labels[0]);
  memory_.arena_bytes = count_ * sizeof(Entry);
  return true;
}

bool PageLabelRefresh::retain_add(lv_display_t *display, lv_event_cb_t callback, lv_event_code_t filter, void *user) {
  if (display != display_ || callback != callback_ || filter != LV_EVENT_UPDATE_LAYOUT_COMPLETED) return false;
  auto *entry = find(user); if (!entry) return false;
  if (entry->registered) { ++memory_.reused_adds; return true; }
  __real_lv_display_add_event_cb(display, callback, filter, user);
  entry->registered = registered(entry->label);
  return true;
}
bool PageLabelRefresh::retain_remove(lv_display_t *display, lv_event_cb_t callback, void *user) {
  if (display != display_ || callback != callback_) return false;
  auto *entry = find(user); if (!entry || !entry->registered) return false;
  ++memory_.retained_removals; return true;
}

void PageLabelRefresh::object_deleted(lv_event_t *event) {
  if (lv_event_get_target(event) != lv_event_get_current_target(event)) return;
  auto *self = static_cast<PageLabelRefresh *>(lv_event_get_user_data(event));
  auto *entry = self->find(lv_event_get_target(event)); if (!entry) return;
  if (self->display_ && entry->registered)
    __real_lv_display_remove_event_cb_with_user_data(self->display_, self->callback_, entry->label);
  entry->registered = false; entry->live = false;
}
void PageLabelRefresh::display_deleted(lv_event_t *event) {
  auto *self = static_cast<PageLabelRefresh *>(lv_event_get_user_data(event));
  if (self->display_ != lv_event_get_target(event)) return;
  if (selected == self) selected = nullptr;
  self->display_ = nullptr;
  for (size_t i = 0; i < self->count_; ++i) self->entries_[i].registered = false;
}

bool PageLabelRefresh::shutdown() {
  if (!entries_) return true;
  if (!lv_is_initialized() || page_lvgl_dispatching()) return false;
  // Complete outstanding refreshes before removing their persistent callbacks;
  // surviving bank labels can then return to normal LVGL scheduling safely.
  if (display_) for (size_t i = 0; i < count_; ++i) if (entries_[i].live) {
    lv_obj_update_layout(entries_[i].label); break;
  }
  if (selected == this) selected = nullptr;
  for (size_t i = 0; i < count_; ++i) {
    auto &entry = entries_[i];
    if (display_ && entry.registered)
      __real_lv_display_remove_event_cb_with_user_data(display_, callback_, entry.label);
    if (i < attached_ && entry.live) lv_obj_remove_event_cb_with_user_data(entry.label, object_deleted, this);
  }
  if (display_) __real_lv_display_remove_event_cb_with_user_data(display_, display_deleted, this);
  allocator_.release(entries_, count_ * sizeof(Entry));
  entries_ = nullptr; count_ = attached_ = 0; display_ = nullptr; callback_ = nullptr; memory_ = {};
  return true;
}

PageLabelRefreshMemory PageLabelRefresh::memory() const {
  auto result = memory_;
  for (size_t i = 0; i < count_; ++i) {
    result.live_labels += entries_[i].live;
    result.retained_callbacks += entries_[i].registered;
  }
  return result;
}
}  // namespace espcontrol::pages

extern "C" void __wrap_lv_display_add_event_cb(lv_display_t *d, lv_event_cb_t cb, lv_event_code_t filter, void *user) {
  auto *cache = espcontrol::pages::selected;
  if (!cache || !cache->retain_add(d, cb, filter, user)) __real_lv_display_add_event_cb(d, cb, filter, user);
}
extern "C" uint32_t __wrap_lv_display_remove_event_cb_with_user_data(lv_display_t *d, lv_event_cb_t cb, void *user) {
  auto *cache = espcontrol::pages::selected;
  if (cache && cache->retain_remove(d, cb, user)) return 1;
  return __real_lv_display_remove_event_cb_with_user_data(d, cb, user);
}
#endif  // USE_LVGL
